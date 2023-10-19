#ifndef DC_H
#define DC_H

struct dc;

/*give*/ struct dc *dc_new(void);

void dc_del(/*take*/ struct dc *self);

void dc_clear(struct dc *self);

void dc_draw_line(struct dc *self, int x0, int y0, int x1, int y1);

void dc_draw_hr(struct dc *self);

void dc_draw_hr1(struct dc *self, int width);

void dc_draw_str(struct dc *self, const char *str);

void dc_newline(struct dc *self);

void dc_inc_y(struct dc *self, const int dy);

void dc_flush(const struct dc *self);

int dc_get_x(const struct dc *self);

void dc_set_x(struct dc *self, const int x);

int dc_get_y(const struct dc *self);

void dc_set_y(struct dc *self, const int y);

int dc_get_width(const struct dc *self);

int dc_get_screen_width(const struct dc *self);

void dc_set_width(struct dc *self, const int width);

void dc_set_left_x(struct dc *self, const int x);

#endif
