//
// reviewed: 2025-02-24
//
#ifndef GRAPHD_H
#define GRAPHD_H

#include "dc.h"
#include <stdint.h>

struct graphd;

/*gives*/ struct graphd* graphd_new(uint32_t nvalues);

void graphd_del(/*takes*/ struct graphd* self);

void graphd_draw(const struct graphd* self, struct dc* dc, uint32_t height,
                 uint64_t max_value);

void graphd_add_value(struct graphd* self, uint64_t value);

#endif
