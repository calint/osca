#ifndef GRAPHD_H
#define GRAPHD_H

struct dc;
struct graphd;

struct /*give*/ graphd *graphd_new(int nvalues);

void graphd_del(/*take*/ struct graphd *self);

void graphd_print(const struct graphd *self);

void graphd_draw(const struct graphd *self, struct dc *dc, int height,
                 long long max_value);

void graphd_add_value(struct graphd *self, long long value);

#endif
