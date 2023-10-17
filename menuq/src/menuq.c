#define APP "menuq"
#include<X11/Xlib.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<X11/keysym.h>
#include <X11/Xutil.h>
#define BORDER_WIDTH 1
int main(){
//	puts(APP);
	Display*dpy=XOpenDisplay(NULL);
	if(!dpy){
		//fprintf(stderr, "!!! could not open display\n");
		return 1;
	}
	const int scr=DefaultScreen(dpy);
	const int screen_width=DisplayWidth(dpy,scr);
	const int win_height=23;
	const Window win=XCreateSimpleWindow(dpy,RootWindow(dpy,scr),-BORDER_WIDTH,0,screen_width,win_height,0,BlackPixel(dpy,scr),BlackPixel(dpy,scr));
//	XSelectInput(dpy,win,ExposureMask|KeyPressMask);
	XSelectInput(dpy,win,KeyPressMask);
	XMapWindow(dpy,win);
	const GC gc=XCreateGC(dpy,win,0,NULL);
	const int x_init=(screen_width>>1)-(screen_width>>2)+(screen_width>>3);
	int x=x_init;
	const int y_init=(win_height>>1)+(win_height>>2);
	int y=y_init;
	const char cursor_str[1]="_";
	XSetForeground(dpy,gc,WhitePixel(dpy,scr));
	XDrawString(dpy,win,gc,x,y,cursor_str,1);
	char buf[32];
	int bufi=0;
	char*bufp=buf;
	*bufp=0;
	const int char_width=7;
	const int char_y_wiggle=3;
	const int char_y_wiggle_up=-1;
	const int options_start_x=(screen_width>>1)+(screen_width>>2);
	int go=1;
	XEvent e;
	while(go){
		XNextEvent(dpy,&e);
		switch(e.type){
		default:
			break;
		case Expose:
//			XSetForeground(dpy,gc,BlackPixel(dpy,scr));
//			XFillRectangle(dpy,win,gc,0,0,screen_width,win_height);
//			XSetForeground(dpy,gc,WhitePixel(dpy,scr));
//			const int buflen=strlen(buf);
//			printf("%d   [%s]\n",buflen,buf);
//			XDrawString(dpy,win,gc,x_init,y_init,buf,buflen);
//			x=x_init+char_width*buflen;
//			XDrawString(dpy,win,gc,x,y,cursor_str,1);
			break;
		case DestroyNotify:
			go=0;
			break;
		case KeyPress:{
			int keycode=0;
			//printf("KeyPress %d %d\n",e.xbutton.button,e.xbutton.state);
			keycode=e.xkey.keycode;
			if(keycode==9){//esc
				go=0;
				bufp=buf;
				*bufp=0;
				break;
			}
			keycode=e.xkey.keycode;
			if(keycode==36){//return
				go=0;
				*bufp=0;
				break;
			}
			if(keycode==22){//backspace
				if(bufi==0)
					break;
				x-=char_width;
				XSetForeground(dpy,gc,BlackPixel(dpy,scr));
				XFillRectangle(dpy,win,gc,x,0,char_width<<1,win_height);
				XSetForeground(dpy,gc,WhitePixel(dpy,scr));
				XDrawString(dpy,win,gc,x,y,"_",1);
				bufi--;
				bufp--;
				*bufp=0;
			}else{
				// clear cursor
				XSetForeground(dpy,gc,BlackPixel(dpy,scr));
				XFillRectangle(dpy,win,gc,x,0,char_width,win_height);
				XSetForeground(dpy,gc,WhitePixel(dpy,scr));
				// get printable character
				char buffer[4];
				KeySym keysym;
				XLookupString(&e.xkey,buffer,sizeof buffer,&keysym,NULL);
				if(!buffer[0])// printable character ?
					break;
				XDrawString(dpy,win,gc,x,y,buffer,1);
				x+=char_width;
				*bufp++=buffer[0];
				bufi++;
				if((bufi+1)>=(int)sizeof buf)// buffer full ?
					go=0;
				y+=char_y_wiggle_up+rand()%char_y_wiggle;
				XDrawString(dpy,win,gc,x,y,cursor_str,1);
			}
			XFillRectangle(dpy,win,gc,options_start_x,0,screen_width-options_start_x-1,win_height-1);
			break;
			}
		}
	}
	XFreeGC(dpy,gc);
	XCloseDisplay(dpy);
	if(!*buf)return 0;//empty string
	strcat(buf,"&");
	return system(buf);
}
