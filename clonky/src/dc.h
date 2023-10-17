#ifndef DC_H
#define DC_H
struct dc;
struct dc*dcnew();
void dcdel(struct dc*d);
void dcclrbw(struct dc*d);
void dcdrwline(struct dc*d,const int x0,const int y0,const int x1,const int y1);
void dcdrwhr(struct dc*d);
void dcdrwhr1(struct dc*d,const int w);
void dcdrwstr(struct dc*d,const char*s);
void dccr(struct dc*d);
void dcyinc(struct dc*d,const int dy);
void dcflush(const struct dc*d);
int dcxget(const struct dc*d);
void dcxset(struct dc*d,const int x);
int dcyget(const struct dc*d);
void dcyset(struct dc*d,const int px);
int dcwget(const struct dc*d);
int dcwscrget(const struct dc*d);
void dcwset(struct dc*d,const int width);
void dcxlftset(struct dc*d,const int x);
#endif
