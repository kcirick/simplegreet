#include <time.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "actions.h"
#include "proto.h"
#include "greeter.h"
#include "window.h"

static void 
handle_response(struct response resp, int start_req) 
{
   struct request req;
   //fprintf(stdout, "test\n");
   switch (resp.response_type) {
      case response_type_success: 
         if (start_req) 
            exit(0);

         req.request_type = request_type_start_session;
         strncpy(req.body.request_start_session.cmd, greeter->selected_command, 127);
         handle_response(roundtrip(req), 1);
         break;
      case response_type_auth_message: 
         if (start_req) {
            req.request_type = request_type_cancel_session;
            roundtrip(req);

            char *error = "Unexpected auth question";
            greeter_setup_question(greeter, QuestionTypeInitial, greeter_get_initial_question(), error);
            break;
         }

         greeter_setup_question(greeter,
               (enum QuestionType)resp.body.response_auth_message.auth_message_type,
               resp.body.response_auth_message.auth_message,
               NULL);
         break;
      case response_type_roundtrip_error:
      case response_type_error: 
         req.request_type = request_type_cancel_session;
         roundtrip(req);

         char* error = NULL;
         if (resp.response_type == response_type_error &&
               resp.body.response_error.error_type == error_type_auth) {
            error = "Login failed";
         } else {
            error = resp.body.response_error.description;
         }
         greeter_setup_question(greeter, QuestionTypeInitial, greeter_get_initial_question(), error);
         break;
   }
}

void 
action_answer_question(GtkWidget *widget, gpointer data) 
{
   struct Window *ctx = data;
   struct request req;

   if (greeter->selected_command) {
      free(greeter->selected_command);
      greeter->selected_command = NULL;
   }
   greeter->selected_command = g_strdup(gtk_combo_box_text_get_active_text((GtkComboBoxText*)ctx->command_selector));

   req.request_type = request_type_create_session;
   if (ctx->username_field != NULL) 
      strncpy(req.body.request_create_session.username, gtk_entry_get_text((GtkEntry*)ctx->username_field), 127);

   handle_response(roundtrip(req), 0);

   req.request_type = request_type_post_auth_message_response;
   if (ctx->password_field != NULL)
      strncpy(req.body.request_post_auth_message_response.response, gtk_entry_get_text((GtkEntry*)ctx->password_field), 127);

   handle_response(roundtrip(req), 0);

   //req.request_type = request_type_post_auth_message_response;
   //req.body.request_post_auth_message_response.response[0] = '\0';
   //handle_response(roundtrip(req), 0);
}

void 
action_cancel_question(GtkWidget *widget, gpointer data) 
{
   struct request req = {
      .request_type = request_type_cancel_session,
   };
   struct response resp = roundtrip(req);
   if (resp.response_type != response_type_success) 
      exit(1);

   greeter_setup_question(greeter, QuestionTypeInitial, greeter_get_initial_question(), NULL);
}
