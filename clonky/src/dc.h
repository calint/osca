#ifndef DC_H
#define DC_H

struct dc;

/*gives*/ struct dc *dc_new(const char *font_name, double font_size,
                            int line_height);

void dc_del(/*takes*/ struct dc *self);

void dc_clear(struct dc *self);

void dc_draw_line(struct dc *self, int x0, int y0, int x1, int y1);

void dc_draw_hr(struct dc *self);

void dc_draw_hr1(struct dc *self, int width);

void dc_draw_str(struct dc *self, const char *str);

void dc_newline(struct dc *self);

void dc_inc_y(struct dc *self, int dy);

void dc_flush(const struct dc *self);

int dc_get_x(const struct dc *self);

void dc_set_x(struct dc *self, int x);

int dc_get_y(const struct dc *self);

void dc_set_y(struct dc *self, int y);

unsigned dc_get_width(const struct dc *self);

unsigned dc_get_screen_width(const struct dc *self);

void dc_set_width(struct dc *self, unsigned width);

void dc_set_left_x(struct dc *self, int x);

#endif
