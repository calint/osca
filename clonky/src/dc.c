#include "dc.h"
#include <X11/Xft/Xft.h>
#include <locale.h>
#include <stdio.h>

struct dc {
  int scr;
  Display *dpy;
  Window win;
  GC gc;
  Colormap color_map;
  XftFont *font;
  XftDraw *draw;
  XftColor color;
  unsigned display_width;
  unsigned display_height;
  unsigned width;
  int top_y;
  unsigned hr_pixels_before;
  unsigned hr_pixels_after;
  int margin_left;
  unsigned line_height;
  int current_y;
  int current_x;
  XRenderColor render_color;
};

/*gives*/ struct dc *dc_new(const char *font_name, double font_size,
                            unsigned line_height, int top_y, unsigned width,
                            unsigned hr_pixels_before, unsigned hr_pixels_after,
                            unsigned align) {
  struct dc *self = calloc(1, sizeof(struct dc));
  setlocale(LC_ALL, "");
  self->dpy = XOpenDisplay(NULL);
  if (!self->dpy) {
    printf("!!! could not open display\n");
    exit(1);
  }
  self->scr = DefaultScreen(self->dpy);
  self->display_width = (unsigned)DisplayWidth(self->dpy, self->scr);
  self->display_height = (unsigned)DisplayHeight(self->dpy, self->scr);
  self->width = width;
  self->margin_left = align == 0 ? 0 : (int)(self->display_width - width);
  self->top_y = top_y;
  self->hr_pixels_before = hr_pixels_before;
  self->hr_pixels_after = hr_pixels_after;
  self->line_height = line_height;
  self->current_x = self->margin_left;
  self->current_y = 0;
  self->win = RootWindow(self->dpy, self->scr);
  self->gc = XCreateGC(self->dpy, self->win, 0, NULL);
  self->color_map = DefaultColormap(self->dpy, self->scr);
  self->font = XftFontOpen(self->dpy, self->scr, XFT_FAMILY, XftTypeString,
                           font_name, XFT_ENCODING, XftTypeString, "UTF-8",
                           XFT_SIZE, XftTypeDouble, font_size, NULL);
  self->draw =
      XftDrawCreate(self->dpy, self->win, DefaultVisual(self->dpy, self->scr),
                    self->color_map);
  XRenderColor xrendcolwhite = {0xffff, 0xffff, 0xffff, 0xffff};
  self->render_color = xrendcolwhite;
  XftColorAllocValue(self->dpy, DefaultVisual(self->dpy, self->scr),
                     self->color_map, &self->render_color, &self->color);
  return self;
}

void dc_del(/*takes*/ struct dc *self) {
  puts("· deleting dc");
  XftDrawDestroy(self->draw);
  XftFontClose(self->dpy, self->font);
  XFree(self->gc);
  free(self);
}

void dc_clear(struct dc *self) {
  XSetForeground(self->dpy, self->gc, BlackPixel(self->dpy, self->scr));
  XFillRectangle(self->dpy, self->win, self->gc, self->margin_left, self->top_y,
                 self->width,
                 (unsigned)((int)self->display_height - self->top_y));
  XSetForeground(self->dpy, self->gc, WhitePixel(self->dpy, self->scr));
  self->current_y = self->top_y;
  self->current_x = self->margin_left;
}

void dc_draw_line(struct dc *self, const int x0, const int y0, const int x1,
                  const int y1) {
  XDrawLine(self->dpy, self->win, self->gc, x0, y0, x1, y1);
}

void dc_newline(struct dc *self) {
  self->current_y += (int)self->line_height;
  self->current_x = self->margin_left;
}

void dc_draw_str(struct dc *self, const char *str) {
  XftDrawStringUtf8(self->draw, &self->color, self->font, self->current_x,
                    self->current_y, (const FcChar8 *)str, (int)strlen(str));
}

void dc_draw_hr(struct dc *self) {
  self->current_y += (int)self->hr_pixels_before;
  XDrawLine(self->dpy, self->win, self->gc, self->margin_left, self->current_y,
            self->margin_left + (int)self->width, self->current_y);
  self->current_y += (int)self->hr_pixels_after;
}

void dc_draw_hr1(struct dc *self, const int width) {
  self->current_y += (int)self->hr_pixels_before;
  XDrawLine(self->dpy, self->win, self->gc, self->margin_left, self->current_y,
            self->margin_left + width, self->current_y);
  self->current_y += (int)self->hr_pixels_after;
}

void dc_inc_y(struct dc *self, const int dy) { self->current_y += dy; }

void dc_flush(const struct dc *self) { XFlush(self->dpy); }

int dc_get_x(const struct dc *self) { return self->current_x; }

int dc_get_y(const struct dc *self) { return self->current_y; }
