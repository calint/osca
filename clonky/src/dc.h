#ifndef DC_H
#define DC_H

struct dc;

struct /*give*/ dc *dc_new();

void dc_del(/*take*/ struct dc *self);

void dc_clear(struct dc *self);

void dc_draw_line(struct dc *self, const int x0, const int y0, const int x1,
                  const int y1);

void dc_draw_hr(struct dc *self);

void dc_draw_hr1(struct dc *self, const int w);

void dc_draw_str(struct dc *self, const char *s);

void dc_newline(struct dc *self);

void dc_inc_y(struct dc *self, const int dy);

void dc_flush(const struct dc *self);

int dc_get_x(const struct dc *self);

void dc_set_x(struct dc *self, const int x);

int dc_get_y(const struct dc *self);

void dc_set_y(struct dc *self, const int px);

int dc_get_width(const struct dc *self);

int dc_get_screen_width(const struct dc *self);

void dc_set_width(struct dc *self, const int width);

void dc_set_left_x(struct dc *self, const int x);

#endif
