#ifndef _GTKGREET_H
#define _GTKGREET_H

#include <gtk/gtk.h>

enum QuestionType {
   QuestionTypeInitial = 0,
   QuestionTypeVisible = 1,
   QuestionTypeSecret = 2,
   QuestionTypeInfo = 3,
   QuestionTypeError = 4,
};

// Defined in window.h
struct Window;

struct SimpleGreeter {
   GtkApplication *app;
   GArray *windows;

   struct Window *focused_window;
   guint draw_clock_source;

#ifdef LAYER_SHELL
   gboolean use_layer_shell;
#endif
   char* command;

   char* selected_command;
   enum QuestionType question_type;
   char *question;
   char *error;
   char time[64];
   int question_cnt;
};

extern struct SimpleGreeter *greeter;

static inline char* greeter_get_initial_question() { return "Username: "; }

struct Window* greeter_window_by_widget(struct SimpleGreeter *greeter, GtkWidget *window);
struct Window* greeter_window_by_monitor(struct SimpleGreeter *greeter, GdkMonitor *monitor);
void greeter_remove_window_by_widget(struct SimpleGreeter *greeter, GtkWidget *widget);
void greeter_focus_window(struct SimpleGreeter *greeter, struct Window* win);
void greeter_setup_question(struct SimpleGreeter *greeter, enum QuestionType type, char* question, char* error);
void greeter_update_clocks(struct SimpleGreeter *greeter);
struct SimpleGreeter* create_greeter();
void greeter_activate(struct SimpleGreeter *greeter);
void greeter_destroy(struct SimpleGreeter *greeter);

#endif
