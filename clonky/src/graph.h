#ifndef GRAPH_H
#define GRAPH_H

struct dc;
struct graph;

/*gives*/ struct graph *graph_new(unsigned nvalues);

void graph_del(/*takes*/ struct graph *self);

void graph_draw(const struct graph *self, struct dc *dc, int height,
                long long max_value);

void graph_add_value(struct graph *self, long long value);

#endif
