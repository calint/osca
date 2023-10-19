#include <gtk/gtk.h>

#define WIDTH 200
#define HEIGHT 200

static void on_window_destroy(GtkWidget *widget, gpointer data) {
  gtk_main_quit();
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "sticky");
  gtk_window_set_default_size(GTK_WINDOW(window), WIDTH, HEIGHT);

  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy),
                   NULL);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  GtkWidget *text_view = gtk_text_view_new();

  if (argc > 1) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, argv[1], -1);
  }

  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);

  gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);

  const char *css = "textview { font: 9pt monospace; background-color: yellow; "
                    "padding: 5pt; } textview text { "
                    "background: yellow; color: black; caret-color: black; }";
  GtkCssProvider *css_provider = gtk_css_provider_new();
  GError *error = NULL;
  gtk_css_provider_load_from_data(css_provider, css, -1, &error);

  if (error) {
    g_print("error loading css: %s\n", error->message);
    g_error_free(error);
  }

  gtk_style_context_add_provider(gtk_widget_get_style_context(text_view),
                                 GTK_STYLE_PROVIDER(css_provider),
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css_provider);

  gtk_widget_show_all(window);
  gtk_main();
  return 0;
}
