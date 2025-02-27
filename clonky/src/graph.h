//
// reviewed: 2025-02-24
//
#ifndef GRAPH_H
#define GRAPH_H

#include "dc.h"
#include <stdint.h>

struct graph;

/*gives*/ struct graph *graph_new(uint32_t nvalues);

void graph_del(/*takes*/ struct graph *self);

void graph_draw(const struct graph *self, struct dc *dc, uint32_t height,
                uint64_t max_value);

void graph_add_value(struct graph *self, uint64_t value);

#endif
