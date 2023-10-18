#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
struct graph {
  int nvalues;
  long long *values;
  int ix;
};
struct graph *graph_new(int nvalues) {
  struct graph *g = malloc(sizeof(struct graph));
  if (!g) {
    printf("graphnew can not alloc\n");
    exit(1);
  }
  g->nvalues = nvalues;
  g->values = (long long *)calloc(nvalues, sizeof(long long));
  if (!g->values) {
    puts("graphnew can not alloc array of ints %d\n");
    exit(1);
  }
  g->ix = 0;
  //	graphprint(g);
  return g;
}
void graph_print(const struct graph *g) {
  printf("graph:\n size %lu\n addr:%p\n nvalues: %d\n values: %p\n ix: %d\n",
         sizeof(*g), (void *)g, g->nvalues, (void *)g->values, g->ix);
}
void graph_del(struct graph *g) {
  free(g->values);
  free(g);
}
void graph_add_value(struct graph *g, const long long value) {
  //	printf("graphaddvalue %x %d   %d\n",g->values,g->ix,value);
  g->values[g->ix] = value;
  g->ix++;
  if (g->ix == g->nvalues)
    g->ix = 0;
}
void graph_draw(const struct graph *g, struct dc *dc, const int ysclshft) {
  const int x = dc_get_x(dc);
  const int y = dc_get_y(dc);
  dc_draw_line(dc, x, y - (100 >> ysclshft), x + g->nvalues,
            y - (100 >> ysclshft));
  int i = g->ix;
  int xx = x;
  while (i < g->nvalues) {
    long long v = g->values[i] >> ysclshft;
    if (v == 0 && g->values[i] != 0)
      v = 1;
    dc_draw_line(dc, xx, y, xx, y - v);
    xx++;
    i++;
  }
  i = 0;
  while (i < g->ix) {
    long long v = g->values[i] >> ysclshft;
    if (v == 0 && g->values[i] != 0)
      v = 1;
    dc_draw_line(dc, xx, y, xx, y - v);
    xx++;
    i++;
  }
}
static long long _adjust(long long v, const int height) {
  if (v > height)
    v = height;
  if (v < 0)
    v = 0;
  return v;
}
void graph_draw2(const struct graph *g, struct dc *dc, const int height,
                const long long maxvalue) {
  if (maxvalue == 0)
    return;
  const int x = dc_get_x(dc);
  const int y = dc_get_y(dc);
  dc_draw_line(dc, x, y - height, x + g->nvalues, y - height);
  int i = g->ix;
  int xx = x;
  while (i < g->nvalues) {
    long long v = g->values[i] * height / maxvalue;
    if (v == 0 && g->values[i] != 0)
      v = 1;
    v = _adjust(v, height);
    dc_draw_line(dc, xx, y, xx, y - v);
    xx++;
    i++;
  }
  i = 0;
  while (i < g->ix) {
    long long v = g->values[i] * height / maxvalue;
    if (v == 0 && g->values[i] != 0)
      v = 1;
    v = _adjust(v, height);
    dc_draw_line(dc, xx, y, xx, y - v);
    xx++;
    i++;
  }
}
