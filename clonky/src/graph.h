#ifndef GRAPH_H
#define GRAPH_H

struct dc;
struct graph;

struct /*give*/ graph *graph_new(int nvalues);

void graph_del(/*take*/ struct graph *self);

void graph_print(const struct graph *self);

void graph_draw(const struct graph *self, struct dc *dc,
                int y_value_shift_right);

void graph_draw2(const struct graph *self, struct dc *dc, int height,
                 long long max_value);

void graph_add_value(struct graph *self, long long value);

#endif
