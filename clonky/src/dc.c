#include "dc.h"
#include <X11/Xft/Xft.h>
#include <locale.h>
#include <stdio.h>

struct dc {
  int screen;
  Display *display;
  Window window;
  GC gc;
  Colormap color_map;
  XftFont *font;
  XftDraw *draw;
  XftColor color;
  unsigned display_width;
  unsigned display_height;
  unsigned width;
  int margin_top;
  unsigned pixels_before_hr;
  unsigned pixels_after_hr;
  int margin_left;
  unsigned line_height;
  int current_y;
  int current_x;
  XRenderColor render_color;
};

/*gives*/ struct dc *dc_new(const char *font_name, double font_size,
                            unsigned line_height, int margin_top,
                            unsigned width, unsigned pixels_before_hr,
                            unsigned pixels_after_hr, unsigned align) {
  struct dc *self = calloc(1, sizeof(struct dc));
  setlocale(LC_ALL, "");
  self->display = XOpenDisplay(NULL);
  if (!self->display) {
    printf("!!! could not open display\n");
    exit(1);
  }
  self->screen = DefaultScreen(self->display);
  self->display_width = (unsigned)DisplayWidth(self->display, self->screen);
  self->display_height = (unsigned)DisplayHeight(self->display, self->screen);
  self->width = width;
  self->margin_left = align == 0 ? 0 : (int)(self->display_width - width);
  self->margin_top = margin_top;
  self->pixels_before_hr = pixels_before_hr;
  self->pixels_after_hr = pixels_after_hr;
  self->line_height = line_height;
  self->current_x = self->margin_left;
  self->current_y = 0;
  self->window = RootWindow(self->display, self->screen);
  self->gc = XCreateGC(self->display, self->window, 0, NULL);
  self->color_map = DefaultColormap(self->display, self->screen);
  self->font =
      XftFontOpen(self->display, self->screen, XFT_FAMILY, XftTypeString,
                  font_name, XFT_ENCODING, XftTypeString, "UTF-8", XFT_SIZE,
                  XftTypeDouble, font_size, NULL);
  self->draw = XftDrawCreate(self->display, self->window,
                             DefaultVisual(self->display, self->screen),
                             self->color_map);
  XRenderColor xrendcolwhite = {0xffff, 0xffff, 0xffff, 0xffff};
  self->render_color = xrendcolwhite;
  XftColorAllocValue(self->display, DefaultVisual(self->display, self->screen),
                     self->color_map, &self->render_color, &self->color);
  return self;
}

void dc_del(/*takes*/ struct dc *self) {
  puts("· deleting dc");
  XftDrawDestroy(self->draw);
  XftFontClose(self->display, self->font);
  XFree(self->gc);
  free(self);
}

void dc_clear(struct dc *self) {
  XSetForeground(self->display, self->gc,
                 BlackPixel(self->display, self->screen));
  XFillRectangle(self->display, self->window, self->gc, self->margin_left,
                 self->margin_top, self->width,
                 (unsigned)((int)self->display_height - self->margin_top));
  XSetForeground(self->display, self->gc,
                 WhitePixel(self->display, self->screen));
  self->current_y = self->margin_top;
  self->current_x = self->margin_left;
}

void dc_draw_line(const struct dc *self, const int x0, const int y0,
                  const int x1, const int y1) {
  XDrawLine(self->display, self->window, self->gc, x0, y0, x1, y1);
}

void dc_newline(struct dc *self) {
  self->current_y += (int)self->line_height;
  self->current_x = self->margin_left;
}

void dc_draw_str(const struct dc *self, const char *str) {
  XftDrawStringUtf8(self->draw, &self->color, self->font, self->current_x,
                    self->current_y, (const FcChar8 *)str, (int)strlen(str));
}

void dc_draw_hr(struct dc *self) {
  self->current_y += (int)self->pixels_before_hr;
  XDrawLine(self->display, self->window, self->gc, self->margin_left,
            self->current_y, self->margin_left + (int)self->width,
            self->current_y);
  self->current_y += (int)self->pixels_after_hr;
}

void dc_draw_hr1(struct dc *self, const int width) {
  self->current_y += (int)self->pixels_before_hr;
  XDrawLine(self->display, self->window, self->gc, self->margin_left,
            self->current_y, self->margin_left + width, self->current_y);
  self->current_y += (int)self->pixels_after_hr;
}

void dc_inc_y(struct dc *self, const int dy) { self->current_y += dy; }

void dc_flush(const struct dc *self) { XFlush(self->display); }

int dc_get_x(const struct dc *self) { return self->current_x; }

int dc_get_y(const struct dc *self) { return self->current_y; }
