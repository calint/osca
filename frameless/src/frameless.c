// todo: make windows count dynamic
// todo: when window dimensions equal screen dimensions consider window
// maximized
// todo: consolidate xwin.vh and bits into one field

#define APP "window manager frameless"
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WIN_MAX_COUNT 128  // maximum number of windows
#define WIN_SLIP 7         // pixels to show of a window folded to the right
#define WIN_BORDER_WIDTH 1 // window border width
#define WIN_BUMP 59        // pixels to 'bump' a window
#define WIN_BORDER_ACTIVE_COLOR 0x00008000
#define WIN_BORDER_INACTIVE_COLOR 0x00000000

typedef int xdesk;
typedef char bits;
typedef char bool;
typedef struct xwin {
  Window w;           // x11 window handle
  int gx, gy;         // position
  unsigned gw, gh;    // width height
  bits vh;            // bit 1 means fullheight    bit 2 means fullwidth
  xdesk desk;         // desk the window is on
  int desk_x;         // x coord of window before folded at desk switch
  unsigned char bits; // bit 1 means allocated
} xwin;
static xwin wins[WIN_MAX_COUNT];
static FILE *flog;
static Display *dpy;
static Window root;
static xdesk dsk = 0;
static int wincount = 0;
static struct scr {
  int id, wi, hi;
} scr;
static unsigned key = 0;
static xwin *winfocused = NULL;
static bool dragging = False;
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
    if (wins[i].bits & 1) { // check if allocated
      if (wins[i].w == w) { // same handle?
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
  xw->bits |= 1; // allocated
  wincount++;
  xw->w = w;
  xw->vh = 0;
  xw->desk = dsk;
  XSetWindowBorderWidth(dpy, w, WIN_BORDER_WIDTH);
  return xw;
}
static void xwinfocus(xwin *this) {
  if (winfocused) {
    XSetWindowBorder(dpy, winfocused->w, WIN_BORDER_INACTIVE_COLOR);
  }
  XSetInputFocus(dpy, this->w, RevertToParent, CurrentTime);
  XSetWindowBorder(dpy, this->w, WIN_BORDER_ACTIVE_COLOR);
  winfocused = this;
}
static void xwinraise(xwin *this) { XRaiseWindow(dpy, this->w); }
static void focusfirstondesk() {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    xwin *w = &wins[i];
    if ((w->bits & 1) && (w->desk == dsk)) {
      xwinraise(w);
      xwinfocus(w);
      return;
    }
  }
  winfocused = NULL;
}
static xwin *_winfind(Window w) {
  for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
    if (wins[i].w == w)
      return &wins[i];
  }
  return NULL;
}
static void xwinfree(Window w) {
  xwin *xw = _winfind(w);
  if (!xw) {
    return;
  }
  if (xw->bits & 1) { // if allocateds
    xw->bits &= 0xfe; // mark free
    wincount--;
  }
  if (winfocused == xw) {
    winfocused = NULL;
  }
}
static void xwingeom(xwin *this) {
  Window wsink;
  unsigned int dummy;
  XGetGeometry(dpy, this->w, (Window *)&wsink, &this->gx, &this->gy, &this->gw,
               &this->gh, &dummy, &dummy);
}
static void xwingeomset(xwin *this, int x, int y, int w, int h) {
  XMoveResizeWindow(dpy, this->w, x, y, w, h);
}
static void xwingeomset2(xwin *this) {
  XMoveResizeWindow(dpy, this->w, this->gx, this->gy, this->gw, this->gh);
}
static void xwingeomcenter(xwin *this) {
  xwingeom(this);
  int nx = (scr.wi - this->gw) >> 1; //? magicnum
  int ny = (scr.hi - this->gh) >> 1; //? magicnum
  xwingeomset(this, nx, ny, this->gw, this->gh);
}
static void xwingeomwider(xwin *this) {
  xwingeom(this);
  int nw = ((this->gw << 2) + this->gw) >> 2; //? magicnum
  int nx = this->gx - ((nw - this->gw) >> 1); //? magicnum
  xwingeomset(this, nx, this->gy, nw, this->gh);
}
static void xwingeomthinner(xwin *this) {
  xwingeom(this);
  int nw = ((this->gw << 1) + this->gw) >> 2; //? magicnum
  int nx = this->gx - ((nw - this->gw) >> 1); //? magicnum
  xwingeomset(this, nx, this->gy, nw, this->gh);
}
static void xwinclose(xwin *this) {
  XEvent ke;
  ke.type = ClientMessage;
  ke.xclient.window = this->w;
  ke.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
  ke.xclient.format = 32;
  ke.xclient.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
  ke.xclient.data.l[1] = CurrentTime;
  XSendEvent(dpy, this->w, False, NoEventMask, &ke);
}
static void xwintogglefullscreen(xwin *this) {
  if ((this->vh & 3) == 3) { // toggle from fullscreen
    xwingeomset(this, this->gx, this->gy, this->gw, this->gh);
    this->vh = 0;
  } else { // toggle to fullscreen
    xwingeom(this);
    xwingeomset(this, -WIN_BORDER_WIDTH, -WIN_BORDER_WIDTH, scr.wi, scr.hi);
    this->vh |= 3;
  }
}
static void xwintogglefullheight(xwin *this) {
  if (this->vh & 1) {
    int gy = this->gy;
    int gh = this->gh;
    xwingeom(this);
    xwingeomset(this, this->gx, gy, this->gw, gh);
  } else {
    xwingeom(this);
    xwingeomset(this, this->gx, -WIN_BORDER_WIDTH, this->gw, scr.hi);
  }
  this->vh ^= 1;
}
static void xwintogglefullwidth(xwin *this) {
  if (this->vh & 2) {
    int gx = this->gx;
    int gw = this->gw;
    xwingeom(this);
    xwingeomset(this, gx, this->gy, gw, this->gh);
  } else {
    xwingeom(this);
    xwingeomset(this, -WIN_BORDER_WIDTH, this->gy, scr.wi, this->gh);
  }
  this->vh ^= 2;
}
static void xwinhide(xwin *this) {
  xwingeom(this);
  this->desk_x = this->gx;
  int slip = rand() % WIN_SLIP;
  this->gx = (scr.wi - 13 + slip); //? magicnum
  xwingeomset2(this);
}
static void xwinshow(xwin *this) {
  this->gx = this->desk_x;
  xwingeomset2(this);
}
static void xwinbump(xwin *this, int r) {
  xwingeom(this);
  this->gx += (rand() % r) - (r >> 1); //? magicnum
  this->gy += (rand() % r) - (r >> 1); //? magicnum
  xwingeomset2(this);
}
static int _xwinix(xwin *this) {
  if (this == NULL)
    return -1;
  int k;
  for (k = 0; k < WIN_MAX_COUNT; k++)
    if (this == &wins[k])
      return k;
  return -1;
}
static int _focustry(int k) {
  xwin *w = &wins[k];
  if ((w->bits & 1) && (w->desk == dsk)) {
    xwinraise(w);
    xwinfocus(w);
    return 1;
  }
  return 0;
}
static void focusnext() {
  int k0 = _xwinix(winfocused);
  int k = k0;
  while (++k < WIN_MAX_COUNT) {
    if (_focustry(k)) {
      return;
    }
  }
  k = 0;
  while (k <= k0) {
    if (_focustry(k)) {
      return;
    }
    k++;
  }
  focusfirstondesk();
}
static void focusprev() {
  int k0 = _xwinix(winfocused);
  int k = k0;
  while (--k >= 0) {
    if (_focustry(k)) {
      return;
    }
  }
  k = WIN_MAX_COUNT;
  while (--k >= 0) {
    if (_focustry(k)) {
      return;
    }
  }
  focusfirstondesk();
}
static void togglefullscreen() {
  if (!winfocused)
    return;
  xwintogglefullscreen(winfocused);
}
static void togglefullheight() {
  if (!winfocused)
    return;
  xwintogglefullheight(winfocused);
}
static void togglefullwidth() {
  if (!winfocused)
    return;
  xwintogglefullwidth(winfocused);
}
static void deskshow(int dsk, int dskprv) {
  int n;
  for (n = 0; n < WIN_MAX_COUNT; n++) {
    xwin *xw = &wins[n];
    if (!(xw->bits & 1)) //? magicnum
      continue;
    if (xw->w == 0)
      continue;
    if (xw->w == root)
      continue;
    if (xw->desk == dskprv)
      xwinhide(xw);
    if (xw->desk == dsk)
      xwinshow(xw);
  }
}
// static void desksave(int dsk, FILE *f) {
//   int n = 0;
//   fprintf(flog, "desktop %d\n", dsk);
//   fflush(flog);
//   for (n = 0; n < xwinsct; n++) {
//     xwin *w = &wins[n];
//     if (!(w->bits & 1)) //? magicnum
//       continue;
//     xwingeom(w);
//     char **argv;
//     int argc;
//     XGetCommand(dpy, w->w, &argv, &argc);
//     if (argc > 0)
//       while (argc--)
//         fprintf(flog, "%s ", *argv++);
//     else
//       fprintf(flog, "%x", (unsigned int)w->w);
//     //		fprintf(f,"   %x %dx%d+%d+%d\n",(unsigned
//     // int)w->w,w->rw,w->rh,w->rx,w->ry);
//     fflush(f);
//   }
// }
static int errorhandler(Display *d, XErrorEvent *e) {
  char buffer_return[1024] = "";
  int length = 1024;
  XGetErrorText(d, e->error_code, buffer_return, length);
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
  puts(APP);
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
      XGrabButton(dpy, AnyButton, Mod4Mask, xw->w, True, ButtonPressMask,
                  GrabModeAsync, GrabModeAsync, None, None);
      XSelectInput(dpy, xw->w, EnterWindowMask);
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
        //			case 118://insert
        //			case 127://pause
      case 57: // n
        // desksave(dsk, flog);
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
      case 96: // F12
        //				XSetCloseDownMode(dpy,RetainPermanent);
        //				XCloseDisplay(dpy);
        break;
      case 38:  // a
      case 111: // up
        dskprv = dsk;
        dsk++;
        if (ev.xkey.state & ShiftMask) {
          if (winfocused) {
            winfocused->desk = dsk;
            xwingeom(winfocused);
            winfocused->desk_x = winfocused->gx;
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
            winfocused->desk_x = winfocused->gx;
            xwinraise(winfocused);
          }
        }
        deskshow(dsk, dskprv);
        break;
      }
      break;
    case KeyRelease:
      if (key == ev.xkey.keycode)
        key = 0;
      break;
    case ButtonPress:
      dragging = True;
      xw = xwinget(ev.xbutton.window);
      xwinfocus(xw);
      XGrabPointer(dpy, xw->w, True, PointerMotionMask | ButtonReleaseMask,
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
      int nx = xw->gx + xdiff;
      int nw = xw->gw + xdiff;
      int ny = xw->gy + ydiff;
      int nh = xw->gh + ydiff;
      if (xw->vh & 2) {
        nx = -WIN_BORDER_WIDTH;
        nw = scr.wi;
      }
      if (xw->vh & 1) {
        ny = -WIN_BORDER_WIDTH;
        nh = scr.hi;
      }
      if (buttonevstart.button == 3) {
        if (nw < 0)
          nw = 0;
        if (nh < 0)
          nh = 0;
        xwingeomset(xw, xw->gx, xw->gy, nw, nh);
        break;
      }
      switch (key) {
      default:
        xwingeomset(xw, nx, ny, xw->gw, xw->gh);
        break;
      case 27: // r
        if (nw < 0)
          nw = 0;
        if (nh < 0)
          nh = 0;
        xwingeomset(xw, xw->gx, xw->gy, nw, nh);
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