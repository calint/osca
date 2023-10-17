#ifndef GRAPHD_H
#define GRAPHD_H
#include"dc.h"
struct graphd;
struct graphd*graphdnew(const int nvalues);
void graphddel(struct graphd*g);
void graphdprint(const struct graphd*g);
void graphddraw(const struct graphd*g,struct dc*dc,const int height,const long long maxvalue);
void graphdaddvalue(struct graphd*g,const long long value);
#endif
