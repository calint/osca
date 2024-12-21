#include "graphd.h"
#include "dc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct graphd {
  unsigned nvalues;
  long long *values;
  unsigned ix;
  long long _value_prev;
};

struct /*give*/ graphd *graphd_new(const unsigned nvalues) {
  struct graphd *self = calloc(1, sizeof(struct graphd));
  if (!self) {
    printf("!!! graphd_new cannot alloc\n");
    exit(1);
  }
  self->nvalues = nvalues;
  self->values = (long long *)calloc(nvalues, sizeof(long long));
  if (!self->values) {
    printf("!!! graphd_new cannot alloc array of size %d\n", nvalues);
    exit(1);
  }
  self->ix = 0;
  return self;
}

void graphd_del(/*take*/ struct graphd *self) {
  free(self->values);
  free(self);
}

void graphd_add_value(struct graphd *self, const long long value) {
  long long delta = value - self->_value_prev;
  self->_value_prev = value;
  self->values[self->ix] = delta;
  self->ix++;
  if (self->ix == self->nvalues) {
    self->ix = 0;
  }
}

static long long cap_value(long long v, const int height) {
  if (v > height) {
    return height;
  }
  if (v < 0) {
    return 0;
  }
  return v;
}

void graphd_draw(const struct graphd *self, struct dc *dc, const int height,
                 const long long max_value) {
  const int dc_x = dc_get_x(dc);
  const int dc_y = dc_get_y(dc);
  // draw top line
  dc_draw_line(dc, dc_x, dc_y - height, dc_x + (int)self->nvalues,
               dc_y - height);
  int x = dc_x;
  // circular buffer, draw from current index to end
  for (unsigned i = self->ix; i < self->nvalues; i++) {
    const long long value =
        cap_value(self->values[i] * height / max_value, height);
    dc_draw_line(dc, x, dc_y, x, dc_y - (int)value);
    x++;
  }
  // draw from 0 to current index
  for (unsigned i = 0; i < self->ix; i++) {
    const long long value =
        cap_value(self->values[i] * height / max_value, height);
    dc_draw_line(dc, x, dc_y, x, dc_y - (int)value);
    x++;
  }
}
