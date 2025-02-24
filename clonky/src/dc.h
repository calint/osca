#ifndef DC_H
#define DC_H

#include <stdint.h>

struct dc;

/*gives*/ struct dc *dc_new(const char *font_name, double font_size,
                            uint32_t line_height, int32_t margin_top,
                            uint32_t width, uint32_t pixels_before_hr,
                            uint32_t pixels_after_hr, uint32_t align);

void dc_del(/*takes*/ struct dc *self);

// clears the draw area and resets the cursor to the top left corner
void dc_clear(struct dc *self);

void dc_draw_line(const struct dc *self, int32_t x0, int32_t y0, int32_t x1,
                  int32_t y1);

// draws horizontal rule
void dc_draw_hr(struct dc *self);

// draws horizontal rule with custom width
void dc_draw_hr1(struct dc *self, uint32_t width);

void dc_draw_str(const struct dc *self, const char *str);

// moves the cursor to the next line
void dc_newline(struct dc *self);

// moves the cursor down by 'dy' pixels
void dc_inc_y(struct dc *self, uint32_t dy);

// flushes the display
void dc_flush(const struct dc *self);

int32_t dc_get_x(const struct dc *self);

int32_t dc_get_y(const struct dc *self);

#endif
