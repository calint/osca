#include "graph.h"
#include<stdlib.h>
#include<stdio.h>
struct graph{
	int nvalues;
	long long*values;
	int ix;
};
struct graph*graphnew(int nvalues){
	struct graph*g=malloc(sizeof(struct graph));
	if(!g){
		printf("graphnew can not alloc\n");
		exit(1);
	}
	g->nvalues=nvalues;
	g->values=(long long*)calloc(nvalues,sizeof(long long));
	if(!g->values){
		puts("graphnew can not alloc array of ints %d\n");
		exit(1);
	}
	g->ix=0;
	//	graphprint(g);
	return g;
}
void graphprint(const struct graph*g){
	printf("graph:\n size %lu\n addr:%p\n nvalues: %d\n values: %p\n ix: %d\n",sizeof(*g),(void*)g,g->nvalues,(void*)g->values,g->ix);
}
void graphdel(struct graph*g){
	free(g->values);
	free(g);
}
void graphaddvalue(struct graph*g,const long long value){
	//	printf("graphaddvalue %x %d   %d\n",g->values,g->ix,value);
	g->values[g->ix]=value;
	g->ix++;
	if(g->ix==g->nvalues)
		g->ix=0;
}
void graphdraw(const struct graph*g,struct dc*dc,const int ysclshft){
	const int x=dcxget(dc);
	const int y=dcyget(dc);
	dcdrwline(dc,x,y-(100>>ysclshft),x+g->nvalues,y-(100>>ysclshft));
	int i=g->ix;
	int xx=x;
	while(i<g->nvalues){
		long long v=g->values[i]>>ysclshft;
		if(v==0&&g->values[i]!=0)
			v=1;
		dcdrwline(dc,xx,y,xx,y-v);
		xx++;
		i++;
	}
	i=0;
	while(i<g->ix){
		long long v=g->values[i]>>ysclshft;
		if(v==0&&g->values[i]!=0)
			v=1;
		dcdrwline(dc,xx,y,xx,y-v);
		xx++;
		i++;
	}
}
static long long _adjust(long long v,const int height){
	if(v>height)
		v=height;
	if(v<0)
		v=0;
	return v;
}
void graphdraw2(const struct graph*g,struct dc*dc,const int height,const long long maxvalue){
	if(maxvalue==0)
		return;
	const int x=dcxget(dc);
	const int y=dcyget(dc);
	dcdrwline(dc,x,y-height,x+g->nvalues,y-height);
	int i=g->ix;
	int xx=x;
	while(i<g->nvalues){
		long long v=g->values[i]*height/maxvalue;
		if(v==0&&g->values[i]!=0)
			v=1;
		v=_adjust(v,height);
		dcdrwline(dc,xx,y,xx,y-v);
		xx++;
		i++;
	}
	i=0;
	while(i<g->ix){
		long long v=g->values[i]*height/maxvalue;
		if(v==0&&g->values[i]!=0)
			v=1;
		v=_adjust(v,height);
		dcdrwline(dc,xx,y,xx,y-v);
		xx++;
		i++;
	}
}
