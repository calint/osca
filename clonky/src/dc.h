#ifndef DC_H
#define DC_H
struct dc;
struct dc *dc_new();
void dc_del(struct dc *d);
void dc_clear(struct dc *d);
void dc_draw_line(struct dc *d, const int x0, const int y0, const int x1,
                  const int y1);
void dc_draw_hr(struct dc *d);
void dc_draw_hr1(struct dc *d, const int w);
void dc_draw_str(struct dc *d, const char *s);
void dc_newline(struct dc *d);
void dc_inc_y(struct dc *d, const int dy);
void dc_flush(const struct dc *d);
int dc_get_x(const struct dc *d);
void dc_set_x(struct dc *d, const int x);
int dc_get_y(const struct dc *d);
void dc_set_y(struct dc *d, const int px);
int dc_get_width(const struct dc *d);
int dc_get_screen_width(const struct dc *d);
void dc_set_width(struct dc *d, const int width);
void dc_set_left_x(struct dc *d, const int x);
#endif
