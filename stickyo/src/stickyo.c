#include <gtk/gtk.h>

#define WIDTH 200
#define HEIGHT 200

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "stickyo");
  gtk_window_set_default_size(GTK_WINDOW(window), WIDTH, HEIGHT);

  GtkWidget *text_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);

  const gchar *text = g_getenv("STICKYO_TEXT");

  // Set the text of the text view
  if (text != NULL) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, text, -1);
  }

  // GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  // gtk_text_buffer_set_text(buffer, "Hello World", -1);

  const char *css = "textview { font: 9pt monospace; background-color: "
                    "yellow;padding: 5pt; } textview text { background: "
                    "yellow; color: black; caret-color: black;}";

  GtkCssProvider *css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(css_provider, css);

  GdkDisplay *display = gtk_widget_get_display(window);

  gtk_style_context_add_provider_for_display(
      display, GTK_STYLE_PROVIDER(css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_window_set_child(GTK_WINDOW(window), text_view);

  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  GtkApplication *app = gtk_application_new("com.github.calint.stickyo",
                                            G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
