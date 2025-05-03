#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// frameless border width (assumed the default 1)
#define FRAMELESS_BORDER_WIDTH 1

// window height
#define WIN_HEIGHT 57

// effect: amount of random displacement of next character on y axis
#define Y_WIGGLE 5

// effect: displacement of y at next character
#define Y_WIGGLE_UP -2

#define FONT_NAME "monospace"
#define FONT_SIZE 24.0 // parameter is double
#define FONT_WIDTH 20  // approximated

// #define MENUQ_DEBUG
#ifdef MENUQ_DEBUG
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
#endif

int main(void) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        // fprintf(stderr, "!!! could not open display\n");
        return 1;
    }
    const int scr = DefaultScreen(dpy);
    const unsigned scr_width = (unsigned)DisplayWidth(dpy, scr);

    // frameless will center the window, set width to screen width plus 2
    // borders
    const Window win = XCreateSimpleWindow(
        dpy, RootWindow(dpy, scr), 0, 0,
        scr_width + (FRAMELESS_BORDER_WIDTH << 1), WIN_HEIGHT, 0,
        BlackPixel(dpy, scr), BlackPixel(dpy, scr));
    XSelectInput(dpy, win, KeyPressMask);
    XMapWindow(dpy, win);

    const GC gc = XCreateGC(dpy, win, 0, NULL);

    // load and set font
    Colormap cmap = DefaultColormap(dpy, scr);
    XftFont *font = XftFontOpen(dpy, scr, XFT_FAMILY, XftTypeString, FONT_NAME,
                                XFT_SIZE, XftTypeDouble, FONT_SIZE, NULL);
    if (font == NULL) {
        fprintf(stderr, "unable to load font.\n");
        return 1;
    }
    XftDraw *draw = XftDrawCreate(dpy, win, DefaultVisual(dpy, scr), cmap);
    XRenderColor rendcol = {0xffff, 0xffff, 0xffff, 0xffff};
    XftColor color;
    XftColorAllocValue(dpy, DefaultVisual(dpy, scr), cmap, &rendcol, &color);

    // initial x and y
    int x = (int)((scr_width >> 1) - (scr_width >> 2) + (scr_width >> 3));
    int y = (WIN_HEIGHT >> 1) + (WIN_HEIGHT >> 2);
    const char cursor_str[] = "_";
    XSetForeground(dpy, gc, WhitePixel(dpy, scr));
    // -1 to exclude '\0'
    XftDrawStringUtf8(draw, &color, font, x, y, (const FcChar8 *)cursor_str,
                      (int)sizeof(cursor_str) - 1);
    // the buffer that accumulates the string to be executed
    char buf[32] = "";
    char *buf_ptr = buf;
    unsigned buf_ix = 0;
    const unsigned options_start_x = (scr_width >> 1) + (scr_width >> 3);

    // loop condition
    char go = True;
    // True if window received WM_DELETE_WINDOW
    char closed = False;

    XEvent ev;
    while (go) {
        XNextEvent(dpy, &ev);
#ifdef MENUQ_DEBUG
        printf("event %s\n", ix_evnames[ev.xany.type]);
#endif
        switch (ev.type) {
        default:
            break;
        case ClientMessage:
            if (ev.xclient.message_type ==
                    XInternAtom(dpy, "WM_PROTOCOLS", False) &&
                ev.xclient.data.l[0] ==
                    (long)XInternAtom(dpy, "WM_DELETE_WINDOW", False)) {
                go = False;
                closed = True;
            }
            break;
        case KeyPress: {
            unsigned keycode = ev.xkey.keycode;
            if (keycode == 9) { // esc
                go = 0;
                buf_ptr = buf;
                *buf_ptr = 0;
                break;
            }
            if (keycode == 36) { // return
                go = 0;
                *buf_ptr = 0;
                break;
            }
            if (keycode == 22) { // backspace
                if (buf_ix == 0) {
                    break;
                }
                // back one character
                x -= FONT_WIDTH;
                // erase the character and cursor
                XSetForeground(dpy, gc, BlackPixel(dpy, scr));
                XFillRectangle(dpy, win, gc, x, 0, FONT_WIDTH << 1, WIN_HEIGHT);
                // draw cursor
                XSetForeground(dpy, gc, WhitePixel(dpy, scr));
                XftDrawStringUtf8(draw, &color, font, x, y,
                                  (const FcChar8 *)cursor_str,
                                  (int)sizeof(cursor_str) - 1);
                // adjust buffer pointer and indexs
                buf_ix--;
                buf_ptr--;
                *buf_ptr = 0;
                break;
            }
            // not escape, return or backspace
            // erase cursor
            XSetForeground(dpy, gc, BlackPixel(dpy, scr));
            XFillRectangle(dpy, win, gc, x, 0, FONT_WIDTH, WIN_HEIGHT);
            // get printable character
            char printable_char[4] = "";
            KeySym keysym = 0;
            XLookupString(&ev.xkey, printable_char, sizeof(printable_char),
                          &keysym, NULL);
            XSetForeground(dpy, gc, WhitePixel(dpy, scr));
            if (!printable_char[0]) {
                // not printable character, print cursor
                // -1 to exclude the '\0'
                XftDrawStringUtf8(draw, &color, font, x, y,
                                  (const FcChar8 *)cursor_str,
                                  (int)sizeof(cursor_str) - 1);
                break;
            }
            // printable character, draw
            XftDrawStringUtf8(draw, &color, font, x, y,
                              (const FcChar8 *)printable_char, 1);
            // append to string
            *buf_ptr++ = printable_char[0];
            buf_ix++;
            // +2 to leave space for '&' and '\0'
            if ((buf_ix + 2) >= sizeof(buf)) {
                // buffer full
                go = 0;
            }
            // update x, y and draw cursor
            x += FONT_WIDTH;
            y += Y_WIGGLE_UP + rand() % Y_WIGGLE;
            // draw cursor
            // -1 to exclude '\0'
            XftDrawStringUtf8(draw, &color, font, x, y,
                              (const FcChar8 *)cursor_str,
                              (int)sizeof(cursor_str) - 1);

            // draw the effect
            XFillRectangle(dpy, win, gc, (int)options_start_x, 0,
                           scr_width - options_start_x - 1, WIN_HEIGHT - 1);
            break;
        }
        }
    }
    // clean-up
    XftDrawDestroy(draw);
    XftFontClose(dpy, font);
    XFreeGC(dpy, gc);
    XCloseDisplay(dpy);
    if (closed) {
        // windows was closed through message
        return 0;
    }
    if (buf[0] == '\0') {
        // empty string
        return 0;
    }
    // append '&' and execute
    *buf_ptr++ = '&';
    *buf_ptr = '\0';
#ifdef MENUQ_DEBUG
    puts(buf);
#endif
    return system(buf);
}
