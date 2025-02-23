#ifndef GRAPHD_H
#define GRAPHD_H

#include "dc.h"

struct graphd;

/*gives*/ struct graphd *graphd_new(unsigned nvalues);

void graphd_del(/*takes*/ struct graphd *self);

void graphd_draw(const struct graphd *self, struct dc *dc, int height,
                 long long max_value);

void graphd_add_value(struct graphd *self, long long value);

#endif
