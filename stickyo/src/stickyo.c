#include<gtk/gtk.h>
#define PADDING 5
#define BACKGROUND_COLOR "yellow"
#define FONT "monospace 9"
#define WIDTH 200
#define HEIGHT 200
void on_window_destroy(/*GtkWidget*widget,gpointer data*/){
	gtk_main_quit();
}
int main(int argc,char**argv){
	gtk_init(&argc,&argv);
	GdkColor color;
	gdk_color_parse(BACKGROUND_COLOR,&color);
	GtkWidget*window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_modify_bg(window,GTK_STATE_NORMAL,&color);
	gtk_window_set_title(GTK_WINDOW(window),"sticky");
	gtk_window_set_default_size(GTK_WINDOW(window),WIDTH,HEIGHT);//? magicnum
	//? set position at pointer
	g_signal_connect(G_OBJECT(window),"destroy",G_CALLBACK (on_window_destroy),NULL);
	GtkWidget*vbox=gtk_vbox_new(FALSE,2);
	gtk_container_add(GTK_CONTAINER(window),vbox);
	GtkWidget*text_view=gtk_text_view_new();
	gtk_widget_modify_base(text_view,GTK_STATE_NORMAL,&color);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view),PADDING);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view),PADDING);
	PangoFontDescription*font_desc;
	font_desc = pango_font_description_from_string(FONT);
	gtk_widget_modify_font(text_view,font_desc);
	pango_font_description_free(font_desc);
	gtk_box_pack_start(GTK_BOX(vbox),text_view,TRUE,TRUE,PADDING);
	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}









/*
void on_button_clicked(GtkWidget*button,GtkTextBuffer*buffer){
	GtkTextIter start,end;
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	gchar*text=gtk_text_buffer_get_text(buffer,&start,&end,FALSE);
	g_print("%s",text);
	g_free(text);
	gtk_main_quit();
}
*/

/*
	
//	GtkTextBuffer*buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
//	gtk_text_buffer_set_text(buffer,"Hello Text View!",-1);

	GdkColor red={0,0xffff,0x0000,0x0000};
	gtk_widget_modify_bg(text_view,GTK_STATE_ACTIVE,&red);
*/


