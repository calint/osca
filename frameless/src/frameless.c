#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdlib.h>
#include <sys/time.h>

// debugging log
// #define FRAMELESS_DEBUG
// #define FRAMELESS_DEBUG_FILE "frameless.log"

#ifdef FRAMELESS_DEBUG
#include <stdio.h>

static const char* ix_event_names[LASTEvent] = {
    "unknown",        "unknown",        "KeyPress",         "KeyRelease",
    "ButtonPress",    "ButtonRelease",  "MotionNotify",     "EnterNotify",
    "LeaveNotify",    "FocusIn",        "FocusOut",         "KeymapNotify",
    "Expose",         "GraphicsExpose", "NoExpose",         "VisibilityNotify",
    "CreateNotify",   "DestroyNotify",  "UnmapNotify",      "MapNotify",
    "MapRequest",     "ReparentNotify", "ConfigureNotify",  "ConfigureRequest",
    "GravityNotify",  "ResizeRequest",  "CirculateNotify",  "CirculateRequest",
    "PropertyNotify", "SelectionClear", "SelectionRequest", "SelectionNotify",
    "ColormapNotify", "ClientMessage",  "MappingNotify",    "GenericEvent"};

// log file
static FILE* flog;
#endif

// maximum number of windows
#define WIN_MAX_COUNT 128

// pixels to show of a window folded to the right; example 21
#define WIN_SLIP_DX 0

// random pixels relative to WIN_SLIP_DX; example 7
#define WIN_SLIP 0

// window border width
#define WIN_BORDER_WIDTH 1

#define WIN_BORDER_ACTIVE_COLOR 0x00008000

#define WIN_BORDER_INACTIVE_COLOR 0x00000000

// minimum width and height
#define WIN_MIN_WIDTH_HEIGHT 4

// pixels to 'bump' a window
#define WIN_BUMP_PX 200

// key bindings (use 'xev' to find out keycode for key)
#define KEY_LAUNCH_CONSOLE 54 // c
#define CMD_LAUNCH_CONSOLE "osca-console"

#define KEY_LAUNCH_FILES 41 // f
#define CMD_LAUNCH_FILES "osca-files"

#define KEY_LAUNCH_EDITOR 26 // e
#define CMD_LAUNCH_EDITOR "osca-editor"

#define KEY_LAUNCH_MEDIA 58 // m
#define CMD_LAUNCH_MEDIA "osca-media"

#define KEY_LAUNCH_MIXER 55 // v
#define CMD_LAUNCH_MIXER "osca-mixer"

#define KEY_LAUNCH_INTERNET 31 // i
#define CMD_LAUNCH_INTERNET "osca-internet"

#define KEY_LAUNCH_STICKY 53 // x
#define CMD_LAUNCH_STICKY "osca-sticky"

#define KEY_LAUNCH_BINARIES 32 // o
#define CMD_LAUNCH_BINARIES "osca-binaries"

#define KEY_LAUNCH_SNAPSHOT 33 // p
#define CMD_LAUNCH_SNAPSHOT "osca-screenshot"
#define CMD_LAUNCH_SNAPSHOT_SHIFT "osca-screenshot-select"

#define CMD_FN_SCREEN_BRIGHTNESS_DOWN "osca-screen-brightness-down"
#define CMD_FN_SCREEN_BRIGHTNESS_UP "osca-screen-brightness-up"
#define CMD_FN_VOLUME_TOGGLE "osca-volume-toggle"
#define CMD_FN_VOLUME_DOWN "osca-volume-down"
#define CMD_FN_VOLUME_UP "osca-volume-up"

#define KEY_WINDOW_CLOSE 9            // esc
#define KEY_WINDOW_CLOSE_ALT 49       // §
#define KEY_WINDOW_BUMP 56            // b
#define KEY_WINDOW_CENTER 39          // s
#define KEY_WINDOW_WIDER 25           // w
#define KEY_WINDOW_RESIZE 27          // r
#define KEY_WINDOW_FULLSCREEN 10      // 1
#define KEY_WINDOW_FULLHEIGHT 11      // 2
#define KEY_WINDOW_FULLWIDTH 12       // 3
#define KEY_WINDOW_SURPRISE 19        // 0
#define KEY_WINDOW_FOCUS_PREVIOUS 113 // left
#define KEY_WINDOW_FOCUS_NEXT 114     // right
#define KEY_WINDOW_FOCUS_NEXT_ALT 52  // z
#define KEY_WINDOW_KILL 119           // del

#define KEY_DESKTOP_UP 111      // up
#define KEY_DESKTOP_UP_ALT 24   // q
#define KEY_DESKTOP_DOWN 116    // down
#define KEY_DESKTOP_DOWN_ALT 38 // a
#define KEY_DESKTOP_0 43        // h
#define KEY_DESKTOP_1 44        // j
#define KEY_DESKTOP_2 45        // k
#define KEY_DESKTOP_3 46        // l

#define IGNORED_ENTER_AFTER_MAP_TIME_US 500000
// time to ignore enter notifies after a map notify
// note: solves the problem with newly launched applications losing focus

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

// xwin.bits
#define XWIN_BIT_FULL_HEIGHT 1
#define XWIN_BIT_FULL_WIDTH 2
#define XWIN_BIT_ALLOCATED 4
#define XWIN_BIT_FOCUSED 8
#define XWIN_BITS_FULL_SCREEN 3

// mapped xwin to X11 Window
static xwin wins[WIN_MAX_COUNT];

// number of windows mapped
static unsigned xwin_count;

static Display* display;
static Window root_window;
static int current_desk;
static xwin* focused_window;
static suseconds_t time_of_last_map_notify_us;

static Bool is_switching_desktop;
// True while switching desktop, False after any key release

// default screen info
static struct screen {
    unsigned id;
    unsigned wi; // width
    unsigned hi; // height
} screen;

static suseconds_t current_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

static xwin* xwin_get_by_window(Window w) {
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
#ifdef FRAMELESS_DEBUG
        fprintf(flog, "!!! no free windows\n");
        fflush(flog);
#endif
        exit(3);
    }
    xwin* xw = &wins[first_avail];
    xw->bits = XWIN_BIT_ALLOCATED;
    xwin_count++;
    xw->win = w;
    xw->desk = current_desk;
    XSetWindowBorderWidth(display, w, WIN_BORDER_WIDTH);
    return xw;
}

static void xwin_focus_on(xwin* this) {
    XSetInputFocus(display, this->win, RevertToParent, CurrentTime);
    XSetWindowBorder(display, this->win, WIN_BORDER_ACTIVE_COLOR);
    this->bits |= XWIN_BIT_FOCUSED;
}

static void xwin_focus_off(xwin* this) {
    XSetWindowBorder(display, this->win, WIN_BORDER_INACTIVE_COLOR);
    this->bits &= ~XWIN_BIT_FOCUSED;
}

static void xwin_raise(xwin* this) { XRaiseWindow(display, this->win); }

static xwin* winx_find_by_window(Window w) {
    for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
        if (wins[i].win == w)
            return &wins[i];
    }
    return NULL;
}

static void xwin_read_geom(xwin* this) {
    Window wsink;
    unsigned dummy;
    XGetGeometry(display, this->win, &wsink, &this->x, &this->y, &this->wi,
                 &this->hi, &dummy, &dummy);
}

static void xwin_set_geom(xwin* this) {
    XMoveResizeWindow(display, this->win, this->x, this->y, this->wi, this->hi);
}

static void xwin_center(xwin* this) {
    xwin_read_geom(this);
    if (!(this->bits & XWIN_BIT_FULL_WIDTH)) {
        // only adjust x if window is not in full width
        this->x = (int)((screen.wi - this->wi) >> 1);
    }
    if (!(this->bits & XWIN_BIT_FULL_HEIGHT)) {
        // only adjust y if window is not in full height
        this->y = (int)((screen.hi - this->hi) >> 1);
    }
    xwin_set_geom(this);
}

static void xwin_wider(xwin* this) {
    xwin_read_geom(this);
    unsigned wi_prv = this->wi;
    this->wi = ((this->wi << 2) + this->wi) >> 2;
    this->x = this->x - (int)((this->wi - wi_prv) >> 1);
    xwin_set_geom(this);
}

static void xwin_thinner(xwin* this) {
    xwin_read_geom(this);
    unsigned wi_prv = this->wi;
    this->wi = ((this->wi << 1) + this->wi) >> 2;
    if (this->wi < WIN_MIN_WIDTH_HEIGHT) {
        this->wi = WIN_MIN_WIDTH_HEIGHT;
    }
    this->x = this->x - (int)((this->wi - wi_prv) >> 1);
    xwin_set_geom(this);
}

static void xwin_close(xwin* this) {
    XEvent ke;
    ke.type = ClientMessage;
    ke.xclient.window = this->win;
    ke.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", True);
    ke.xclient.format = 32;
    ke.xclient.data.l[0] = (long)XInternAtom(display, "WM_DELETE_WINDOW", True);
    ke.xclient.data.l[1] = CurrentTime;
    XSendEvent(display, this->win, False, NoEventMask, &ke);
}

static void xwin_toggle_fullscreen(xwin* this) {
    xwin_read_geom(this);
    if ((this->bits & XWIN_BITS_FULL_SCREEN) == XWIN_BITS_FULL_SCREEN) {
        // toggle from fullscreen
        this->x = this->x_pf;
        this->y = this->y_pf;
        this->wi = this->wi_pf;
        this->hi = this->hi_pf;
        this->bits &= ~XWIN_BITS_FULL_SCREEN;
    } else {
        // toggle to fullscreen
        this->x_pf = this->x;
        this->y_pf = this->y;
        this->wi_pf = this->wi;
        this->hi_pf = this->hi;
        this->x = -WIN_BORDER_WIDTH;
        this->y = -WIN_BORDER_WIDTH;
        this->wi = screen.wi;
        this->hi = screen.hi;
        this->bits |= XWIN_BITS_FULL_SCREEN;
    }
    xwin_set_geom(this);
}

static void xwin_toggle_fullheight(xwin* this) {
    xwin_read_geom(this);
    if (this->bits & XWIN_BIT_FULL_HEIGHT) {
        this->y = this->y_pf;
        this->hi = this->hi_pf;
    } else {
        this->y_pf = this->y;
        this->hi_pf = this->hi;
        this->y = -WIN_BORDER_WIDTH;
        this->hi = screen.hi;
    }
    this->bits ^= XWIN_BIT_FULL_HEIGHT;
    xwin_set_geom(this);
}

static void xwin_toggle_fullwidth(xwin* this) {
    xwin_read_geom(this);
    if (this->bits & XWIN_BIT_FULL_WIDTH) {
        this->x = this->x_pf;
        this->wi = this->wi_pf;
    } else {
        this->x_pf = this->x;
        this->wi_pf = this->wi;
        this->x = -WIN_BORDER_WIDTH;
        this->wi = screen.wi;
    }
    this->bits ^= XWIN_BIT_FULL_WIDTH;
    xwin_set_geom(this);
}

// fold window to the right of screen
static void xwin_hide(xwin* this) {
    xwin_read_geom(this);
    this->desk_x = this->x;
    unsigned slip = WIN_SLIP == 0 ? 0 : (unsigned)(rand() % WIN_SLIP);
    this->x = (int)(screen.wi - WIN_SLIP_DX + slip);
    xwin_set_geom(this);
}

// unfold window from the right of screen
static void xwin_show(xwin* this) {
    this->x = this->desk_x;
    xwin_set_geom(this);
}

static void xwin_bump(xwin* this, int rand_amt) {
    xwin_read_geom(this);
    this->x += (rand() % rand_amt) - (rand_amt >> 1);
    this->y += (rand() % rand_amt) - (rand_amt >> 1);
    xwin_set_geom(this);
}

static int xwin_ix(xwin* this) {
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

static void focus_on_window(xwin* xw) {
    if (focused_window) {
        xwin_focus_off(focused_window);
    }
    xwin_focus_on(xw);
    focused_window = xw;
}

// returns True if window got focus False otherwise
static Bool focus_window_by_index_try(unsigned ix) {
    xwin* xw = &wins[ix];
    if (xw->bits & XWIN_BIT_ALLOCATED && xw->desk == current_desk) {
        xwin_raise(xw);
        focus_on_window(xw);
        return True;
    }
    return False;
}

static void desk_show(int dsk, int dsk_prv) {
    for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
        xwin* xw = &wins[i];
        if (!(xw->bits & XWIN_BIT_ALLOCATED)) {
            continue;
        }
        if (xw->win == 0 || xw->win == root_window) {
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

// returns True if there is only one window and is focused
static Bool focus_on_only_window_on_desk(void) {
    xwin* focus = NULL;
    for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
        xwin* xw = &wins[i];
        if (xw->bits & XWIN_BIT_ALLOCATED && xw->desk == current_desk) {
            if (focus) {
                return False; // there are more than 1 window on
                              // this desk
            }
            focus = xw;
        }
    }
    if (focus) {
        focus_on_window(focus);
        return True;
    }
    return False;
}

// focus on previously focused window if available or some window on this
// desktop
static void focus_window_after_desk_switch(void) {
    xwin* focus = NULL;
    for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
        xwin* xw = &wins[i];
        if (xw->bits & XWIN_BIT_ALLOCATED && xw->desk == current_desk) {
            if (xw->bits & XWIN_BIT_FOCUSED) {
                // found focused window on this desk
                focus = xw;
                break;
            }
            // focus on some window on this desk
            focus = xw;
        }
    }
    focused_window = NULL;
    if (!focus) {
        // didn't find any window to focus; focus on root window
        XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
        return;
    }
    focus_on_window(focus);
}

// turns off focus for window on desktop 'dsk'
// used when switching desktop with window (host + shift + up/down)
static void turn_off_window_focus_on_desk(int dsk) {
    for (unsigned i = 0; i < WIN_MAX_COUNT; i++) {
        xwin* xw = &wins[i];
        if (xw->bits & XWIN_BIT_ALLOCATED && xw->desk == dsk &&
            xw->bits & XWIN_BIT_FOCUSED) {
            xwin_focus_off(xw);
            if (focused_window == xw) {
                focused_window = NULL;
            }
        }
    }
}

static void focus_next_window(void) {
    int i0 = xwin_ix(focused_window);
    // note: 'i0' might be -1 but incremented to 0
    int i = i0;
    while (++i < WIN_MAX_COUNT) {
        if (focus_window_by_index_try((unsigned)i)) {
            return;
        }
    }
    i = 0;
    // note: if 'i0' is -1 then no window will be focused
    // '<= i0' not '< i0' to focus on same window if no other window
    // available
    while (i <= i0) {
        if (focus_window_by_index_try((unsigned)i)) {
            return;
        }
        i++;
    }
}

static void focus_previous_window(void) {
    int i0 = xwin_ix(focused_window);
    int i = i0;
    while (--i >= 0) {
        if (focus_window_by_index_try((unsigned)i)) {
            return;
        }
    }
    i = WIN_MAX_COUNT;
    // '>= i0' not '> i0' to focus on same window if no other window
    // available
    while (--i >= i0) {
        if (focus_window_by_index_try((unsigned)i)) {
            return;
        }
    }
}

static void free_window_and_resolve_focus(Window w) {
    xwin* win = winx_find_by_window(w);
    if (!win) {
        // window not found
        return;
    }
    win->bits = 0; // mark free
    xwin_count--;
    if (focused_window == win) {
        focused_window = NULL;
    }
    // try to focus window under pointer
    Window root_return, child_return;
    int root_x, root_y, win_x, win_y;
    unsigned mask_return;
    if (XQueryPointer(display, root_window, &root_return, &child_return,
                      &root_x, &root_y, &win_x, &win_y, &mask_return)) {
        if (child_return != None) {
            xwin* xw = xwin_get_by_window(child_return);
            focus_on_window(xw);
            return;
        }
    }
    // if there is just one window on desk focus it
    focus_on_only_window_on_desk();
}

static void switch_to_desk(int dsk, Bool move_focused_window) {
    if (dsk == current_desk) {
        return;
    }
    is_switching_desktop = True; // set global state
    int dsk_prv = current_desk;
    current_desk = dsk; // set global state
    if (move_focused_window && focused_window) {
        // 'focused_window' is the new focused window on desk
        // 'current_desk'
        turn_off_window_focus_on_desk(current_desk);
        // change desk on focused window
        focused_window->desk = current_desk;
        // set 'desk_x' because 'desk_show' will restore it to 'x'
        focused_window->desk_x = focused_window->x;
        // place focused window on top
        xwin_raise(focused_window);
    }
    desk_show(current_desk, dsk_prv);
    focus_window_after_desk_switch();
}

static int error_handler(Display* d, XErrorEvent* e) {
#ifdef FRAMELESS_DEBUG
    char buf[1024] = "";
    XGetErrorText(d, e->error_code, buf, sizeof(buf));
    fprintf(flog, "!!! x11 error\n");
    fprintf(flog, "!!!         text: %s\n", buf);
    fprintf(flog, "!!!       serial: %lu\n", e->serial);
    fprintf(flog, "!!!         type: %d\n", e->type);
    fprintf(flog, "!!!   error code: %d\n", (unsigned)e->error_code);
    fprintf(flog, "!!! request code: %d\n", e->request_code);
    fprintf(flog, "!!!   minor code: %d\n", e->minor_code);
    fprintf(flog, "!!!  resource id: %p\n", (void*)e->resourceid);
    fflush(flog);
#endif
    return 0;
}

int main(int argc, char** args, char** env) {
#ifdef FRAMELESS_DEBUG
    flog = fopen(FRAMELESS_DEBUG_FILE, "a");
    if (!flog) {
        exit(1);
    }
    fprintf(flog, "frameless window manager\n");
    fprintf(flog, "\narguments:\n");
    while (argc--) {
        fprintf(flog, "%s\n", *args++);
    }
    fprintf(flog, "\nenvironment:\n");
    while (*env) {
        fprintf(flog, "%s\n", *env++);
    }
    fflush(flog);
#endif

    XSetErrorHandler(error_handler);

    display = XOpenDisplay(NULL);
    if (!display) {
        exit(2);
    }

    screen.id = (unsigned)DefaultScreen(display);
    screen.wi = (unsigned)DisplayWidth(display, screen.id);
    screen.hi = (unsigned)DisplayHeight(display, screen.id);

    root_window = DefaultRootWindow(display);
    Cursor root_window_cursor = XCreateFontCursor(display, XC_arrow);
    XDefineCursor(display, root_window, root_window_cursor);
    XGrabKey(display, AnyKey, Mod4Mask, root_window, True, GrabModeAsync,
             GrabModeAsync);
    XGrabKey(display, AnyKey, Mod4Mask + ShiftMask, root_window, True,
             GrabModeAsync, GrabModeAsync);
    XSelectInput(display, root_window, SubstructureNotifyMask);

    unsigned key_pressed = 0; // current key pressed

    // dragging state
    Bool is_dragging = False;
    int dragging_prev_x = 0;
    int dragging_prev_y = 0;
    unsigned dragging_button = 0;

    // key codes for media keys
    const KeyCode key_code_audio_lower_volume =
        XKeysymToKeycode(display, XF86XK_AudioLowerVolume);
    const KeyCode key_code_audio_raise_volume =
        XKeysymToKeycode(display, XF86XK_AudioRaiseVolume);
    const KeyCode key_code_audio_mute =
        XKeysymToKeycode(display, XF86XK_AudioMute);
    const KeyCode key_code_mon_brightness_down =
        XKeysymToKeycode(display, XF86XK_MonBrightnessDown);
    const KeyCode key_code_mon_brightness_up =
        XKeysymToKeycode(display, XF86XK_MonBrightnessUp);

    xwin* move_or_resize_window = NULL;
    // window that is being moved or resized

    XEvent ev = {0};
    while (!XNextEvent(display, &ev)) {
#ifdef FRAMELESS_DEBUG
        fprintf(flog, "%lu: event: %s   win=%p\n", ev.xany.serial,
                ix_event_names[ev.type], (void*)ev.xany.window);
        fflush(flog);
#endif
        switch (ev.type) {
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        default: {
#ifdef FRAMELESS_DEBUG
            fprintf(flog, "  unhandled\n");
            fflush(flog);
#endif
            break;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        case MapNotify: {
            if (ev.xmap.override_redirect) {
#ifdef FRAMELESS_DEBUG
                fprintf(flog, "  ignored\n");
                fflush(flog);
#endif
                break;
            }
            xwin* xw = xwin_get_by_window(ev.xmap.window);
            xwin_center(xw);
            focus_on_window(xw);
            XGrabButton(display, AnyButton, Mod4Mask, xw->win, True,
                        ButtonPressMask, GrabModeAsync, GrabModeAsync, None,
                        None);
            XSelectInput(display, xw->win, EnterWindowMask);
            time_of_last_map_notify_us = current_time_us();
            break;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        case UnmapNotify: {
            free_window_and_resolve_focus(ev.xunmap.window);
            break;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        case EnterNotify: {
            if (is_dragging || is_switching_desktop ||
                current_time_us() - time_of_last_map_notify_us <
                    IGNORED_ENTER_AFTER_MAP_TIME_US) {
                // * if dragging then it is resizing, don't change focus
                // * if switching desktop, don't focus on the window that is
                //   under the pointer; focus on previously focused window on
                //   that desktop
                // * when launching a new window ignore the event for
                //   IGNORED_ENTER_AFTER_MAP_TIME_MS since it might lose focus
                //   from the newly launched application detail: a newly mapped
                //   window is at 0,0 and moved to center of screen and focused.
                //   if the move leaves the pointer on some other window then
                //   that window receives focus. this somewhat hacky way of
                //   avoiding that is good enough compared to the complications.
#ifdef FRAMELESS_DEBUG
                fprintf(flog, "  ignored\n");
                fflush(flog);
#endif
                break;
            }
            xwin* xw = xwin_get_by_window(ev.xcrossing.window);
            focus_on_window(xw);
            break;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        case KeyPress: {
            key_pressed = ev.xkey.keycode;
            switch (key_pressed) {
            case KEY_LAUNCH_CONSOLE:
                system(CMD_LAUNCH_CONSOLE);
                break;
            case KEY_LAUNCH_FILES:
                system(CMD_LAUNCH_FILES);
                break;
            case KEY_LAUNCH_EDITOR:
                system(CMD_LAUNCH_EDITOR);
                break;
            case KEY_LAUNCH_MEDIA:
                system(CMD_LAUNCH_MEDIA);
                break;
            case KEY_LAUNCH_MIXER:
                system(CMD_LAUNCH_MIXER);
                break;
            case KEY_LAUNCH_INTERNET:
                system(CMD_LAUNCH_INTERNET);
                break;
            case KEY_LAUNCH_STICKY:
                system(CMD_LAUNCH_STICKY);
                break;
            case KEY_LAUNCH_BINARIES:
                system(CMD_LAUNCH_BINARIES);
                break;
            case KEY_LAUNCH_SNAPSHOT:
                if (ev.xkey.state & ShiftMask) {
                    system(CMD_LAUNCH_SNAPSHOT_SHIFT);
                } else {
                    system(CMD_LAUNCH_SNAPSHOT);
                }
                break;
            case KEY_WINDOW_CLOSE:
            case KEY_WINDOW_CLOSE_ALT:
                if (focused_window) {
                    xwin_close(focused_window);
                }
                break;
            case KEY_WINDOW_BUMP:
                if (focused_window) {
                    // turn of full width and/or height bit
                    focused_window->bits &=
                        ~(XWIN_BIT_FULL_WIDTH | XWIN_BIT_FULL_HEIGHT);
                    xwin_bump(focused_window, WIN_BUMP_PX);
                }
                break;
            case KEY_WINDOW_CENTER:
                if (focused_window) {
                    xwin_center(focused_window);
                }
                break;
            case KEY_WINDOW_WIDER:
                if (focused_window) {
                    if (ev.xkey.state & ShiftMask) {
                        xwin_thinner(focused_window);
                    } else {
                        xwin_wider(focused_window);
                    }
                    // if window was full width then turn
                    // that bit off
                    focused_window->bits &= ~XWIN_BIT_FULL_WIDTH;
                }
                break;
            case KEY_WINDOW_FULLSCREEN:
                if (focused_window) {
                    xwin_toggle_fullscreen(focused_window);
                }
                break;
            case KEY_WINDOW_FULLHEIGHT:
                if (focused_window) {
                    xwin_toggle_fullheight(focused_window);
                }
                break;
            case KEY_WINDOW_FULLWIDTH:
                if (focused_window) {
                    xwin_toggle_fullwidth(focused_window);
                }
                break;
            case KEY_WINDOW_SURPRISE:
                if (focused_window) {
                    xwin_bump(focused_window, WIN_BUMP_PX);
                }
                break;
            case KEY_WINDOW_FOCUS_PREVIOUS:
                focus_previous_window();
                break;
            case KEY_WINDOW_FOCUS_NEXT:
            case KEY_WINDOW_FOCUS_NEXT_ALT:
                focus_next_window();
                break;
            case KEY_WINDOW_KILL:
                if (ev.xkey.state & ShiftMask) {
                    // log-out
                    return 0;
                }
                XKillClient(display, ev.xkey.subwindow);
                break;
            case KEY_DESKTOP_UP:
            case KEY_DESKTOP_UP_ALT:
                switch_to_desk(current_desk + 1, ev.xkey.state & ShiftMask);
                break;
            case KEY_DESKTOP_DOWN:
            case KEY_DESKTOP_DOWN_ALT:
                switch_to_desk(current_desk - 1, ev.xkey.state & ShiftMask);
                break;
            case KEY_DESKTOP_0:
                switch_to_desk(0, ev.xkey.state & ShiftMask);
                break;
            case KEY_DESKTOP_1:
                switch_to_desk(1, ev.xkey.state & ShiftMask);
                break;
            case KEY_DESKTOP_2:
                switch_to_desk(2, ev.xkey.state & ShiftMask);
                break;
            case KEY_DESKTOP_3:
                switch_to_desk(3, ev.xkey.state & ShiftMask);
                break;

            default:
                // handle media keys
                if (key_pressed == key_code_mon_brightness_down) {
                    system(CMD_FN_SCREEN_BRIGHTNESS_DOWN);
                } else if (key_pressed == key_code_mon_brightness_up) {
                    system(CMD_FN_SCREEN_BRIGHTNESS_UP);
                } else if (key_pressed == key_code_audio_mute) {
                    system(CMD_FN_VOLUME_TOGGLE);
                } else if (key_pressed == key_code_audio_lower_volume) {
                    system(CMD_FN_VOLUME_DOWN);
                } else if (key_pressed == key_code_audio_raise_volume) {
                    system(CMD_FN_VOLUME_UP);
                }
                break;
            }
            break;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        case KeyRelease: {
            if (key_pressed == ev.xkey.keycode) {
                key_pressed = 0;
            }
            //? should check that the key was switch desktop
            is_switching_desktop = False;
            break;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        case ButtonPress: {
            is_dragging = True;
            dragging_prev_x = ev.xbutton.x_root;
            dragging_prev_y = ev.xbutton.y_root;
            dragging_button = ev.xbutton.button;
            move_or_resize_window = xwin_get_by_window(ev.xbutton.window);
            focus_on_window(move_or_resize_window);
            XGrabPointer(display, move_or_resize_window->win, True,
                         PointerMotionMask | ButtonReleaseMask, GrabModeAsync,
                         GrabModeAsync, None, None, CurrentTime);
            xwin_raise(move_or_resize_window);
            xwin_read_geom(move_or_resize_window);
            break;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        case MotionNotify: {
            while (XCheckTypedEvent(display, MotionNotify, &ev))
                ;
            int xdiff = ev.xbutton.x_root - dragging_prev_x;
            int ydiff = ev.xbutton.y_root - dragging_prev_y;
            dragging_prev_x = ev.xbutton.x_root;
            dragging_prev_y = ev.xbutton.y_root;
            int new_x = move_or_resize_window->x + xdiff;
            int new_wi = (int)move_or_resize_window->wi + xdiff;
            int new_y = move_or_resize_window->y + ydiff;
            int new_hi = (int)move_or_resize_window->hi + ydiff;
            if (move_or_resize_window->bits & XWIN_BIT_FULL_WIDTH) {
                new_x = -WIN_BORDER_WIDTH;
                new_wi = (int)screen.wi;
            }
            if (move_or_resize_window->bits & XWIN_BIT_FULL_HEIGHT) {
                new_y = -WIN_BORDER_WIDTH;
                new_hi = (int)screen.hi;
            }
            if (dragging_button == 3) {
                // right mouse button is pressed
                if (new_wi < WIN_MIN_WIDTH_HEIGHT) {
                    new_wi = WIN_MIN_WIDTH_HEIGHT;
                }
                if (new_hi < WIN_MIN_WIDTH_HEIGHT) {
                    new_hi = WIN_MIN_WIDTH_HEIGHT;
                }
                move_or_resize_window->wi = (unsigned)new_wi;
                move_or_resize_window->hi = (unsigned)new_hi;
                xwin_set_geom(move_or_resize_window);
                break;
            }
            switch (key_pressed) {
            default: // moving window
                move_or_resize_window->x = new_x;
                move_or_resize_window->y = new_y;
                xwin_set_geom(move_or_resize_window);
                break;
            case KEY_WINDOW_RESIZE:
                if (new_wi < WIN_MIN_WIDTH_HEIGHT) {
                    new_wi = WIN_MIN_WIDTH_HEIGHT;
                }
                if (new_hi < WIN_MIN_WIDTH_HEIGHT) {
                    new_hi = WIN_MIN_WIDTH_HEIGHT;
                }
                move_or_resize_window->wi = (unsigned)new_wi;
                move_or_resize_window->hi = (unsigned)new_hi;
                xwin_set_geom(move_or_resize_window);
                break;
            }
            break;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        case ButtonRelease: {
            is_dragging = False;
            dragging_button = 0;
            move_or_resize_window = xwin_get_by_window(ev.xbutton.window);
            // in case a window was dragged from folded state
            // (different desktop)
            move_or_resize_window->desk = current_desk;
            XUngrabPointer(display, CurrentTime);
            move_or_resize_window = NULL;
            break;
        }
        }
    }
    //? clean-up is done by OS
    return 0;
}
