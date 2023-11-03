#include <gtk/gtk.h>

#define WINDOW_WIDTH 200
#define WINDOW_HEIGHT 200

static void command_line(GApplication *app, GApplicationCommandLine *cmdline) {
  gint argc = 0;
  gchar **argv = g_application_command_line_get_arguments(cmdline, &argc);

  GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
  gtk_window_set_title(GTK_WINDOW(window), "sticky");
  gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);

  GtkWidget *text_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);

  if (argc > 1) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, argv[1], -1);
  }

  const char *css = "textview {"
                    "  padding: 5pt;"
                    "  font: 9pt monospace;"
                    "  background-color: yellow;"
                    "  color: black;"
                    "  caret-color: black;"
                    "}";

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
  GtkApplication *application = gtk_application_new(
      "com.github.calint.osca.stickyo", G_APPLICATION_HANDLES_COMMAND_LINE);

  g_signal_connect(application, "command-line", G_CALLBACK(command_line), NULL);

  int status = g_application_run(G_APPLICATION(application), argc, argv);

  g_object_unref(application);
  return status;
}
