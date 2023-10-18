#include "dc.h"
#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct graphd {
  int nvalues;
  long long *values;
  int ix;
  long long _value_prev;
};

struct graphd *graphd_new(const int nvalues) {
  struct graphd *self = calloc(sizeof(struct graphd), 1);
  if (!self) {
    printf("dgraphnew can not alloc\n");
    exit(1);
  }
  self->nvalues = nvalues;
  self->values = (long long *)calloc(nvalues, sizeof(long long));
  if (!self->values) {
    printf("graphnew can not alloc array of ints %d\n", nvalues);
    exit(1);
  }
  self->ix = 0;
  //	graphprint(g);
  return self;
}

void graphd_print(const struct graphd *self) {
  printf("dgraph:\n size %lu\n addr:%p\n nvalues: %d\n values: %p\n ix: %d\n",
         sizeof(*self), (void *)self, self->nvalues, (void *)self->values, self->ix);
}

void graphd_del(struct graphd *self) {
  free(self->values);
  free(self);
}

void graphd_add_value(struct graphd *self, const long long value) {
  long long delta = value - self->_value_prev;
  self->_value_prev = value;
  self->values[self->ix] = delta;
  self->ix++;
  if (self->ix == self->nvalues)
    self->ix = 0;
}

static long long _adjust(long long v, const int height) {
  if (v > height) {
    return height;
  }
  if (v < 0) {
    return 0;
  }
  return v;
}

void graphd_draw(struct graphd *self, struct dc *dc, const int height,
                 const long long max_value) {
  const int dc_x = dc_get_x(dc);
  const int dc_y = dc_get_y(dc);
  // draw top line
  dc_draw_line(dc, dc_x, dc_y - height, dc_x + self->nvalues, dc_y - height);
  int x = dc_x;
  // circular buffer, draw from current index to end
  for (int i = self->ix; i < self->nvalues; i++) {
    long long v = self->values[i] * height / max_value;
    v = _adjust(v, height);
    if (v == 0 && self->values[i] != 0) {
      v = 1;
    }
    dc_draw_line(dc, x, dc_y, x, dc_y - v);
    x++;
  }
  // draw from 0 to current index
  for (int i = 0; i < self->ix; i++) {
    long long v = self->values[i] * height / max_value;
    v = _adjust(v, height);
    if (v == 0 && self->values[i] != 0) {
      v = 1;
    }
    dc_draw_line(dc, x, dc_y, x, dc_y - v);
    x++;
  }
}
