#include "dc.h"
#include <X11/Xft/Xft.h>
#include <locale.h>

struct dc {
  int scr;
  Display *dpy;
  Window win;
  GC gc;
  Colormap cmap;
  XftFont *font;
  XftDraw *draw;
  XftColor color;
  unsigned int dpy_width;
  unsigned int dpy_height;
  unsigned int width;
  int xlft;
  int ytop;
  int ddoty;
  int doty;
  int dotx;
  XRenderColor rendcol;
};

struct /*give*/ dc *dc_new() {
  struct dc *self = calloc(sizeof(struct dc), 1);
  setlocale(LC_ALL, "");
  self->dpy = XOpenDisplay(NULL);
  if (!self->dpy) {
    fprintf(stderr, "!!! could not open display\n");
    return 0;
  }
  self->scr = DefaultScreen(self->dpy);
  self->dpy_width = DisplayWidth(self->dpy, self->scr);
  self->dpy_height = DisplayHeight(self->dpy, self->scr);
  self->width = self->dpy_width;
  self->xlft = 0;
  self->ytop = 0;
  self->dotx = 0;
  self->doty = 0;
  self->ddoty = 10;
  self->win = RootWindow(self->dpy, self->scr);
  self->gc = XCreateGC(self->dpy, self->win, 0, NULL);
  self->cmap = DefaultColormap(self->dpy, self->scr);
  self->font = XftFontOpen(self->dpy, self->scr, XFT_FAMILY, XftTypeString,
                           "sans", XFT_SIZE, XftTypeDouble, 7.0, NULL);
  self->draw = XftDrawCreate(self->dpy, self->win,
                             DefaultVisual(self->dpy, self->scr), self->cmap);
  XRenderColor xrendcolwhite = {0xffff, 0xffff, 0xffff, 0xffff};
  self->rendcol = xrendcolwhite;
  XftColorAllocValue(self->dpy, DefaultVisual(self->dpy, self->scr), self->cmap,
                     &self->rendcol, &self->color);
  return self;
}

void dc_del(/*take*/ struct dc *self) {
  puts("· deleting dc");
  XftDrawDestroy(self->draw);
  XftFontClose(self->dpy, self->font);
  XFree(self->gc);
  free(self);
}

void dc_clear(struct dc *self) {
  XSetForeground(self->dpy, self->gc, BlackPixel(self->dpy, self->scr));
  XFillRectangle(self->dpy, self->win, self->gc, self->xlft, self->ytop,
                 self->width, self->dpy_height);
  XSetForeground(self->dpy, self->gc, WhitePixel(self->dpy, self->scr));
}

void dc_draw_line(struct dc *self, const int x0, const int y0, const int x1,
                  const int y1) {
  XDrawLine(self->dpy, self->win, self->gc, self->xlft + x0, self->ytop + y0,
            self->xlft + x1, self->ytop + y1);
}

void dc_newline(struct dc *self) { self->doty += self->ddoty; }

void dc_draw_str(struct dc *self, const char *s) {
  XftDrawStringUtf8(self->draw, &self->color, self->font,
                    self->xlft + self->dotx, self->ytop + self->doty,
                    (FcChar8 *)s, strlen(s));
}

void dc_draw_hr(struct dc *self) {
  self->doty += 3;
  XDrawLine(self->dpy, self->win, self->gc, self->xlft, self->doty,
            self->xlft + self->width, self->doty);
}

void dc_draw_hr1(struct dc *self, const int w) {
  self->doty += 3;
  XDrawLine(self->dpy, self->win, self->gc, self->xlft, self->doty,
            self->xlft + w, self->doty);
}

void dc_inc_y(struct dc *self, const int dy) { self->doty += dy; }

void dc_flush(const struct dc *self) { XFlush(self->dpy); }

int dc_get_x(const struct dc *self) { return self->dotx; }

void dc_set_x(struct dc *self, const int x) { self->dotx = x; }

void dc_set_left_x(struct dc *self, const int x) { self->xlft = x; }

int dc_get_y(const struct dc *self) { return self->doty; }

void dc_set_y(struct dc *self, const int y) { self->doty = y; }

int dc_get_width(const struct dc *self) { return self->width; }

void dc_set_width(struct dc *self, const int width) { self->width = width; }

int dc_get_screen_width(const struct dc *self) { return self->dpy_width; }
