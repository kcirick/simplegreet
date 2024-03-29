#ifndef _WINDOW_H
#define _WINDOW_H

#include <gtk/gtk.h>

// Defined in gtkgreet.h
enum QuestionType;

struct Window {
   GdkMonitor *monitor;

   GtkWidget *window;
   GtkWidget *window_box;

   GtkWidget *top_box;
   GtkWidget *clock_label;
   GtkWidget *power_label;
   GtkWidget *command_selector;

   GtkWidget *body_box;
   GtkWidget *input_box;
   GtkWidget *input_field;

#ifdef LAYER_SHELL
   gulong enter_notify_handler;
#endif

   int question_cnt;
};

struct Window *create_window(GdkMonitor *monitor);
void window_configure(struct Window *win);
//void window_setup_input_box(struct Window *ctx, enum QuestionType type, char* question, char* error);
void window_update_clock(struct Window *ctx);
void window_swap_focus(struct Window *win, struct Window *old);

#endif
