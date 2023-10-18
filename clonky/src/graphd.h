#ifndef GRAPHD_H
#define GRAPHD_H
#include "dc.h"
struct graphd;
struct graphd *graphd_new(const int nvalues);
void graphd_del(struct graphd *g);
void graphd_print(const struct graphd *g);
void graphd_draw(const struct graphd *g, struct dc *dc, const int height,
                const long long maxvalue);
void graphd_add_value(struct graphd *g, const long long value);
#endif
