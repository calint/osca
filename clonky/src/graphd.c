#include "graphd.h"
#include "dc.h"
#include <stdio.h>
#include <stdlib.h>

struct graphd {
  uint64_t *values;
  uint64_t _value_prev;
  uint32_t nvalues;
  uint32_t ix;
};

/*gives*/ struct graphd *graphd_new(const uint32_t nvalues) {
  struct graphd *self = calloc(1, sizeof(struct graphd));
  if (!self) {
    printf("!!! graphd_new cannot alloc\n");
    exit(1);
  }
  self->nvalues = nvalues;
  self->values = (uint64_t *)calloc(nvalues, sizeof(uint64_t));
  if (!self->values) {
    printf("!!! graphd_new cannot alloc array of size %d\n", nvalues);
    exit(1);
  }
  self->ix = 0;
  return self;
}

void graphd_del(/*takes*/ struct graphd *self) {
  free(self->values);
  free(self);
}

void graphd_add_value(struct graphd *self, const uint64_t value) {
  uint64_t delta = value - self->_value_prev;
  self->_value_prev = value;
  self->values[self->ix] = delta;
  self->ix++;
  if (self->ix == self->nvalues) {
    self->ix = 0;
  }
}

static uint64_t cap_value(const uint64_t v, const uint32_t height) {
  if (v > height) {
    return height;
  }
  return v;
}

void graphd_draw(const struct graphd *self, struct dc *dc,
                 const uint32_t height, const uint64_t max_value) {
  const int32_t dc_x = dc_get_x(dc);
  const int32_t dc_y = dc_get_y(dc);
  // draw top line
  const int32_t top_y = dc_y - (int32_t)height;
  dc_draw_line(dc, dc_x, top_y, dc_x + (int32_t)self->nvalues, top_y);
  int32_t x = dc_x;
  // circular buffer, draw from current index to end
  for (uint32_t i = self->ix; i < self->nvalues; i++) {
    const uint64_t value =
        cap_value(self->values[i] * height / max_value, height);
    dc_draw_line(dc, x, dc_y - (int32_t)value, x, dc_y);
    x++;
  }
  // draw from 0 to current index
  for (uint32_t i = 0; i < self->ix; i++) {
    const uint64_t value =
        cap_value(self->values[i] * height / max_value, height);
    dc_draw_line(dc, x, dc_y - (int32_t)value, x, dc_y);
    x++;
  }
}
