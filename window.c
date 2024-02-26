#include <time.h>
#include <assert.h>

#include <gtk/gtk.h>
#ifdef LAYER_SHELL
#include <gtk-layer-shell.h>
#endif

#include "proto.h"
#include "window.h"
#include "greeter.h"
#include "actions.h"

static void window_set_focus(struct Window *win, struct Window *old);

#ifdef LAYER_SHELL
static void 
window_set_focus_layer_shell(struct Window *win, struct Window *old) 
{
   if (old != NULL)
      gtk_layer_set_keyboard_interactivity(GTK_WINDOW(old->window), FALSE);

   gtk_layer_set_keyboard_interactivity(GTK_WINDOW(win->window), TRUE);
}

static gboolean 
window_enter_notify(GtkWidget *widget, gpointer data) 
{
   struct Window *win = greeter_window_by_widget(greeter, widget);
   greeter_focus_window(greeter, win);

   return FALSE;
}

static void 
window_setup_layershell(struct Window *ctx) 
{
   gtk_widget_add_events(ctx->window, GDK_ENTER_NOTIFY_MASK);
   if (ctx->enter_notify_handler > 0) {
      g_signal_handler_disconnect(ctx->window, ctx->enter_notify_handler);
      ctx->enter_notify_handler = 0;
   }
   ctx->enter_notify_handler = g_signal_connect(ctx->window, "enter-notify-event", G_CALLBACK(window_enter_notify), NULL);

   gtk_layer_init_for_window(GTK_WINDOW(ctx->window));
   gtk_layer_set_layer(GTK_WINDOW(ctx->window), GTK_LAYER_SHELL_LAYER_TOP);
   gtk_layer_set_monitor(GTK_WINDOW(ctx->window), ctx->monitor);
   gtk_layer_auto_exclusive_zone_enable(GTK_WINDOW(ctx->window));
   gtk_layer_set_anchor(GTK_WINDOW(ctx->window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
   gtk_layer_set_anchor(GTK_WINDOW(ctx->window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
   gtk_layer_set_anchor(GTK_WINDOW(ctx->window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
   gtk_layer_set_anchor(GTK_WINDOW(ctx->window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
}
#endif

void 
window_update_clock(struct Window *ctx) 
{
   gtk_label_set_markup((GtkLabel*)ctx->clock_label, greeter->time);
}

static void 
window_setup_input_box(struct Window *ctx, enum QuestionType type, char* question, char* error) 
{
   if (greeter->focused_window != NULL && ctx != greeter->focused_window)
      return;

   if (ctx->input_box != NULL) {
      if (greeter->question_cnt == ctx->question_cnt)
         return;

      gtk_widget_destroy(ctx->input_box);
      ctx->input_box = NULL;

      // Children of the box
      ctx->input_field = NULL;
   }

   ctx->input_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
   GtkWidget *question_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
   gtk_widget_set_halign(question_box, GTK_ALIGN_END);
   GtkWidget *label;

   switch (type) {
      case QuestionTypeInitial:
      case QuestionTypeVisible:
      case QuestionTypeSecret:
         label = gtk_label_new(question);
         gtk_widget_set_halign(label, GTK_ALIGN_END);
         gtk_container_add(GTK_CONTAINER(question_box), label);

         ctx->input_field = gtk_entry_new();
         gtk_widget_set_name(ctx->input_field, "input-field");
         if (type == QuestionTypeSecret) {
            gtk_entry_set_input_purpose((GtkEntry*)ctx->input_field, GTK_INPUT_PURPOSE_PASSWORD);
            gtk_entry_set_visibility((GtkEntry*)ctx->input_field, FALSE);
         }
         g_signal_connect(ctx->input_field, "activate", G_CALLBACK(action_answer_question), ctx);
         gtk_widget_set_size_request(ctx->input_field, 250, -1);
         gtk_widget_set_halign(ctx->input_field, GTK_ALIGN_END);
         gtk_container_add(GTK_CONTAINER(question_box), ctx->input_field);
         break;
      case QuestionTypeInfo:
      case QuestionTypeError:
         label = gtk_label_new(question);
         gtk_widget_set_halign(label, GTK_ALIGN_END);
         gtk_container_add(GTK_CONTAINER(question_box), label);
         break;
   }

   gtk_container_add(GTK_CONTAINER(ctx->input_box), question_box);

   gtk_container_add(GTK_CONTAINER(ctx->body_box), ctx->input_box);

   // Buttons
   GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
   gtk_widget_set_halign(button_box, GTK_ALIGN_END);
   gtk_container_add(GTK_CONTAINER(ctx->input_box), button_box);

   // Error message
   if (error != NULL) {
      GtkWidget *label = gtk_label_new(error);
      char err[128];
      snprintf(err, 128, "<span color=\"red\">%s</span>", error);
      gtk_label_set_markup((GtkLabel*)label, err);
      gtk_widget_set_halign(label, GTK_ALIGN_END);
      gtk_container_add(GTK_CONTAINER(button_box), label);
   }

   switch (type) {
      case QuestionTypeVisible:
      case QuestionTypeSecret:
      case QuestionTypeInfo:
      case QuestionTypeError: 
         GtkWidget *cancel_button = gtk_button_new_with_label("Cancel");
         gtk_widget_set_halign(cancel_button, GTK_ALIGN_END);
         gtk_container_add(GTK_CONTAINER(button_box), cancel_button);
         g_signal_connect(cancel_button, "clicked", G_CALLBACK(action_cancel_question), ctx);
         break;
      case QuestionTypeInitial:
         break;
   }

   GtkWidget *continue_button = gtk_button_new_with_label("Log in");
   g_signal_connect(continue_button, "clicked", G_CALLBACK(action_answer_question), ctx);
   GtkStyleContext *continue_button_style = gtk_widget_get_style_context(continue_button);
   gtk_style_context_add_class(continue_button_style, "suggested-action");

   gtk_widget_set_halign(continue_button, GTK_ALIGN_END);
   gtk_container_add(GTK_CONTAINER(button_box), continue_button);

   gtk_widget_show_all(ctx->window);

   if (ctx->input_field != NULL) 
      gtk_widget_grab_focus(ctx->input_field);
}

static void
window_setup_top_box(struct Window *ctx)
{
   // Power label
   ctx->power_label = gtk_label_new("F1 - Power Off  |  F2 - Reboot");
   gtk_widget_set_name(ctx->power_label, "power_label");
   gtk_widget_set_valign(ctx->power_label, GTK_ALIGN_CENTER);
   gtk_widget_set_halign(ctx->power_label, GTK_ALIGN_START);
   gtk_box_pack_start(GTK_BOX(ctx->top_box), ctx->power_label, TRUE, TRUE, 0);

   // Clock widget
   ctx->clock_label = gtk_label_new("");
   gtk_widget_set_name(ctx->clock_label, "clock_label");
   gtk_widget_set_valign(ctx->clock_label, GTK_ALIGN_CENTER);
   gtk_widget_set_halign(ctx->clock_label, GTK_ALIGN_CENTER);
   gtk_box_set_center_widget(GTK_BOX(ctx->top_box), ctx->clock_label);
   gtk_box_pack_start(GTK_BOX(ctx->top_box), ctx->clock_label, TRUE, TRUE, 0);

   window_update_clock(ctx);

   // command selector
   ctx->command_selector = gtk_combo_box_text_new();
   gtk_widget_set_name(ctx->command_selector, "command_selector");
   //gtk_widget_set_size_request(ctx->command_selector, 384, -1);
   config_update_command_selector(ctx->command_selector, TRUE);
   //gtk_combo_box_text_append((GtkComboBoxText*)ctx->command_selector, NULL, greeter->command);
   gtk_widget_set_halign(ctx->command_selector, GTK_ALIGN_END);
   //gtk_combo_box_set_active((GtkComboBox*)ctx->command_selector, 0);

   GtkWidget *selector_entry = gtk_bin_get_child((GtkBin*)ctx->command_selector);
   gtk_entry_set_placeholder_text((GtkEntry*)selector_entry, "Command to run on login");

   gtk_box_pack_end(GTK_BOX(ctx->top_box), ctx->command_selector, TRUE, TRUE, 0);
}

static void 
window_empty(struct Window *ctx) 
{
   ctx->window_box = NULL;
   ctx->clock_label = NULL;
   ctx->body_box = NULL;
   ctx->input_box = NULL;
   ctx->input_field = NULL;
   ctx->top_box = NULL;
   ctx->power_label = NULL;
   ctx->clock_label = NULL;
   ctx->command_selector = NULL;
}

static void 
window_setup(struct Window *ctx) 
{
   // Create general structure if it is missing
   if (ctx->window_box == NULL) {
      ctx->window_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_widget_set_name(ctx->window_box, "window");
      gtk_container_add(GTK_CONTAINER(ctx->window), ctx->window_box);
   }

   if (ctx->top_box == NULL) {
      ctx->top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
      gtk_widget_set_name(ctx->top_box, "top_box");
      gtk_widget_set_valign(ctx->top_box, GTK_ALIGN_START);
      gtk_widget_set_halign(ctx->top_box, GTK_ALIGN_FILL);
      gtk_widget_set_hexpand(ctx->top_box, TRUE);
      gtk_container_add(GTK_CONTAINER(ctx->window_box), ctx->top_box);

      window_setup_top_box(ctx);
   }

   // Update input area if necessary
   if (greeter->focused_window == ctx || greeter->focused_window == NULL) {
      if (ctx->body_box == NULL) {
         ctx->body_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
         gtk_widget_set_halign(ctx->body_box, GTK_ALIGN_CENTER);
         gtk_widget_set_valign(ctx->body_box, GTK_ALIGN_CENTER);
         gtk_widget_set_name(ctx->body_box, "body_box");
         gtk_widget_set_size_request(ctx->body_box, 250, -1);
         gtk_container_add(GTK_CONTAINER(ctx->window_box), ctx->body_box);
      }

      window_setup_input_box(ctx, greeter->question_type, greeter->question, greeter->error);
   } else if (ctx->body_box != NULL) {
      gtk_widget_destroy(ctx->body_box);
      ctx->body_box = NULL;
      ctx->input_box = NULL;
      ctx->input_field = NULL;
   }

   //window_update_clock(ctx);
   ctx->question_cnt = greeter->question_cnt;
}

static void 
window_destroy_notify(GtkWidget *widget, gpointer data) 
{
   window_empty(greeter_window_by_widget(greeter, widget));
   greeter_remove_window_by_widget(greeter, widget);
}

static void 
window_set_focus(struct Window *win, struct Window *old) 
{
   assert(win != NULL);

   window_setup(win);

   if (old != NULL && old != win) {
      if (old->input_field != NULL && win->input_field != NULL) {
         // Get previous cursor position
         gint cursor_pos = 0;
         g_object_get((GtkEntry*)old->input_field, "cursor-position", &cursor_pos, NULL);

         // Move content
         gtk_entry_set_text((GtkEntry*)win->input_field, gtk_entry_get_text((GtkEntry*)old->input_field));
         gtk_entry_set_text((GtkEntry*)old->input_field, "");

         // Update new cursor position
         g_signal_emit_by_name((GtkEntry*)win->input_field, "move-cursor", GTK_MOVEMENT_BUFFER_ENDS, -1, FALSE);
         g_signal_emit_by_name((GtkEntry*)win->input_field, "move-cursor", GTK_MOVEMENT_LOGICAL_POSITIONS, cursor_pos, FALSE);
      }
      if (old->command_selector != NULL && win->command_selector != NULL) {
         gtk_combo_box_set_active((GtkComboBox*)win->command_selector, gtk_combo_box_get_active((GtkComboBox*)old->command_selector));
      }
      window_setup(old);
      gtk_widget_show_all(old->window);
   }
   gtk_widget_show_all(win->window);
}

void 
window_swap_focus(struct Window *win, struct Window *old) 
{
#ifdef LAYER_SHELL
   if (greeter->use_layer_shell)
      window_set_focus_layer_shell(win, old);
#endif
   window_set_focus(win, old);
}

void 
window_configure(struct Window *w) 
{
#ifdef LAYER_SHELL
   if (greeter->use_layer_shell)
      window_setup_layershell(w);
#endif
   window_setup(w);
   gtk_widget_show_all(w->window);
}

struct Window *
create_window(GdkMonitor *monitor) 
{
   struct Window *w = calloc(1, sizeof(struct Window));
   if (w == NULL) {
      fprintf(stderr, "failed to allocate Window instance\n");
      exit(1);
   }
   w->monitor = monitor;
   g_array_append_val(greeter->windows, w);

   w->window = gtk_application_window_new(greeter->app);
   g_signal_connect(w->window, "destroy", G_CALLBACK(window_destroy_notify), NULL);
   gtk_window_set_title(GTK_WINDOW(w->window), "Greeter");
   gtk_window_set_default_size(GTK_WINDOW(w->window), 200, 200);

   return w;
}
