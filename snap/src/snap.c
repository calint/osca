#include <X11/Xlib.h>
#include <cairo-xlib.h>
#include <cairo.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    puts("usage: snap <filename>");
    return -1;
  }
  const char *file_name = argv[1];
  Display *disp = XOpenDisplay(NULL);
  int scr = DefaultScreen(disp);
  Window root = DefaultRootWindow(disp);
  cairo_surface_t *surface = cairo_xlib_surface_create(
      disp, root, DefaultVisual(disp, scr), DisplayWidth(disp, scr),
      DisplayHeight(disp, scr));
  cairo_surface_write_to_png(surface, file_name);
  cairo_surface_destroy(surface);
  return 0;
}
