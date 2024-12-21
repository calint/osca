#ifndef DC_H
#define DC_H

struct dc;

/*gives*/ struct dc *dc_new(const char *font_name, double font_size,
                            unsigned line_height, int top_y, unsigned width,
                            unsigned hr_pixels_before, unsigned hr_pixels_after,
                            unsigned align);

void dc_del(/*takes*/ struct dc *self);

// clears the draw area and resets the cursor to the top left corner
void dc_clear(struct dc *self);

void dc_draw_line(const struct dc *self, int x0, int y0, int x1, int y1);

// draws horizontal rule
void dc_draw_hr(struct dc *self);

// draws horizontal rule with custom width
void dc_draw_hr1(struct dc *self, int width);

void dc_draw_str(const struct dc *self, const char *str);

// moves the cursor to the next line
void dc_newline(struct dc *self);

// moves the cursor down by 'dy' pixels
void dc_inc_y(struct dc *self, int dy);

// flushes the display
void dc_flush(const struct dc *self);

int dc_get_x(const struct dc *self);

int dc_get_y(const struct dc *self);

#endif
