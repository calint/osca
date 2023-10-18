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
struct dc *dc_new() {
  struct dc *this = calloc(sizeof(struct dc), 1);
  setlocale(LC_ALL, "");
  this->dpy = XOpenDisplay(NULL);
  if (!this->dpy) {
    fprintf(stderr, "!!! could not open display\n");
    return 0;
  }
  this->scr = DefaultScreen(this->dpy);
  this->dpy_width = DisplayWidth(this->dpy, this->scr);
  this->dpy_height = DisplayHeight(this->dpy, this->scr);
  this->width = this->dpy_width;
  this->xlft = 0;
  this->ytop = 0;
  this->dotx = 0;
  this->doty = 0;
  this->ddoty = 10;
  this->win = RootWindow(this->dpy, this->scr);
  this->gc = XCreateGC(this->dpy, this->win, 0, NULL);
  this->cmap = DefaultColormap(this->dpy, this->scr);
  this->font = XftFontOpen(this->dpy, this->scr, XFT_FAMILY, XftTypeString,
                           "sans", XFT_SIZE, XftTypeDouble, 7.0, NULL);
  this->draw = XftDrawCreate(this->dpy, this->win,
                             DefaultVisual(this->dpy, this->scr), this->cmap);
  XRenderColor xrendcolwhite = {0xffff, 0xffff, 0xffff, 0xffff};
  this->rendcol = xrendcolwhite;
  XftColorAllocValue(this->dpy, DefaultVisual(this->dpy, this->scr), this->cmap,
                     &this->rendcol, &this->color);
  return this;
}
void dc_del(struct dc *this) {
  XftDrawDestroy(this->draw);
  XftFontClose(this->dpy, this->font);
  XFree(this->gc);
  free(this);
}
void dc_clear(struct dc *this) {
  XSetForeground(this->dpy, this->gc, BlackPixel(this->dpy, this->scr));
  XFillRectangle(this->dpy, this->win, this->gc, this->xlft, this->ytop,
                 this->width, this->dpy_height);
  XSetForeground(this->dpy, this->gc, WhitePixel(this->dpy, this->scr));
}
void dc_draw_line(struct dc *this, const int x0, const int y0, const int x1,
               const int y1) {
  XDrawLine(this->dpy, this->win, this->gc, this->xlft + x0, this->ytop + y0,
            this->xlft + x1, this->ytop + y1);
}
void dc_newline(struct dc *this) { this->doty += this->ddoty; }
void dc_draw_str(struct dc *this, const char *s) {
  XftDrawStringUtf8(this->draw, &this->color, this->font,
                    this->xlft + this->dotx, this->ytop + this->doty,
                    (FcChar8 *)s, strlen(s));
}
void dc_draw_hr(struct dc *this) {
  this->doty += 3;
  XDrawLine(this->dpy, this->win, this->gc, this->xlft, this->doty,
            this->xlft + this->width, this->doty);
}
void dc_draw_hr1(struct dc *this, const int w) {
  this->doty += 3;
  XDrawLine(this->dpy, this->win, this->gc, this->xlft, this->doty,
            this->xlft + w, this->doty);
}
void dc_inc_y(struct dc *this, const int dy) { this->doty += dy; }
void dc_flush(const struct dc *this) { XFlush(this->dpy); }
int dc_get_x(const struct dc *this) { return this->dotx; }
void dc_set_x(struct dc *this, const int x) { this->dotx = x; }
void dc_set_left_x(struct dc *this, const int x) { this->xlft = x; }
int dc_get_y(const struct dc *this) { return this->doty; }
void dc_set_y(struct dc *this, const int y) { this->doty = y; }
int dc_get_width(const struct dc *this) { return this->width; }
void dc_set_width(struct dc *this, const int width) { this->width = width; }
int dc_get_screen_width(const struct dc *this) { return this->dpy_width; }
