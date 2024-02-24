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
   GdkPixbuf *background;

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
};

extern struct SimpleGreeter *greeter;

struct Window* greeter_window_by_widget(struct SimpleGreeter *, GtkWidget *);
struct Window* greeter_window_by_monitor(struct SimpleGreeter *, GdkMonitor *);
void greeter_remove_window_by_widget(struct SimpleGreeter *, GtkWidget *);
void greeter_focus_window(struct SimpleGreeter *, struct Window *);
void greeter_setup_question(struct SimpleGreeter *, enum QuestionType, char *, char *);
void greeter_update_clock(struct SimpleGreeter *);
struct SimpleGreeter* create_greeter();
void greeter_activate(struct SimpleGreeter *);
void greeter_destroy(struct SimpleGreeter *);
char* greeter_get_initial_question();

#endif
