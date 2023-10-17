#include "graph.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
struct graphd{
	int nvalues;
	long long*values;
	int ix;
	long long _prvvalue;
};
struct graphd*graphdnew(const int nvalues){
	struct graphd*g=calloc(sizeof(struct graphd),1);
	if(!g){
		printf("dgraphnew can not alloc\n");
		exit(1);
	}
	g->nvalues=nvalues;
	g->values=(long long*)calloc(nvalues,sizeof(long long));
	if(!g->values){
		printf("graphnew can not alloc array of ints %d\n",nvalues);
		exit(1);
	}
	g->ix=0;
	//	graphprint(g);
	return g;
}
void graphdprint(const struct graphd*g){
	printf("dgraph:\n size %lu\n addr:%p\n nvalues: %d\n values: %p\n ix: %d\n",sizeof(*g),(void*)g,g->nvalues,(void*)g->values,g->ix);
}
void graphddel(struct graphd*g){
	free(g->values);
	free(g);
}
void graphdaddvalue(struct graphd*g,const long long value){
	long long dv=value-g->_prvvalue;
	g->_prvvalue=value;
	g->values[g->ix]=dv;
	g->ix++;
	if(g->ix==g->nvalues)
		g->ix=0;
}
static long long _adjust(long long v,const int height){
	if(v>height)
		v=height;
	if(v<0)
		v=0;
	return v;
}
void graphddraw(struct graphd*g,struct dc*dc,const int height,const long long maxvalue){
	const int x=dcxget(dc);
	const int y=dcyget(dc);
	dcdrwline(dc,x,y-height,x+g->nvalues,y-height);
	int i=g->ix;
	int xx=x;
	while(i<g->nvalues){
		long long v=g->values[i]*height/maxvalue;
		v=_adjust(v,height);
		if(v==0&&g->values[i]!=0)
			v=1;
		dcdrwline(dc,xx,y,xx,y-v);
		xx++;
		i++;
	}
	i=0;
	while(i<g->ix){
		long long v=g->values[i]*height/maxvalue;
		v=_adjust(v,height);
		if(v==0&&g->values[i]!=0)
			v=1;
		dcdrwline(dc,xx,y,xx,y-v);
		xx++;
		i++;
	}
}
