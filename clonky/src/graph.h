#ifndef GRAPH_H
#define GRAPH_H
#include "dc.h"
struct graph;
struct graph *graph_new(const int nvalues);
void graph_del(struct graph *g);
void graph_print(const struct graph *g);
void graph_draw(const struct graph *g, struct dc *dc, const int ysclshft);
void graph_draw2(const struct graph *g, struct dc *dc, const int height,
                const long long maxvalue);
void graph_add_value(struct graph *g, const long long value);
#endif
