#include <X11/Xlib.h>
#include <cairo-xlib.h>
#include <cairo.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        puts("usage: snap <filename>");
        return -1;
    }
    const char* file_name = argv[1];
    Display* dsp = XOpenDisplay(NULL);
    int scr = DefaultScreen(dsp);
    Window root = DefaultRootWindow(dsp);
    cairo_surface_t* surface = cairo_xlib_surface_create(
        dsp, root, DefaultVisual(dsp, scr), DisplayWidth(dsp, scr),
        DisplayHeight(dsp, scr));
    cairo_surface_write_to_png(surface, file_name);
    cairo_surface_destroy(surface);
    return 0;
}
