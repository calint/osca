#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// maximum number of windows
#define WIN_MAX_COUNT 128

// pixels to show of a window folded to the right
#define WIN_SLIP_DX 13

// random pixels relative to WIN_SLIP_DX
#define WIN_SLIP 7

// window border width
#define WIN_BORDER_WIDTH 1

// pixels to 'bump' a window
#define WIN_BUMP 59

#define WIN_BORDER_ACTIVE_COLOR 0x00008000
#define WIN_BORDER_INACTIVE_COLOR 0x00000000

// xwin.bits
#define XWIN_BIT_FULL_HEIGHT 1
#define XWIN_BIT_FULL_WIDTH 2
#define XWIN_BIT_ALLOCATED 4
#define XWIN_BITS_FULL_SCREEN 3

typedef struct xwin {
  Window win;  // x11 window handle
  int x;       // position x
  int y;       // position y
  unsigned wi; // width
  unsigned hi; // height
  int x_pf;    // x pre full width / height / screen
  int y_pf;    // y pre full width / height / screen
  int wi_pf;   // wi pre full width / height / screen
  int hi_pf;   // hi pre full width / height / screen
  int desk;    // desk the window is on
  int desk_x;  // x coord of window before folded at desk switch
  char bits;   // bit 1: fullheight  bit 2: fullwidth  bit 3: allocated
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
static struct scr {
  int id, wi, hi;
} scr;

// current key pressed
static unsigned key_pressed;

// current focused window
static xwin *win_focused;

// dragging state
static char is_dragging;
static int dragging_start_x;
static int dragging_start_y;
static int dragging_button;

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

static xwin *xwin_get(Window w) {
  int first_avail = -1;
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (wins[i].bits & XWIN_BIT_ALLOCATED) {
      if (wins[i].win == w) {
        return &wins[i];
      }
    } else {
      if (first_avail == -1) {
        first_avail = i;
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
  }
  XSetInputFocus(dpy, this->win, RevertToParent, CurrentTime);
  XSetWindowBorder(dpy, this->win, WIN_BORDER_ACTIVE_COLOR);
  win_focused = this;
}
static void xwin_raise(xwin *this) { XRaiseWindow(dpy, this->win); }
static xwin *_win_find(Window w) {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (wins[i].win == w)
      return &wins[i];
  }
  return NULL;
}
static void xwin_free(Window w) {
  xwin *win = _win_find(w);
  if (!win) {
    return;
  }
  if (win->bits & XWIN_BIT_ALLOCATED) {
    win->bits = 0; // mark free
    xwin_count--;
    // fprintf(flog, "windows allocated: %d\n", wincount);
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
  this->x = (scr.wi - this->wi) >> 1;
  this->y = (scr.hi - this->hi) >> 1;
  xwin_set_geom(this);
}
static void xwin_wider(xwin *this) {
  xwin_read_geom(this);
  unsigned wi_prv = this->wi;
  this->wi = ((this->wi << 2) + this->wi) >> 2;
  this->x = this->x - ((this->wi - wi_prv) >> 1);
  xwin_set_geom(this);
}
static void xwin_thinner(xwin *this) {
  xwin_read_geom(this);
  unsigned wi_prv = this->wi;
  this->wi = ((this->wi << 1) + this->wi) >> 2;
  this->x = this->x - ((this->wi - wi_prv) >> 1);
  xwin_set_geom(this);
}
static void xwin_close(xwin *this) {
  XEvent ke;
  ke.type = ClientMessage;
  ke.xclient.window = this->win;
  ke.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
  ke.xclient.format = 32;
  ke.xclient.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
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
    this->wi = scr.wi;
    this->hi = scr.hi;
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
    this->hi = scr.hi;
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
    this->wi = scr.wi;
    xwin_set_geom(this);
  }
  this->bits ^= XWIN_BIT_FULL_WIDTH;
}
static void xwin_hide(xwin *this) {
  xwin_read_geom(this);
  this->desk_x = this->x;
  int slip = rand() % WIN_SLIP;
  this->x = (scr.wi - WIN_SLIP_DX + slip);
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
static int _xwin_ix(xwin *this) {
  if (this == NULL) {
    return -1;
  }
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (this == &wins[i]) {
      return i;
    }
  }
  return -1;
}
static char _focus_try(unsigned ix) {
  xwin *w = &wins[ix];
  if ((w->bits & XWIN_BIT_ALLOCATED) && (w->desk == dsk)) {
    xwin_raise(w);
    xwin_focus(w);
    return True;
  }
  return False;
}
static void desk_show(int dsk, int dsk_prv) {
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
    if (xw->desk == dsk) {
      xwin_show(xw);
    }
  }
}
static void focus_first_win_on_desk() {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    xwin *w = &wins[i];
    if ((w->bits & XWIN_BIT_ALLOCATED) && (w->desk == dsk)) {
      xwin_raise(w);
      xwin_focus(w);
      return;
    }
  }
  win_focused = NULL;
}
static void focus_next_win() {
  int i0 = _xwin_ix(win_focused);
  int i = i0;
  while (++i < WIN_MAX_COUNT) {
    if (_focus_try(i)) {
      return;
    }
  }
  i = 0;
  while (i <= i0) {
    if (_focus_try(i)) {
      return;
    }
    i++;
  }
  focus_first_win_on_desk();
}
static void focus_prev_win() {
  int i0 = _xwin_ix(win_focused);
  int i = i0;
  while (--i >= 0) {
    if (_focus_try(i)) {
      return;
    }
  }
  i = WIN_MAX_COUNT;
  while (--i >= 0) {
    if (_focus_try(i)) {
      return;
    }
  }
  focus_first_win_on_desk();
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

  scr.id = DefaultScreen(dpy);
  scr.wi = DisplayWidth(dpy, scr.id);
  scr.hi = DisplayHeight(dpy, scr.id);

  root = DefaultRootWindow(dpy);
  Cursor root_window_cursor = XCreateFontCursor(dpy, XC_arrow);
  XDefineCursor(dpy, root, root_window_cursor);
  XGrabKey(dpy, AnyKey, Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, AnyKey, Mod4Mask + ShiftMask, root, True, GrabModeAsync,
           GrabModeAsync);
  // XGrabKey(dpy, 122, 0, root, True, GrabModeAsync, GrabModeAsync); // volume
  // down XGrabKey(dpy, 123, 0, root, True, GrabModeAsync, GrabModeAsync); //
  // volume sup XGrabKey(dpy, 107, 0, root, True, GrabModeAsync, GrabModeAsync);
  // // print
  XSelectInput(dpy, root, SubstructureNotifyMask);

  // previous desk
  int dsk_prv = 0;
  // temporary
  xwin *xw = NULL;
  XEvent ev;
  while (!XNextEvent(dpy, &ev)) {
    // fprintf(flog, "event: %s   win=%p\n", ix_evnames[ev.type],
    //         (void *)ev.xany.window);
    switch (ev.type) {
    default:
      // fprintf(flog, "  unhandled\n");
      break;
    case MapNotify:
      if (ev.xmap.window == root || ev.xmap.window == 0 ||
          ev.xmap.override_redirect) {
        break;
      }
      xw = xwin_get(ev.xmap.window);
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
      xwin_free(ev.xmap.window);
      break;
    case EnterNotify:
      if (is_dragging)
        break;
      xw = xwin_get(ev.xcrossing.window);
      xwin_focus(xw);
      break;
    case KeyPress:
      key_pressed = ev.xkey.keycode;
      if (ev.xkey.subwindow) {
        xw = xwin_get(ev.xkey.subwindow);
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
        system("xii-qbin");
        break;
      case 31: // i
        system("xii-browser");
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
        }
        break;
      case 56: // b
        if (win_focused) {
          xwin_bump(win_focused, WIN_BUMP);
        }
        break;
      case 12: // 3
        if (win_focused) {
          xwin_toggle_fullscreen(win_focused);
        }
        break;
      case 13: // 4
        if (win_focused) {
          xwin_toggle_fullheight(win_focused);
        }
        break;
      case 14: // 5
        if (win_focused) {
          xwin_toggle_fullwidth(win_focused);
        }
        break;
      case 15: // 6
        if (win_focused) {
          xwin_bump(win_focused, 200);
        }
        break;
      case 113: // left
        focus_prev_win();
        break;
      case 52:  // z
      case 114: // right
        focus_next_win();
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
            xwin_read_geom(win_focused);
            win_focused->desk_x = win_focused->x;
            xwin_raise(win_focused);
          }
        }
        desk_show(dsk, dsk_prv);
        break;
      case 40:  // d
      case 116: // down
        dsk_prv = dsk;
        dsk--;
        if (ev.xkey.state & ShiftMask) {
          if (win_focused) {
            win_focused->desk = dsk;
            xwin_read_geom(win_focused);
            win_focused->desk_x = win_focused->x;
            xwin_raise(win_focused);
          }
        }
        desk_show(dsk, dsk_prv);
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
      xw = xwin_get(ev.xbutton.window);
      xwin_focus(xw);
      XGrabPointer(dpy, xw->win, True, PointerMotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
      xwin_raise(xw);
      xwin_read_geom(xw);
      dragging_start_x = ev.xbutton.x_root;
      dragging_start_y = ev.xbutton.y_root;
      dragging_button = ev.xbutton.button;
      break;
    case MotionNotify:
      while (XCheckTypedEvent(dpy, MotionNotify, &ev))
        ;
      int xdiff = ev.xbutton.x_root - dragging_start_x;
      int ydiff = ev.xbutton.y_root - dragging_start_y;
      dragging_start_x = ev.xbutton.x_root;
      dragging_start_y = ev.xbutton.y_root;
      int new_x = xw->x + xdiff;
      int new_wi = xw->wi + xdiff;
      int new_y = xw->y + ydiff;
      int new_hi = xw->hi + ydiff;
      if (xw->bits & XWIN_BIT_FULL_WIDTH) {
        new_x = -WIN_BORDER_WIDTH;
        new_wi = scr.wi;
      }
      if (xw->bits & XWIN_BIT_FULL_HEIGHT) {
        new_y = -WIN_BORDER_WIDTH;
        new_hi = scr.hi;
      }
      if (dragging_button == 3) {
        if (new_wi < 1) {
          new_wi = 1;
        }
        if (new_hi < 1) {
          new_hi = 1;
        }
        xw->wi = new_wi;
        xw->hi = new_hi;
        xwin_set_geom(xw);
        break;
      }
      switch (key_pressed) {
      default:
        xw->x = new_x;
        xw->y = new_y;
        xwin_set_geom(xw);
        break;
      case 27: // r
        if (new_wi < 1) {
          new_wi = 1;
        }
        if (new_hi < 1) {
          new_hi = 1;
        }
        xw->wi = new_wi;
        xw->hi = new_hi;
        xwin_set_geom(xw);
        break;
      }
      break;
    case ButtonRelease:
      is_dragging = False;
      xw = xwin_get(ev.xbutton.window);
      xw->desk = dsk;
      XUngrabPointer(dpy, CurrentTime);
      break;
    }
  }
  //? clean-up cursor
  return 0;
}