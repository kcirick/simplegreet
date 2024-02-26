#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

static int nsessions = 0;
static char session_names[16][128];
static char session_commands[16][128];

static const char fname_lastsession[] = "/var/lib/greetd/simplegreet-lastsession";

static void 
trim(char *orig)
{
   size_t i, len;

   if(orig==NULL) return;

   // right trim
   len = strlen(orig);
   if(len == 0) return;
   for (i=len-1; i>0; i--){
      if(isspace(orig[i])) orig[i]=0;
      else                 break;
   }
   if(isspace(orig[i])) orig[i]=0;

   // left trim
   i=0;
   len = strlen(orig);
   if(len==0) return;
   while(orig[i] && isspace(orig[i])) i++;
   memmove(orig, orig+i, len -i + 1);
}

int
get_sessions () 
{
   struct dirent *dp;
   DIR *dfd;

   char* dir = "/usr/local/share/wayland-sessions";

   if((dfd = opendir(dir))==NULL){
      fprintf(stderr, "Can't open %s\n", dir);
      return 0;
   }

   char filename_qfd[100];

   while((dp = readdir(dfd)) != NULL) {
      if ((dp->d_name)[0] == '.') continue;

      if (nsessions>15){
         fprintf(stderr, "Can't load more than 16 sessions\n");
         return nsessions;
      }

      sprintf(filename_qfd, "%s/%s", dir, dp->d_name);

      FILE *f;
      if(!(f=fopen(filename_qfd, "r"))){
         fprintf(stderr, "Error reading file %s\n", filename_qfd);
         continue;
      }
      char buffer[128];
      char id[32];
      char value[128];
      char* token;
      //fprintf(stdout, "File = %s\n", filename_qfd);
      while(fgets(buffer, sizeof buffer, f)){

         if(buffer[0]=='\n' || buffer[0]=='#' || buffer[0]=='[') continue;

         token = strtok(buffer, "=");
         strncpy(id, token, sizeof id);
         trim(id);

         token = strtok(NULL, "=");
         strncpy(value, token, sizeof value);
         trim(value);

         if(!strcmp(id, "Name")) strcpy(session_names[nsessions], value);
         if(!strcmp(id, "Exec")) strcpy(session_commands[nsessions], value);
      }
      fprintf(stdout, "Session %d : name = %s / command = %s\n", nsessions, session_names[nsessions], session_commands[nsessions]);
      nsessions++;

      fclose(f);
   }
   return nsessions;
}

void
config_update_command_selector(GtkWidget *combobox, gboolean set_last_session)
{
   for(int i=0; i<nsessions; i++){
      gtk_combo_box_text_append((GtkComboBoxText*)combobox, NULL, session_names[i]);
   }

   if(!set_last_session) return;

   char *last_session;
   last_session = g_strdup(config_read_last_session());

   if(last_session == '\0') return;

   for(int i=0; i<16; i++){
      if(!strcmp(last_session, session_names[i])){
         gtk_combo_box_set_active((GtkComboBox*)combobox, i);
         break;
      }
   }
}

char *
config_get_command_from_selector(GtkWidget *combobox, gboolean save_session)
{
   int id = gtk_combo_box_get_active((GtkComboBox*)combobox);
   fprintf(stdout, "Selected command = %s\n", session_commands[id]);

   if(save_session)
      config_write_last_session(session_names[id]);

   return session_commands[id];
}

void
config_write_last_session(char* lastsession)
{
   
   FILE *f;

   if(!(f=fopen(fname_lastsession, "w+"))) {
      fprintf(stderr, "Error opening file %s\n", fname_lastsession);
      return;
   }

   fwrite(lastsession, 1, sizeof(lastsession), f);

   fclose(f);
}

char *
config_read_last_session() 
{
   FILE *f;
   char buffer[64];
   char *session_name;

   if(!(f=fopen(fname_lastsession, "r"))) {
      fprintf(stderr, "Error opening file %s\n", fname_lastsession);
      session_name = g_strdup('\0');
      return NULL;
   }

   while(fgets(buffer, sizeof buffer, f)){
      session_name = g_strdup(buffer);
      fprintf(stdout, "session_name = %s\n", session_name);
   }
   fclose(f);

   return session_name;
}
