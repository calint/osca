// todo: make windows count dynamic
// todo: when window dimensions equal screen dimensions consider window
// maximized
// todo: consolidate xwin.vh and bits into one field

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
static char *ix_evnames[LASTEvent] = {
    "unknown",          "unknown",       // 0
    "KeyPress",         "KeyRelease",    // 2
    "ButtonPress",      "ButtonRelease", // 4
    "MotionNotify",                      // 6
    "EnterNotify",      "LeaveNotify",   // 7 LeaveWindowMask LeaveWindowMask
    "FocusIn",          "FocusOut",      // 9 from XSetFocus
    "KeymapNotify",                      // 11
    "Expose",           "GraphicsExpose",   "NoExpose", // 12
    "VisibilityNotify", "CreateNotify",     "DestroyNotify",
    "UnmapNotify",      "MapNotify", // 15
    "MapRequest",       "ReparentNotify",   "ConfigureNotify",
    "ConfigureRequest", "GravityNotify",    "ResizeRequest",
    "CirculateNotify",  "CirculateRequest", "PropertyNotify",
    "SelectionClear",   "SelectionRequest", "SelectionNotify",
    "ColormapNotify",   "ClientMessage",    "MappingNotify",
    "GenericEvent"};

static xwin *xwinget(Window w) {
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
    fflush(flog);
    sleep(5);
    return xwinget(w);
  }
  xw = &wins[firstavail];
  xw->bits = XWIN_BIT_ALLOCATED;
  wincount++;
  fprintf(flog, "windows allocated: %d\n", wincount);
  xw->win = w;
  xw->desk = dsk;
  XSetWindowBorderWidth(dpy, w, WIN_BORDER_WIDTH);
  return xw;
}
static void xwinfocus(xwin *this) {
  if (winfocused) {
    XSetWindowBorder(dpy, winfocused->win, WIN_BORDER_INACTIVE_COLOR);
  }
  XSetInputFocus(dpy, this->win, RevertToParent, CurrentTime);
  XSetWindowBorder(dpy, this->win, WIN_BORDER_ACTIVE_COLOR);
  winfocused = this;
}
static void xwinraise(xwin *this) { XRaiseWindow(dpy, this->win); }
static void focusfirstondesk() {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    xwin *w = &wins[i];
    if ((w->bits & XWIN_BIT_ALLOCATED) && (w->desk == dsk)) {
      xwinraise(w);
      xwinfocus(w);
      return;
    }
  }
  winfocused = NULL;
}
static xwin *_winfind(Window w) {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (wins[i].win == w)
      return &wins[i];
  }
  return NULL;
}
static void xwinfree(Window w) {
  xwin *xw = _winfind(w);
  if (!xw) {
    return;
  }
  if (xw->bits & XWIN_BIT_ALLOCATED) {
    xw->bits = 0; // mark free
    wincount--;
    fprintf(flog, "windows allocated: %d\n", wincount);
  }
  if (winfocused == xw) {
    winfocused = NULL;
  }
}
static void xwingeom(xwin *this) {
  Window wsink;
  unsigned dummy;
  XGetGeometry(dpy, this->win, (Window *)&wsink, &this->x, &this->y, &this->wi,
               &this->hi, &dummy, &dummy);
}
static void xwingeomset(xwin *this, int x, int y, int w, int h) {
  XMoveResizeWindow(dpy, this->win, x, y, w, h);
}
static void xwingeomset2(xwin *this) {
  XMoveResizeWindow(dpy, this->win, this->x, this->y, this->wi, this->hi);
}
static void xwingeomcenter(xwin *this) {
  xwingeom(this);
  int nx = (scr.wi - this->wi) >> 1;
  int ny = (scr.hi - this->hi) >> 1;
  xwingeomset(this, nx, ny, this->wi, this->hi);
}
static void xwingeomwider(xwin *this) {
  xwingeom(this);
  int nw = ((this->wi << 2) + this->wi) >> 2;
  int nx = this->x - ((nw - this->wi) >> 1);
  xwingeomset(this, nx, this->y, nw, this->hi);
}
static void xwingeomthinner(xwin *this) {
  xwingeom(this);
  int nw = ((this->wi << 1) + this->wi) >> 2;
  int nx = this->x - ((nw - this->wi) >> 1);
  xwingeomset(this, nx, this->y, nw, this->hi);
}
static void xwinclose(xwin *this) {
  XEvent ke;
  ke.type = ClientMessage;
  ke.xclient.window = this->win;
  ke.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
  ke.xclient.format = 32;
  ke.xclient.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
  ke.xclient.data.l[1] = CurrentTime;
  XSendEvent(dpy, this->win, False, NoEventMask, &ke);
}
static void xwintogglefullscreen(xwin *this) {
  if ((this->bits & (XWIN_BITS_FULL_SCREEN)) == (XWIN_BITS_FULL_SCREEN)) {
    // toggle from fullscreen
    xwingeomset(this, this->x, this->y, this->wi, this->hi);
    this->bits &= ~XWIN_BITS_FULL_SCREEN;
  } else {
    // toggle to fullscreen
    xwingeom(this);
    xwingeomset(this, -WIN_BORDER_WIDTH, -WIN_BORDER_WIDTH, scr.wi, scr.hi);
    this->bits |= XWIN_BITS_FULL_SCREEN;
  }
}
static void xwintogglefullheight(xwin *this) {
  if (this->bits & XWIN_BIT_FULL_HEIGHT) {
    int y = this->y;
    int hi = this->hi;
    xwingeom(this);
    xwingeomset(this, this->x, y, this->wi, hi);
  } else {
    xwingeom(this);
    xwingeomset(this, this->x, -WIN_BORDER_WIDTH, this->wi, scr.hi);
  }
  this->bits ^= XWIN_BIT_FULL_HEIGHT;
}
static void xwintogglefullwidth(xwin *this) {
  if (this->bits & XWIN_BIT_FULL_WIDTH) {
    int x = this->x;
    int wi = this->wi;
    xwingeom(this);
    xwingeomset(this, x, this->y, wi, this->hi);
  } else {
    xwingeom(this);
    xwingeomset(this, -WIN_BORDER_WIDTH, this->y, scr.wi, this->hi);
  }
  this->bits ^= XWIN_BIT_FULL_WIDTH;
}
static void xwinhide(xwin *this) {
  xwingeom(this);
  this->desk_x = this->x;
  int slip = rand() % WIN_SLIP;
  this->x = (scr.wi - WIN_SLIP_DX + slip);
  xwingeomset2(this);
}
static void xwinshow(xwin *this) {
  this->x = this->desk_x;
  xwingeomset2(this);
}
static void xwinbump(xwin *this, int r) {
  xwingeom(this);
  this->x += (rand() % r) - (r >> 1);
  this->y += (rand() % r) - (r >> 1);
  xwingeomset2(this);
}
static int _xwinix(xwin *this) {
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
static int _focustry(int k) {
  xwin *w = &wins[k];
  if ((w->bits & XWIN_BIT_ALLOCATED) && (w->desk == dsk)) {
    xwinraise(w);
    xwinfocus(w);
    return 1;
  }
  return 0;
}
static void focusnext() {
  int i0 = _xwinix(winfocused);
  int i = i0;
  while (++i < WIN_MAX_COUNT) {
    if (_focustry(i)) {
      return;
    }
  }
  i = 0;
  while (i <= i0) {
    if (_focustry(i)) {
      return;
    }
    i++;
  }
  focusfirstondesk();
}
static void focusprev() {
  int i0 = _xwinix(winfocused);
  int i = i0;
  while (--i >= 0) {
    if (_focustry(i)) {
      return;
    }
  }
  i = WIN_MAX_COUNT;
  while (--i >= 0) {
    if (_focustry(i)) {
      return;
    }
  }
  focusfirstondesk();
}
static void togglefullscreen() {
  if (!winfocused) {
    return;
  }
  xwintogglefullscreen(winfocused);
}
static void togglefullheight() {
  if (!winfocused) {
    return;
  }
  xwintogglefullheight(winfocused);
}
static void togglefullwidth() {
  if (!winfocused) {
    return;
  }
  xwintogglefullwidth(winfocused);
}
static void deskshow(int dsk, int dskprv) {
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
      xwinhide(xw);
    }
    if (xw->desk == dsk) {
      xwinshow(xw);
    }
  }
}
static int errorhandler(Display *d, XErrorEvent *e) {
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
  XSetErrorHandler(errorhandler);
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
  XButtonEvent buttonevstart;
  memset(&buttonevstart, 0, sizeof(buttonevstart));

  XEvent ev;
  while (!XNextEvent(dpy, &ev)) {
    fprintf(flog, "event: %s   win=%p\n", ix_evnames[ev.type],
            (void *)ev.xany.window);
    switch (ev.type) {
    default:
      fprintf(flog, "  unhandled\n");
      fflush(flog);
      break;
      //		case ClientMessage:break;
      //		case ReparentNotify:break;
      //		case CreateNotify:break;
      //		case DestroyNotify:break;
      //		case ConfigureNotify:break;
      //		case MapRequest:break;
    case MapNotify:
      if (ev.xmap.window == root || ev.xmap.window == 0 ||
          ev.xmap.override_redirect) {
        break;
      }
      xw = xwinget(ev.xmap.window);
      xwingeomcenter(xw);
      xwinfocus(xw);
      XGrabButton(dpy, AnyButton, Mod4Mask, xw->win, True, ButtonPressMask,
                  GrabModeAsync, GrabModeAsync, None, None);
      XSelectInput(dpy, xw->win, EnterWindowMask);
      break;
    case UnmapNotify:
      if (ev.xmap.window == root || ev.xmap.window == 0 ||
          ev.xmap.override_redirect) {
        break;
      }
      xwinfree(ev.xmap.window);
      break;
    case EnterNotify:
      if (dragging)
        break;
      xw = xwinget(ev.xcrossing.window);
      xwinfocus(xw);
      break;
    case KeyPress:
      key = ev.xkey.keycode;
      if (ev.xkey.subwindow) {
        xw = xwinget(ev.xkey.subwindow);
      }
      switch (key) {
      case 53: // x
        system("xii-sticky");
        break;
      case 54: // c
        system("xii-term");
        break;
        //			case 107://sysrq prntscr
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
          xwinclose(winfocused);
        }
        break;
      case 39: // s
        if (winfocused) {
          xwingeomcenter(winfocused);
        }
        break;
      case 25: // w
        if (winfocused) {
          if (ev.xkey.state & ShiftMask) {
            xwingeomthinner(winfocused);
          } else {
            xwingeomwider(winfocused);
          }
        }
        break;
      case 56: // b
        if (winfocused) {
          xwinbump(winfocused, WIN_BUMP);
        }
        break;
      case 12: // 3
        togglefullscreen();
        break;
      case 13: // 4
        togglefullheight();
        break;
      case 14: // 5
        togglefullwidth();
        break;
      case 15: // 6
        xwinbump(winfocused, 200);
        break;
      case 16: // 7
        system("xii-ide");
        break;
        //			case 27://r
      case 113: // left
        focusprev();
        break;
      case 52:  // z
      case 114: // right
        focusnext();
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
            xwingeom(winfocused);
            winfocused->desk_x = winfocused->x;
            xwinraise(winfocused);
          }
        }
        deskshow(dsk, dskprv);
        break;
      case 40:  // d
      case 116: // down
        dskprv = dsk;
        dsk--;
        if (ev.xkey.state & ShiftMask) {
          if (winfocused) {
            winfocused->desk = dsk;
            xwingeom(winfocused);
            winfocused->desk_x = winfocused->x;
            xwinraise(winfocused);
          }
        }
        deskshow(dsk, dskprv);
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
      xw = xwinget(ev.xbutton.window);
      xwinfocus(xw);
      XGrabPointer(dpy, xw->win, True, PointerMotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
      xwinraise(xw);
      xwingeom(xw);
      buttonevstart = ev.xbutton;
      break;
    case MotionNotify:
      while (XCheckTypedEvent(dpy, MotionNotify, &ev))
        ;
      int xdiff = ev.xbutton.x_root - buttonevstart.x_root;
      int ydiff = ev.xbutton.y_root - buttonevstart.y_root;
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
      if (buttonevstart.button == 3) {
        if (nw < 0) {
          nw = 0;
        }
        if (nh < 0) {
          nh = 0;
        }
        xwingeomset(xw, xw->x, xw->y, nw, nh);
        break;
      }
      switch (key) {
      default:
        xwingeomset(xw, nx, ny, xw->wi, xw->hi);
        break;
      case 27: // r
        if (nw < 0) {
          nw = 0;
        }
        if (nh < 0) {
          nh = 0;
        }
        xwingeomset(xw, xw->x, xw->y, nw, nh);
        break;
      }
      break;
    case ButtonRelease:
      dragging = False;
      xw = xwinget(ev.xbutton.window);
      xw->desk = dsk;
      XUngrabPointer(dpy, CurrentTime);
      break;
    }
  }
  return 0;
}