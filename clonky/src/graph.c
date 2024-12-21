#include "graph.h"
#include "dc.h"
#include <stdio.h>
#include <stdlib.h>

struct graph {
  unsigned nvalues;
  long long *values;
  unsigned ix;
};

struct /*gives*/ graph *graph_new(unsigned nvalues) {
  struct graph *self = calloc(1, sizeof(struct graph));
  if (!self) {
    printf("!!! graph_new cannot alloc\n");
    exit(1);
  }
  self->nvalues = nvalues;
  self->values = (long long *)calloc(nvalues, sizeof(long long));
  if (!self->values) {
    printf("!!! graph_new cannot alloc array of size %d\n", nvalues);
    exit(1);
  }
  self->ix = 0;
  return self;
}

void graph_del(/*takes*/ struct graph *self) {
  free(self->values);
  free(self);
}

void graph_add_value(struct graph *self, const long long value) {
  self->values[self->ix] = value;
  self->ix++;
  if (self->ix == self->nvalues) {
    self->ix = 0;
  }
}

static long long cap_value(const long long value, const int height) {
  if (value > height) {
    return height;
  }
  if (value < 0) {
    return 0;
  }
  return value;
}

void graph_draw(const struct graph *self, struct dc *dc, const int height,
                const long long max_value) {
  const int dc_x = dc_get_x(dc);
  const int dc_y = dc_get_y(dc);
  // draw top line
  const int top_y = dc_y - height;
  dc_draw_line(dc, dc_x, top_y, dc_x + (int)self->nvalues, top_y);
  int x = dc_x;
  // circular buffer, draw from current index to end
  for (unsigned i = self->ix; i < self->nvalues; i++) {
    const long long value =
        cap_value(self->values[i] * height / max_value, height);
    dc_draw_line(dc, x, dc_y - (int)value, x, dc_y);
    x++;
  }
  // draw from 0 to current index
  for (unsigned i = 0; i < self->ix; i++) {
    const long long value =
        cap_value(self->values[i] * height / max_value, height);
    dc_draw_line(dc, x, dc_y - (int)value, x, dc_y);
    x++;
  }
}
