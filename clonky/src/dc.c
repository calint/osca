#include<X11/Xft/Xft.h>
#include<locale.h>
#include"dc.h"
struct dc{
	int scr;
	Display*dpy;
	Window win;
	GC gc;
	Colormap cmap;
	XftFont*font;
	XftDraw*draw;
	XftColor color;
	unsigned int dpy_width;
	unsigned int dpy_height;
	unsigned int width;
	int xlft;
	int ytop;
	int ddoty;
	int doty;
	int dotx;
	XRenderColor rendcol;
};
struct dc*dcnew(){
	struct dc*this=calloc(sizeof(struct dc),1);
	setlocale(LC_ALL,"");
	this->dpy=XOpenDisplay(NULL);
	if(!this->dpy){
		fprintf(stderr,"!!! could not open display\n");
		return 0;
	}
	this->scr=DefaultScreen(this->dpy);
	this->dpy_width=DisplayWidth(this->dpy,this->scr);
	this->dpy_height=DisplayHeight(this->dpy,this->scr);
	this->width=this->dpy_width;
	this->xlft=0;
	this->ytop=0;
	this->dotx=0;
	this->doty=0;
	this->ddoty=10;
	this->win=RootWindow(this->dpy,this->scr);
	this->gc=XCreateGC(this->dpy,this->win,0,NULL);
	this->cmap=DefaultColormap(this->dpy,this->scr);
	this->font=XftFontOpen(this->dpy,this->scr,XFT_FAMILY,XftTypeString,"sans",XFT_SIZE,XftTypeDouble,7.0,NULL);
	this->draw=XftDrawCreate(this->dpy,this->win,DefaultVisual(this->dpy,this->scr),this->cmap);
	XRenderColor xrendcolwhite={0xffff,0xffff,0xffff,0xffff};
	this->rendcol=xrendcolwhite;
	XftColorAllocValue(this->dpy,DefaultVisual(this->dpy,this->scr),this->cmap,&this->rendcol,&this->color);
	return this;
}
void dcdel(struct dc*this){
	XftDrawDestroy(this->draw);
	XftFontClose(this->dpy,this->font);
	XFree(this->gc);
	free(this);
}
void dcclrbw(struct dc*this){
	XSetForeground(this->dpy,this->gc,BlackPixel(this->dpy,this->scr));
	XFillRectangle(this->dpy,this->win,this->gc,this->xlft,this->ytop,this->width,this->dpy_height);
	XSetForeground(this->dpy,this->gc,WhitePixel(this->dpy,this->scr));
}
void dcdrwline(struct dc*this,const int x0,const int y0,const int x1,const int y1){XDrawLine(this->dpy,this->win,this->gc,this->xlft+x0,this->ytop+y0,this->xlft+x1,this->ytop+y1);}
void dccr(struct dc*this){this->doty+=this->ddoty;}
void dcdrwstr(struct dc*this,const char*s){XftDrawStringUtf8(this->draw,&this->color,this->font,this->xlft+this->dotx,this->ytop+this->doty,(FcChar8*)s,strlen(s));}
void dcdrwhr(struct dc*this){this->doty+=3;XDrawLine(this->dpy,this->win,this->gc,this->xlft,this->doty,this->xlft+this->width,this->doty);}
void dcdrwhr1(struct dc*this,const int w){this->doty+=3;XDrawLine(this->dpy,this->win,this->gc,this->xlft,this->doty,this->xlft+w,this->doty);}
void dcyinc(struct dc*this,const int dy){this->doty+=dy;}
void dcflush(const struct dc*this){XFlush(this->dpy);}
int dcxget(const struct dc*this){return this->dotx;}
void dcxset(struct dc*this,const int x){this->dotx=x;}
void dcxlftset(struct dc*this,const int x){this->xlft=x;}
int dcyget(const struct dc*this){return this->doty;}
void dcyset(struct dc*this,const int y){this->doty=y;}
int dcwget(const struct dc*this){return this->width;}
void dcwset(struct dc*this,const int width){this->width=width;}
int dcwscrget(const struct dc*this){return this->dpy_width;}
