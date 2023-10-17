#include<cairo.h>
#include<cairo-xlib.h>
#include<X11/Xlib.h>
#include<stdio.h>
int main(int argc,char**argv){
	if(argc<2){
		puts("snap <arg>.png");
		return -1;
	}
	*argv++;
	const char*fn=*argv++;
//	puts(fn);
	Display*disp=XOpenDisplay(NULL);
	int scr=DefaultScreen(disp);
	Window root=DefaultRootWindow(disp);
	cairo_surface_t*surface=cairo_xlib_surface_create(disp,root,DefaultVisual(disp,scr),DisplayWidth(disp,scr),DisplayHeight(disp,scr));
	cairo_surface_write_to_png(surface,fn);
	cairo_surface_destroy(surface);
	return 0;
}
