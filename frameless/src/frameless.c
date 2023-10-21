#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// maximum number of windows
#define WIN_MAX_COUNT 128

// pixels to show of a window folded to the right
#define WIN_SLIP_DX 21

// random pixels relative to WIN_SLIP_DX
#define WIN_SLIP 7

// window border width
#define WIN_BORDER_WIDTH 1

#define WIN_BORDER_ACTIVE_COLOR 0x00008000
#define WIN_BORDER_INACTIVE_COLOR 0x00000000

// pixels to 'bump' a window
#define WIN_BUMP_PX 200

// xwin.bits
#define XWIN_BIT_FULL_HEIGHT 1
#define XWIN_BIT_FULL_WIDTH 2
#define XWIN_BIT_ALLOCATED 4
#define XWIN_BIT_FOCUSED 8
#define XWIN_BITS_FULL_SCREEN 3
#define XWIN_MIN_WIDTH_HEIGHT 4

typedef struct xwin {
  Window win;     // x11 window handle
  int x;          // position x
  int y;          // position y
  unsigned wi;    // width
  unsigned hi;    // height
  int x_pf;       // x pre full width / height / screen
  int y_pf;       // y pre full width / height / screen
  unsigned wi_pf; // wi pre full width / height / screen
  unsigned hi_pf; // hi pre full width / height / screen
  int desk;       // desk the window is on
  int desk_x;     // x coord of window before folded at desk switch
  // bit 1: fullheight  bit 2: fullwidth  bit 3: allocated, bit 4: focused
  char bits;
} xwin;

// mapped xwin to X11 Window
static xwin wins[WIN_MAX_COUNT];

// log file
static FILE *flog;

// default display
static Display *dpy;

// root window
static Window root;

// current desk
static int dsk;

// number of windows mapped
static unsigned xwin_count;

// default screen info
static struct screen {
  unsigned id;
  unsigned wi;
  unsigned hi;
} screen;

// current key pressed
static unsigned key_pressed;

// current focused window
static xwin *win_focused;

// dragging state
static char is_dragging;
static int dragging_start_x;
static int dragging_start_y;
static unsigned dragging_button;

// static char *ix_evnames[LASTEvent] = {
//     "unknown",          "unknown",       // 0
//     "KeyPress",         "KeyRelease",    // 2
//     "ButtonPress",      "ButtonRelease", // 4
//     "MotionNotify",                      // 6
//     "EnterNotify",      "LeaveNotify",   // 7 LeaveWindowMask LeaveWindowMask
//     "FocusIn",          "FocusOut",      // 9 from XSetFocus
//     "KeymapNotify",                      // 11
//     "Expose",           "GraphicsExpose",   "NoExpose", // 12
//     "VisibilityNotify", "CreateNotify",     "DestroyNotify",
//     "UnmapNotify",      "MapNotify", // 15
//     "MapRequest",       "ReparentNotify",   "ConfigureNotify",
//     "ConfigureRequest", "GravityNotify",    "ResizeRequest",
//     "CirculateNotify",  "CirculateRequest", "PropertyNotify",
//     "SelectionClear",   "SelectionRequest", "SelectionNotify",
//     "ColormapNotify",   "ClientMessage",    "MappingNotify",
//     "GenericEvent"};

static xwin *xwin_get_by_window(Window w) {
  int first_avail = -1;
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (wins[i].bits & XWIN_BIT_ALLOCATED) {
      if (wins[i].win == w) {
        return &wins[i];
      }
    } else {
      if (first_avail == -1) {
        first_avail = (int)i;
      }
    }
  }
  if (first_avail == -1) {
    fprintf(flog, "!!! no free windows\n");
    exit(-1);
  }
  xwin *xw = &wins[first_avail];
  xw->bits = XWIN_BIT_ALLOCATED;
  xwin_count++;
  // fprintf(flog, "windows allocated: %d\n", wincount);
  xw->win = w;
  xw->desk = dsk;
  XSetWindowBorderWidth(dpy, w, WIN_BORDER_WIDTH);
  return xw;
}

static void xwin_focus(xwin *this) {
  if (win_focused) {
    XSetWindowBorder(dpy, win_focused->win, WIN_BORDER_INACTIVE_COLOR);
    win_focused->bits &= ~XWIN_BIT_FOCUSED;
  }
  XSetInputFocus(dpy, this->win, RevertToParent, CurrentTime);
  XSetWindowBorder(dpy, this->win, WIN_BORDER_ACTIVE_COLOR);
  this->bits |= XWIN_BIT_FOCUSED;
  win_focused = this;
}

static void xwin_raise(xwin *this) { XRaiseWindow(dpy, this->win); }

static xwin *winx_find_by_window(Window w) {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (wins[i].win == w)
      return &wins[i];
  }
  return NULL;
}

static void xwin_free_by_window(Window w) {
  xwin *win = winx_find_by_window(w);
  if (!win) {
    return;
  }
  if (win->bits & XWIN_BIT_ALLOCATED) {
    win->bits = 0; // mark free
    xwin_count--;
  }
  if (win_focused == win) {
    win_focused = NULL;
  }
}

static void xwin_read_geom(xwin *this) {
  Window wsink;
  unsigned dummy;
  XGetGeometry(dpy, this->win, &wsink, &this->x, &this->y, &this->wi, &this->hi,
               &dummy, &dummy);
}

static void xwin_set_geom(xwin *this) {
  XMoveResizeWindow(dpy, this->win, this->x, this->y, this->wi, this->hi);
}

static void xwin_center(xwin *this) {
  xwin_read_geom(this);
  this->x = (int)((screen.wi - this->wi) >> 1);
  this->y = (int)((screen.hi - this->hi) >> 1);
  xwin_set_geom(this);
}

static void xwin_wider(xwin *this) {
  xwin_read_geom(this);
  unsigned wi_prv = this->wi;
  this->wi = ((this->wi << 2) + this->wi) >> 2;
  this->x = this->x - (int)((this->wi - wi_prv) >> 1);
  xwin_set_geom(this);
}

static void xwin_thinner(xwin *this) {
  xwin_read_geom(this);
  unsigned wi_prv = this->wi;
  this->wi = ((this->wi << 1) + this->wi) >> 2;
  if (this->wi < XWIN_MIN_WIDTH_HEIGHT) {
    this->wi = XWIN_MIN_WIDTH_HEIGHT;
  }
  this->x = this->x - (int)((this->wi - wi_prv) >> 1);
  xwin_set_geom(this);
}

static void xwin_close(xwin *this) {
  XEvent ke;
  ke.type = ClientMessage;
  ke.xclient.window = this->win;
  ke.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
  ke.xclient.format = 32;
  ke.xclient.data.l[0] = (long)XInternAtom(dpy, "WM_DELETE_WINDOW", True);
  ke.xclient.data.l[1] = CurrentTime;
  XSendEvent(dpy, this->win, False, NoEventMask, &ke);
}

static void xwin_toggle_fullscreen(xwin *this) {
  if ((this->bits & (XWIN_BITS_FULL_SCREEN)) == (XWIN_BITS_FULL_SCREEN)) {
    // toggle from fullscreen
    xwin_read_geom(this);
    this->x = this->x_pf;
    this->y = this->y_pf;
    this->wi = this->wi_pf;
    this->hi = this->hi_pf;
    xwin_set_geom(this);
    this->bits &= ~XWIN_BITS_FULL_SCREEN;
  } else {
    // toggle to fullscreen
    xwin_read_geom(this);
    this->x_pf = this->x;
    this->y_pf = this->y;
    this->wi_pf = this->wi;
    this->hi_pf = this->hi;
    this->x = -WIN_BORDER_WIDTH;
    this->y = -WIN_BORDER_WIDTH;
    this->wi = screen.wi;
    this->hi = screen.hi;
    xwin_set_geom(this);
    this->bits |= XWIN_BITS_FULL_SCREEN;
  }
}

static void xwin_toggle_fullheight(xwin *this) {
  if (this->bits & XWIN_BIT_FULL_HEIGHT) {
    xwin_read_geom(this);
    this->y = this->y_pf;
    this->hi = this->hi_pf;
    xwin_set_geom(this);
  } else {
    xwin_read_geom(this);
    this->y_pf = this->y;
    this->hi_pf = this->hi;
    this->y = -WIN_BORDER_WIDTH;
    this->hi = screen.hi;
    xwin_set_geom(this);
  }
  this->bits ^= XWIN_BIT_FULL_HEIGHT;
}

static void xwin_toggle_fullwidth(xwin *this) {
  if (this->bits & XWIN_BIT_FULL_WIDTH) {
    xwin_read_geom(this);
    this->x = this->x_pf;
    this->wi = this->wi_pf;
    xwin_set_geom(this);
  } else {
    xwin_read_geom(this);
    this->x_pf = this->x;
    this->wi_pf = this->wi;
    this->x = -WIN_BORDER_WIDTH;
    this->wi = screen.wi;
    xwin_set_geom(this);
  }
  this->bits ^= XWIN_BIT_FULL_WIDTH;
}

static void xwin_hide(xwin *this) {
  xwin_read_geom(this);
  this->desk_x = this->x;
  unsigned slip = (unsigned)(rand() % WIN_SLIP);
  this->x = (int)(screen.wi - WIN_SLIP_DX + slip);
  xwin_set_geom(this);
}

static void xwin_show(xwin *this) {
  this->x = this->desk_x;
  xwin_set_geom(this);
}

static void xwin_bump(xwin *this, int rand_amt) {
  xwin_read_geom(this);
  this->x += (rand() % rand_amt) - (rand_amt >> 1);
  this->y += (rand() % rand_amt) - (rand_amt >> 1);
  xwin_set_geom(this);
}

static int xwin_ix(xwin *this) {
  if (this == NULL) {
    return -1;
  }
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (this == &wins[i]) {
      return (int)i;
    }
  }
  return -1;
}

static char xwin_focus_try_by_index(unsigned ix) {
  xwin *xw = &wins[ix];
  if ((xw->bits & XWIN_BIT_ALLOCATED) && (xw->desk == dsk)) {
    xwin_raise(xw);
    xwin_focus(xw);
    return True;
  }
  return False;
}

static void desk_show(int dsk_id, int dsk_prv) {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    xwin *xw = &wins[i];
    if (!(xw->bits & XWIN_BIT_ALLOCATED)) {
      continue;
    }
    if (xw->win == 0) {
      continue;
    }
    if (xw->win == root) {
      continue;
    }
    if (xw->desk == dsk_prv) {
      xwin_hide(xw);
    }
    if (xw->desk == dsk_id) {
      xwin_show(xw);
    }
  }
}

static void focus_first_window_on_desk(void) {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    xwin *xw = &wins[i];
    if ((xw->bits & XWIN_BIT_ALLOCATED) && (xw->desk == dsk)) {
      xwin_raise(xw);
      xwin_focus(xw);
      return;
    }
  }
  win_focused = NULL;
}

static void focus_on_only_window_on_desk(void) {
  xwin *focus = NULL;
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    xwin *xw = &wins[i];
    if ((xw->bits & XWIN_BIT_ALLOCATED) && (xw->desk == dsk)) {
      if (focus) {
        return; // there are more than 1 window on this desk
      }
      focus = xw;
    }
  }
  if (focus) {
    xwin_focus(focus);
  }
}

// focus on previously focused window or some window on this desktop
static void focus_window_after_desk_switch(void) {
  xwin *focus = NULL;
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    xwin *xw = &wins[i];
    if ((xw->bits & XWIN_BIT_ALLOCATED) && (xw->desk == dsk)) {
      if (xw->bits & XWIN_BIT_FOCUSED) {
        // found focused window on this desk
        focus = xw;
        break;
      }
      // focus on some window on this desk
      focus = xw;
    }
  }
  win_focused = NULL;
  if (!focus) {
    // didn't find any window to focus
    return;
  }
  xwin_focus(focus);
}

static void focus_next_window(void) {
  int i0 = xwin_ix(win_focused);
  int i = i0;
  while (++i < WIN_MAX_COUNT) {
    if (xwin_focus_try_by_index((unsigned)i)) {
      return;
    }
  }
  i = 0;
  while (i <= i0) {
    if (xwin_focus_try_by_index((unsigned)i)) {
      return;
    }
    i++;
  }
  focus_first_window_on_desk();
}

static void focus_previous_window(void) {
  int i0 = xwin_ix(win_focused);
  int i = i0;
  while (--i >= 0) {
    if (xwin_focus_try_by_index((unsigned)i)) {
      return;
    }
  }
  i = WIN_MAX_COUNT;
  while (--i >= 0) {
    if (xwin_focus_try_by_index((unsigned)i)) {
      return;
    }
  }
  focus_first_window_on_desk();
}

static int error_handler(Display *d, XErrorEvent *e) {
  char buffer_return[1024] = "";
  XGetErrorText(d, e->error_code, buffer_return, sizeof(buffer_return));
  fprintf(flog, "!!! x11 error\n");
  fprintf(flog, "!!!       text: %s\n", buffer_return);
  fprintf(flog, "!!!       type: %d\n", e->type);
  fprintf(flog, "!!! resourceid: %p\n", (void *)e->resourceid);
  fprintf(flog, "!!! error code: %d\n", (unsigned)e->error_code);
  fflush(flog);
  return 0;
}

int main(int argc, char **args, char **env) {
  while (argc--) {
    puts(*args++);
  }
  while (*env) {
    puts(*env++);
  }

  puts("frameless window manager");

  XSetErrorHandler(error_handler);

  flog = stdout; // fopen(logfile,"a");
  if (!flog) {
    exit(1);
  }

  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    exit(2);
  }

  screen.id = (unsigned)DefaultScreen(dpy);
  screen.wi = (unsigned)DisplayWidth(dpy, screen.id);
  screen.hi = (unsigned)DisplayHeight(dpy, screen.id);

  root = DefaultRootWindow(dpy);
  Cursor root_window_cursor = XCreateFontCursor(dpy, XC_arrow);
  XDefineCursor(dpy, root, root_window_cursor);
  XGrabKey(dpy, AnyKey, Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, AnyKey, Mod4Mask + ShiftMask, root, True, GrabModeAsync,
           GrabModeAsync);
  XSelectInput(dpy, root, SubstructureNotifyMask);

  // previous desk
  int dsk_prv = 0;
  // temporary
  xwin *xw = NULL;
  XEvent ev;
  while (!XNextEvent(dpy, &ev)) {
    // fprintf(flog, "event: %s   win=%p\n", ix_evnames[ev.type],
    //         (void *)ev.xany.window);
    // fflush(flog);
    switch (ev.type) {
    default:
      // fprintf(flog, "  unhandled\n");
      // fflush(flog);
      break;
    case MapNotify:
      if (ev.xmap.window == root || ev.xmap.window == 0 ||
          ev.xmap.override_redirect) {
        break;
      }
      xw = xwin_get_by_window(ev.xmap.window);
      xwin_center(xw);
      xwin_focus(xw);
      XGrabButton(dpy, AnyButton, Mod4Mask, xw->win, True, ButtonPressMask,
                  GrabModeAsync, GrabModeAsync, None, None);
      XSelectInput(dpy, xw->win, EnterWindowMask);
      break;
    case UnmapNotify:
      if (ev.xmap.window == root || ev.xmap.window == 0 ||
          ev.xmap.override_redirect) {
        break;
      }
      xwin_free_by_window(ev.xmap.window);
      focus_on_only_window_on_desk();
      break;
    case EnterNotify:
      if (is_dragging || key_pressed) {
        // if dragging then it is resizing, don't change focus
        // if key pressed then it is switching desktop, ignore
        //   focus on the window that is under the pointer
        //? todo. check that keys for switching desktop are pressed
        break;
      }
      xw = xwin_get_by_window(ev.xcrossing.window);
      xwin_focus(xw);
      break;
    case KeyPress:
      key_pressed = ev.xkey.keycode;
      if (ev.xkey.subwindow) {
        xw = xwin_get_by_window(ev.xkey.subwindow);
      }
      switch (key_pressed) {
      case 53: // x
        system("xii-sticky");
        break;
      case 54: // c
        system("xii-term");
        break;
      case 33: // p
        system("xii-screenshot");
        break;
      case 24: // q
        system("xii-bin");
        break;
      case 31: // i
        system("xii-internet");
        break;
      case 58: // m
        system("xii-media");
        break;
      case 41: // f
        system("xii-files");
        break;
      case 55: // v
        system("xii-mixer");
        break;
      case 26: // e
        system("xii-editor");
        break;
      case 68: // F2
        system("xii-decrease-screen-brightness");
        break;
      case 69: // F3
        system("xii-increase-screen-brightness");
        break;
      case 72: // F6
        system("xii-vol-toggle");
        break;
      case 73: // F7
        system("xii-vol-down");
        break;
      case 74: // F8
        system("xii-vol-up");
        break;
      case 9:  // esc
      case 49: // §
        if (win_focused) {
          xwin_close(win_focused);
        }
        break;
      case 39: // s
        if (win_focused) {
          xwin_center(win_focused);
        }
        break;
      case 25: // w
        if (win_focused) {
          if (ev.xkey.state & ShiftMask) {
            xwin_thinner(win_focused);
          } else {
            xwin_wider(win_focused);
          }
          // if window was full width then turn that bit off
          if (win_focused->bits & XWIN_BIT_FULL_WIDTH) {
            win_focused->bits &= ~XWIN_BIT_FULL_WIDTH;
          }
        }
        break;
      case 56: // b
        if (win_focused) {
          xwin_bump(win_focused, WIN_BUMP_PX);
        }
        break;
      case 10: // 1
        if (win_focused) {
          xwin_toggle_fullscreen(win_focused);
        }
        break;
      case 11: // 2
        if (win_focused) {
          xwin_toggle_fullheight(win_focused);
        }
        break;
      case 12: // 3
        if (win_focused) {
          xwin_toggle_fullwidth(win_focused);
        }
        break;
      case 19: // 0: surprise
        if (win_focused) {
          xwin_bump(win_focused, WIN_BUMP_PX);
        }
        break;
      case 113: // left
        focus_previous_window();
        break;
      case 52:  // z
      case 114: // right
        focus_next_window();
        break;
      case 119: // del
        XKillClient(dpy, ev.xkey.subwindow);
        break;
      case 38:  // a
      case 111: // up
        dsk_prv = dsk;
        dsk++;
        if (ev.xkey.state & ShiftMask) {
          if (win_focused) {
            win_focused->desk = dsk;
            // set 'desk_x' because 'desk_show' will restore it to 'x'
            win_focused->desk_x = win_focused->x;
            xwin_raise(win_focused);
          }
        }
        desk_show(dsk, dsk_prv);
        focus_window_after_desk_switch();
        // fprintf(flog, "switched to desktop %d from %d\n", dsk, dsk_prv);
        // fflush(flog);
        break;
      case 40:  // d
      case 116: // down
        dsk_prv = dsk;
        dsk--;
        if (ev.xkey.state & ShiftMask) {
          if (win_focused) {
            win_focused->desk = dsk;
            // set 'desk_x' because 'desk_show' will restore it to 'x'
            win_focused->desk_x = win_focused->x;
            xwin_raise(win_focused);
          }
        }
        desk_show(dsk, dsk_prv);
        focus_window_after_desk_switch();
        // fprintf(flog, "switched to desktop %d from %d\n", dsk, dsk_prv);
        // fflush(flog);
        break;
      }
      break;
    case KeyRelease:
      if (key_pressed == ev.xkey.keycode) {
        key_pressed = 0;
      }
      break;
    case ButtonPress:
      is_dragging = True;
      dragging_start_x = ev.xbutton.x_root;
      dragging_start_y = ev.xbutton.y_root;
      dragging_button = ev.xbutton.button;
      xw = xwin_get_by_window(ev.xbutton.window);
      xwin_focus(xw);
      XGrabPointer(dpy, xw->win, True, PointerMotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
      xwin_raise(xw);
      xwin_read_geom(xw);
      break;
    case MotionNotify:
      while (XCheckTypedEvent(dpy, MotionNotify, &ev))
        ;
      int xdiff = ev.xbutton.x_root - dragging_start_x;
      int ydiff = ev.xbutton.y_root - dragging_start_y;
      dragging_start_x = ev.xbutton.x_root;
      dragging_start_y = ev.xbutton.y_root;
      if (!xw) {
        // case should not happen because 'xw' is set wind dragging startss
        break;
      }
      int new_x = xw->x + xdiff;
      unsigned new_wi = (unsigned)((int)xw->wi + xdiff);
      int new_y = xw->y + ydiff;
      unsigned new_hi = (unsigned)((int)xw->hi + ydiff);
      if (xw->bits & XWIN_BIT_FULL_WIDTH) {
        new_x = -WIN_BORDER_WIDTH;
        new_wi = screen.wi;
      }
      if (xw->bits & XWIN_BIT_FULL_HEIGHT) {
        new_y = -WIN_BORDER_WIDTH;
        new_hi = screen.hi;
      }
      if (dragging_button == 3) {
        if (new_wi < XWIN_MIN_WIDTH_HEIGHT) {
          new_wi = XWIN_MIN_WIDTH_HEIGHT;
        }
        if (new_hi < XWIN_MIN_WIDTH_HEIGHT) {
          new_hi = XWIN_MIN_WIDTH_HEIGHT;
        }
        xw->wi = new_wi;
        xw->hi = new_hi;
        xwin_set_geom(xw);
        break;
      }
      switch (key_pressed) {
      default: // moving window
        xw->x = new_x;
        xw->y = new_y;
        xwin_set_geom(xw);
        break;
      case 27: // r (resizing window)
        if (new_wi < XWIN_MIN_WIDTH_HEIGHT) {
          new_wi = XWIN_MIN_WIDTH_HEIGHT;
        }
        if (new_hi < XWIN_MIN_WIDTH_HEIGHT) {
          new_hi = XWIN_MIN_WIDTH_HEIGHT;
        }
        xw->wi = new_wi;
        xw->hi = new_hi;
        xwin_set_geom(xw);
        break;
      }
      break;
    case ButtonRelease:
      is_dragging = False;
      dragging_button = 0;
      xw = xwin_get_by_window(ev.xbutton.window);
      xw->desk = dsk;
      XUngrabPointer(dpy, CurrentTime);
      break;
    }
  }
  //? clean-up cursor
  return 0;
}
