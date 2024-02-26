#ifndef _CONFIG_H
#define _CONFIG_H

#include <gtk/gtk.h>

int get_sessions();

void config_update_command_selector(GtkWidget *, gboolean);
char * config_get_command_from_selector(GtkWidget *, gboolean);

void config_write_last_session(char *);
char * config_read_last_session();

#endif
