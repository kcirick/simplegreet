#include <gtk/gtk.h>

#include "window.h"
#include "greeter.h"

struct Window * 
greeter_window_by_widget(struct SimpleGreeter *greeter, GtkWidget *window) 
{
   for (guint idx = 0; idx < greeter->windows->len; idx++) {
      struct Window *ctx = g_array_index(greeter->windows, struct Window*, idx);
      if (ctx->window == window) 
         return ctx;
   }
   return NULL;
}

struct Window * 
greeter_window_by_monitor(struct SimpleGreeter *greeter, GdkMonitor *monitor)
{
   for (guint idx = 0; idx < greeter->windows->len; idx++) {
      struct Window *ctx = g_array_index(greeter->windows, struct Window*, idx);
      if (ctx->monitor == monitor) 
         return ctx;
   }
   return NULL;
}

void 
greeter_remove_window_by_widget(struct SimpleGreeter *greeter, GtkWidget *widget)
{
   for (guint idx = 0; idx < greeter->windows->len; idx++) {
      struct Window *ctx = g_array_index(greeter->windows, struct Window*, idx);
      if (ctx->window == widget) {
         if (greeter->focused_window) 
            greeter->focused_window = NULL;

         free(ctx);
         g_array_remove_index_fast(greeter->windows, idx);
         return;
      }
   }
}

void 
greeter_focus_window(struct SimpleGreeter *greeter, struct Window* win)
{
   struct Window *old = greeter->focused_window;
   greeter->focused_window = win;
   window_swap_focus(win, old);
   for (guint idx = 0; idx < greeter->windows->len; idx++) {
      struct Window *ctx = g_array_index(greeter->windows, struct Window*, idx);
      window_configure(ctx);
   }
}

void 
greeter_setup_question(struct SimpleGreeter *greeter, enum QuestionType type, char* question, char* error)
{
   if (greeter->question != NULL) {
      free(greeter->question);
      greeter->question = NULL;
   }
   if (greeter->error != NULL) {
      free(greeter->error);
      greeter->error = NULL;
   }
   greeter->question_type = type;
   if (question != NULL)
      greeter->question = g_strdup(question);
   if (error != NULL)
      greeter->error = g_strdup(error);
   greeter->question_cnt += 1;
   
   for (guint idx = 0; idx < greeter->windows->len; idx++) {
      struct Window *ctx = g_array_index(greeter->windows, struct Window*, idx);
      window_configure(ctx);
   }
}

void 
greeter_update_clocks(struct SimpleGreeter *greeter) 
{
   time_t now = time(&now);
   struct tm *now_tm = localtime(&now);
   if (now_tm == NULL) return;

   snprintf(greeter->time, 64, "%d/%d/%d - %02d:%02d", 1900+now_tm->tm_year, 1+now_tm->tm_mon, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min);
   for (guint idx = 0; idx < greeter->windows->len; idx++) {
      struct Window *ctx = g_array_index(greeter->windows, struct Window*, idx);
      window_update_clock(ctx);
   }
}

static int 
greeter_update_clocks_handler(gpointer data) 
{
   struct SimpleGreeter *greeter = (struct SimpleGreeter*)data;
   greeter_update_clocks(greeter);
   return TRUE;
}

struct SimpleGreeter * 
create_greeter() {
   greeter = calloc(1, sizeof(struct SimpleGreeter));
   greeter->app = gtk_application_new("wtf.kl.simplegreeter", G_APPLICATION_DEFAULT_FLAGS);
   greeter->windows = g_array_new(FALSE, TRUE, sizeof(struct Window*));
   greeter->question_cnt = 1;

   return greeter;
}

void 
greeter_activate(struct SimpleGreeter *greeter) 
{
   greeter->draw_clock_source = g_timeout_add_seconds(5, greeter_update_clocks_handler, greeter);
   greeter_setup_question(greeter, QuestionTypeInitial, greeter_get_initial_question(), NULL);
   greeter_update_clocks(greeter);
}

void 
greeter_destroy(struct SimpleGreeter *greeter) 
{
   if (greeter->question != NULL) {
      free(greeter->question);
      greeter->question = NULL;
   }
   if (greeter->error != NULL) {
      free(greeter->error);
      greeter->error = NULL;
   }

   g_object_unref(greeter->app);
   g_array_unref(greeter->windows);

   if (greeter->draw_clock_source > 0) {
      g_source_remove(greeter->draw_clock_source);
      greeter->draw_clock_source = 0;
   }
   free(greeter);
}

char * 
greeter_get_initial_question() 
{
   return "Username:";
}
