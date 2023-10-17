#ifndef GRAPH_H
#define GRAPH_H
#include"dc.h"
struct graph;
struct graph*graphnew(const int nvalues);
void graphdel(struct graph*g);
void graphprint(const struct graph*g);
void graphdraw(const struct graph*g,struct dc*dc,const int ysclshft);
void graphdraw2(const struct graph*g,struct dc*dc,const int height,const long long maxvalue);
void graphaddvalue(struct graph*g,const long long value);
#endif
