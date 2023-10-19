#include "graph.h"
#include "dc.h"
#include <stdio.h>
#include <stdlib.h>

struct graph {
  unsigned nvalues;
  long long *values;
  unsigned ix;
};

struct /*give*/ graph *graph_new(unsigned nvalues) {
  struct graph *self = malloc(sizeof(struct graph));
  if (!self) {
    printf("graphnew can not alloc\n");
    exit(1);
  }
  self->nvalues = nvalues;
  self->values = (long long *)calloc(nvalues, sizeof(long long));
  if (!self->values) {
    puts("graphnew can not alloc array of ints %d\n");
    exit(1);
  }
  self->ix = 0;
  return self;
}

void graph_del(/*take*/ struct graph *self) {
  free(self->values);
  free(self);
}

void graph_add_value(struct graph *self, const long long value) {
  self->values[self->ix] = value;
  self->ix++;
  if (self->ix == self->nvalues)
    self->ix = 0;
}

void graph_draw(const struct graph *self, struct dc *dc,
                const int y_value_shift_right) {
  const int dc_x = dc_get_x(dc);
  const int dc_y = dc_get_y(dc);
  dc_draw_line(dc, dc_x, dc_y - (100 >> y_value_shift_right),
               dc_x + (int)self->nvalues, dc_y - (100 >> y_value_shift_right));
  int x = dc_x;
  // circular buffer, draw from current index to end
  for (unsigned i = self->ix; i < self->nvalues; i++) {
    long long v = self->values[i] >> y_value_shift_right;
    if (v == 0 && self->values[i] != 0) {
      v = 1;
    }
    dc_draw_line(dc, x, dc_y, x, dc_y - (int)v);
    x++;
  }
  // draw from 0 to current index
  for (unsigned i = 0; i < self->ix; i++) {
    long long v = self->values[i] >> y_value_shift_right;
    if (v == 0 && self->values[i] != 0)
      v = 1;
    dc_draw_line(dc, x, dc_y, x, dc_y - (int)v);
    x++;
  }
}

static long long graphd_cap_value(long long v, const int height) {
  if (v > height)
    v = height;
  if (v < 0)
    v = 0;
  return v;
}

void graph_draw2(const struct graph *self, struct dc *dc, const int height,
                 const long long max_value) {
  if (max_value == 0) {
    return;
  }
  const int dc_x = dc_get_x(dc);
  const int dc_y = dc_get_y(dc);
  // draw top line
  dc_draw_line(dc, dc_x, dc_y - height, dc_x + (int)self->nvalues,
               dc_y - height);
  int x = dc_x;
  // circular buffer, draw from current index to end
  for (unsigned i = self->ix; i < self->nvalues; i++) {
    long long v = self->values[i] * height / max_value;
    if (v == 0 && self->values[i] != 0)
      v = 1;
    v = graphd_cap_value(v, height);
    dc_draw_line(dc, x, dc_y, x, dc_y - (int)v);
    x++;
  }
  // draw from 0 to current index
  for (unsigned i = 0; i < self->ix; i++) {
    long long v = self->values[i] * height / max_value;
    if (v == 0 && self->values[i] != 0) {
      v = 1;
    }
    v = graphd_cap_value(v, height);
    dc_draw_line(dc, x, dc_y, x, dc_y - (int)v);
    x++;
  }
}
