#include <assert.h>
#include <gtk/gtk.h>

#include "window.h"
#include "greeter.h"

#include "config.h"

struct SimpleGreeter *greeter = NULL;

static char* command = NULL;
static char* style = NULL;

#ifdef LAYER_SHELL
static gboolean use_layer_shell = FALSE;
#endif

static GOptionEntry entries[] = {
#ifdef LAYER_SHELL
  { "layer-shell", 'l', 0, G_OPTION_ARG_NONE, &use_layer_shell, "Use layer shell", NULL},
#endif
  { "command", 'c', 0, G_OPTION_ARG_STRING, &command, "Command to run", "sway"},
  { "style", 's', 0, G_OPTION_ARG_FILENAME, &style, "CSS style to use", NULL },
  { NULL }
};

#ifdef LAYER_SHELL
static void 
reload_outputs() 
{
   GdkDisplay *display = gdk_display_get_default();

   // Make note of all existing windows
   GArray *dead_windows = g_array_new(FALSE, TRUE, sizeof(struct Window*));
   for (guint idx = 0; idx < greeter->windows->len; idx++) {
      struct Window *ctx = g_array_index(greeter->windows, struct Window*, idx);
      g_array_append_val(dead_windows, ctx);
   }

   // Go through all monitors
   for (int i = 0; i < gdk_display_get_n_monitors(display); i++) {
      GdkMonitor *monitor = gdk_display_get_monitor(display, i);
      struct Window *w = greeter_window_by_monitor(greeter, monitor);
      if (w != NULL) {
         // We already have this monitor, remove from dead_windows list
         for (guint ydx = 0; ydx < dead_windows->len; ydx++) {
            if (w == g_array_index(dead_windows, struct Window*, ydx)) {
               g_array_remove_index_fast(dead_windows, ydx);
               break;
            }
         }
      } else 
         create_window(monitor);
   }

   // Remove all windows left behind
   for (guint idx = 0; idx < dead_windows->len; idx++) {
      struct Window *w = g_array_index(dead_windows, struct Window*, idx);
      gtk_widget_destroy(w->window);
      if (greeter->focused_window == w)
         greeter->focused_window = NULL;
   }

   for (guint idx = 0; idx < greeter->windows->len; idx++) {
      struct Window *win = g_array_index(greeter->windows, struct Window*, idx);
      window_configure(win);
   }

   g_array_unref(dead_windows);
}

static void 
monitors_changed(GdkDisplay *display, GdkMonitor *monitor) 
{
   reload_outputs();
}

static gboolean 
setup_layer_shell() 
{
   if (greeter->use_layer_shell) {
      reload_outputs();
      GdkDisplay *display = gdk_display_get_default();
      g_signal_connect(display, "monitor-added", G_CALLBACK(monitors_changed), NULL);
      g_signal_connect(display, "monitor-removed", G_CALLBACK(monitors_changed), NULL);
      return TRUE;
   } else 
      return FALSE;
}
#else
static gboolean setup_layer_shell() {
   return FALSE;
}
#endif

static void 
activate(GtkApplication *app, gpointer user_data) 
{
   greeter_activate(greeter);
   if (!setup_layer_shell()) {
      struct Window *win = create_window(NULL);
      greeter_focus_window(greeter, win);
      window_configure(win);
   }
}

static void 
attach_custom_style(const char* path) 
{
   GtkCssProvider *provider = gtk_css_provider_new();
   GError *err = NULL;

   gtk_css_provider_load_from_path(provider, path, &err);
   if (err != NULL) {
      g_warning("style loading failed: %s", err->message);
      g_error_free(err);
   } else {
      gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   }
   g_object_unref(provider);
}

int 
main (int argc, char **argv) 
{
   GError *error = NULL;
   GOptionContext *option_context = g_option_context_new("- GTK-based greeter for greetd");
   g_option_context_add_main_entries(option_context, entries, NULL);
   g_option_context_add_group(option_context, gtk_get_option_group(TRUE));
   if (!g_option_context_parse(option_context, &argc, &argv, &error)) {
      g_print("option parsing failed: %s\n", error->message);
      exit(1);
   }

   get_sessions();

   greeter = create_greeter();

#ifdef LAYER_SHELL
   greeter->use_layer_shell = use_layer_shell;
#endif
   greeter->command = command;
   
   if (style != NULL) 
      attach_custom_style(style);

   g_signal_connect(greeter->app, "activate", G_CALLBACK(activate), NULL);

   int status = g_application_run(G_APPLICATION(greeter->app), argc, argv);

   greeter_destroy(greeter);

   return status;
}
