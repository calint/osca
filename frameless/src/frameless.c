#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APP_NAME "window manager frameless"
#define WIN_MAX_COUNT 128  // maximum number of windows
#define WIN_SLIP_DX 13     // pixels to show of a window folded to the right
#define WIN_SLIP 7         // random pixels relative to WIN_SLIP_DX
#define WIN_BORDER_WIDTH 1 // window border width
#define WIN_BUMP 59        // pixels to 'bump' a window
#define WIN_BORDER_ACTIVE_COLOR 0x00008000
#define WIN_BORDER_INACTIVE_COLOR 0x00000000
#define XWIN_BIT_FULL_HEIGHT 1
#define XWIN_BIT_FULL_WIDTH 2
#define XWIN_BIT_ALLOCATED 4
#define XWIN_BITS_FULL_SCREEN 3

typedef int xdesk;
typedef char bool;
typedef struct xwin {
  Window win;  // x11 window handle
  int x;       // position x
  int y;       // position y
  unsigned wi; // width
  unsigned hi; // height
  int x_pf;    // x pre full width / full screen
  int y_pf;    // y pre full width / full screen
  int wi_pf;   // wi pre full width / full screen
  int hi_pf;   // hi pre full width / full screen
  xdesk desk;  // desk the window is on
  int desk_x;  // x coord of window before folded at desk switch
  char bits;   // bit 1: fullheight  bit 2: fullwidth  bit 3: allocated
} xwin;
static xwin wins[WIN_MAX_COUNT];
static FILE *flog;
static Display *dpy;
static Window root;
static xdesk dsk;
static unsigned wincount;
static struct scr {
  int id, wi, hi;
} scr;
static unsigned key;
static xwin *winfocused;
static bool dragging;
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
  xwin *xw = NULL;
  int firstavail = -1;
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (wins[i].bits & XWIN_BIT_ALLOCATED) {
      if (wins[i].win == w) {
        return &wins[i];
      }
    } else {
      if (firstavail == -1) {
        firstavail = i;
      }
    }
  }
  if (firstavail == -1) {
    fprintf(flog, "!!! no free windows\n");
    exit(-1);
  }
  xw = &wins[firstavail];
  xw->bits = XWIN_BIT_ALLOCATED;
  wincount++;
  // fprintf(flog, "windows allocated: %d\n", wincount);
  xw->win = w;
  xw->desk = dsk;
  XSetWindowBorderWidth(dpy, w, WIN_BORDER_WIDTH);
  return xw;
}
static void xwin_focus(xwin *this) {
  if (winfocused) {
    XSetWindowBorder(dpy, winfocused->win, WIN_BORDER_INACTIVE_COLOR);
  }
  XSetInputFocus(dpy, this->win, RevertToParent, CurrentTime);
  XSetWindowBorder(dpy, this->win, WIN_BORDER_ACTIVE_COLOR);
  winfocused = this;
}
static void xwin_raise(xwin *this) { XRaiseWindow(dpy, this->win); }
static void xwin_focus_first_on_desk() {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    xwin *w = &wins[i];
    if ((w->bits & XWIN_BIT_ALLOCATED) && (w->desk == dsk)) {
      xwin_raise(w);
      xwin_focus(w);
      return;
    }
  }
  winfocused = NULL;
}
static xwin *_win_find(Window w) {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (wins[i].win == w)
      return &wins[i];
  }
  return NULL;
}
static void xwin_free(Window w) {
  xwin *xw = _win_find(w);
  if (!xw) {
    return;
  }
  if (xw->bits & XWIN_BIT_ALLOCATED) {
    xw->bits = 0; // mark free
    wincount--;
    // fprintf(flog, "windows allocated: %d\n", wincount);
  }
  if (winfocused == xw) {
    winfocused = NULL;
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
static void xwin_bump(xwin *this, int r) {
  xwin_read_geom(this);
  this->x += (rand() % r) - (r >> 1);
  this->y += (rand() % r) - (r >> 1);
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
static int _focus_try(unsigned ix) {
  xwin *w = &wins[ix];
  if ((w->bits & XWIN_BIT_ALLOCATED) && (w->desk == dsk)) {
    xwin_raise(w);
    xwin_focus(w);
    return 1;
  }
  return 0;
}
static void xwin_focus_next() {
  int i0 = _xwin_ix(winfocused);
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
  xwin_focus_first_on_desk();
}
static void xwin_focus_prev() {
  int i0 = _xwin_ix(winfocused);
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
  xwin_focus_first_on_desk();
}
static void toggle_fullscreen() {
  if (!winfocused) {
    return;
  }
  xwin_toggle_fullscreen(winfocused);
}
static void toggle_fullheight() {
  if (!winfocused) {
    return;
  }
  xwin_toggle_fullheight(winfocused);
}
static void toggle_fullwidth() {
  if (!winfocused) {
    return;
  }
  xwin_toggle_fullwidth(winfocused);
}
static void desk_show(int dsk, int dskprv) {
  int n;
  for (n = 0; n < WIN_MAX_COUNT; n++) {
    xwin *xw = &wins[n];
    if (!(xw->bits & XWIN_BIT_ALLOCATED)) {
      continue;
    }
    if (xw->win == 0) {
      continue;
    }
    if (xw->win == root) {
      continue;
    }
    if (xw->desk == dskprv) {
      xwin_hide(xw);
    }
    if (xw->desk == dsk) {
      xwin_show(xw);
    }
  }
}
static int error_handler(Display *d, XErrorEvent *e) {
  char buffer_return[1024] = "";
  XGetErrorText(d, e->error_code, buffer_return, sizeof(buffer_return));
  fprintf(flog, "!!! x11 error\n");
  fprintf(flog, "!!!       text: %s\n", buffer_return);
  fprintf(flog, "!!!       type: %d\n", e->type);
  fprintf(flog, "!!! resourceid: %p\n", (void *)e->resourceid);
  fprintf(flog, "!!! error code: %d\n", (unsigned int)e->error_code);
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
  puts(APP_NAME);
  srand(0);
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
  XGrabKey(dpy, 122, 0, root, True, GrabModeAsync, GrabModeAsync); // voldown
  XGrabKey(dpy, 123, 0, root, True, GrabModeAsync, GrabModeAsync); // volup
  XGrabKey(dpy, 107, 0, root, True, GrabModeAsync, GrabModeAsync); // print
  XSelectInput(dpy, root, SubstructureNotifyMask);

  int dskprv = 0;
  xwin *xw = NULL;
  XButtonEvent button_start;
  memset(&button_start, 0, sizeof(button_start));

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
      if (dragging)
        break;
      xw = xwin_get(ev.xcrossing.window);
      xwin_focus(xw);
      break;
    case KeyPress:
      key = ev.xkey.keycode;
      if (ev.xkey.subwindow) {
        xw = xwin_get(ev.xkey.subwindow);
      }
      switch (key) {
      case 53: // x
        system("xii-sticky");
        break;
      case 54: // c
        system("xii-term");
        break;
      case 33: // p
        system("xii-scrsht");
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
      case 9:  // esc
      case 49: // §
        if (winfocused) {
          xwin_close(winfocused);
        }
        break;
      case 39: // s
        if (winfocused) {
          xwin_center(winfocused);
        }
        break;
      case 25: // w
        if (winfocused) {
          if (ev.xkey.state & ShiftMask) {
            xwin_thinner(winfocused);
          } else {
            xwin_wider(winfocused);
          }
        }
        break;
      case 56: // b
        if (winfocused) {
          xwin_bump(winfocused, WIN_BUMP);
        }
        break;
      case 12: // 3
        toggle_fullscreen();
        break;
      case 13: // 4
        toggle_fullheight();
        break;
      case 14: // 5
        toggle_fullwidth();
        break;
      case 15: // 6
        xwin_bump(winfocused, 200);
        break;
      case 16: // 7
        system("xii-ide");
        break;
        //			case 27://r
      case 113: // left
        xwin_focus_prev();
        break;
      case 52:  // z
      case 114: // right
        xwin_focus_next();
        break;
      case 119: // del
        XKillClient(dpy, ev.xkey.subwindow);
        break;
      case 72: // toggle mute
        system("xii-vol-toggle");
        break;
      case 68: // F2   screen brightness down
        system("xii-decrease-screen-brightness");
        break;
      case 69: // F3   screen brightness up
        system("xii-increase-screen-brightness");
        break;
      case 73: // F8   volume down
        system("xii-vol-down");
        break;
      case 74: // F9   volume up
        system("xii-vol-up");
        break;
      case 38:  // a
      case 111: // up
        dskprv = dsk;
        dsk++;
        if (ev.xkey.state & ShiftMask) {
          if (winfocused) {
            winfocused->desk = dsk;
            xwin_read_geom(winfocused);
            winfocused->desk_x = winfocused->x;
            xwin_raise(winfocused);
          }
        }
        desk_show(dsk, dskprv);
        break;
      case 40:  // d
      case 116: // down
        dskprv = dsk;
        dsk--;
        if (ev.xkey.state & ShiftMask) {
          if (winfocused) {
            winfocused->desk = dsk;
            xwin_read_geom(winfocused);
            winfocused->desk_x = winfocused->x;
            xwin_raise(winfocused);
          }
        }
        desk_show(dsk, dskprv);
        break;
      }
      break;
    case KeyRelease:
      if (key == ev.xkey.keycode) {
        key = 0;
      }
      break;
    case ButtonPress:
      dragging = True;
      xw = xwin_get(ev.xbutton.window);
      xwin_focus(xw);
      XGrabPointer(dpy, xw->win, True, PointerMotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
      xwin_raise(xw);
      xwin_read_geom(xw);
      button_start = ev.xbutton;
      break;
    case MotionNotify:
      while (XCheckTypedEvent(dpy, MotionNotify, &ev))
        ;
      int xdiff = ev.xbutton.x_root - button_start.x_root;
      int ydiff = ev.xbutton.y_root - button_start.y_root;
      button_start = ev.xbutton;
      int nx = xw->x + xdiff;
      int nw = xw->wi + xdiff;
      int ny = xw->y + ydiff;
      int nh = xw->hi + ydiff;
      if (xw->bits & XWIN_BIT_FULL_WIDTH) {
        nx = -WIN_BORDER_WIDTH;
        nw = scr.wi;
      }
      if (xw->bits & XWIN_BIT_FULL_HEIGHT) {
        ny = -WIN_BORDER_WIDTH;
        nh = scr.hi;
      }
      if (button_start.button == 3) {
        if (nw < 0) {
          nw = 0;
        }
        if (nh < 0) {
          nh = 0;
        }
        xw->wi = nw;
        xw->hi = nh;
        xwin_set_geom(xw);
        break;
      }
      switch (key) {
      default:
        xw->x = nx;
        xw->y = ny;
        xwin_set_geom(xw);
        break;
      case 27: // r
        if (nw < 0) {
          nw = 0;
        }
        if (nh < 0) {
          nh = 0;
        }
        xw->wi = nw;
        xw->hi = nh;
        xwin_set_geom(xw);
        break;
      }
      break;
    case ButtonRelease:
      dragging = False;
      xw = xwin_get(ev.xbutton.window);
      xw->desk = dsk;
      XUngrabPointer(dpy, CurrentTime);
      break;
    }
  }
  return 0;
}