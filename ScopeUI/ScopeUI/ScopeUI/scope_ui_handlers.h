#ifndef _SCOPE_UI_HANDLERS_H_
#define _SCOPE_UI_HANDLERS_H_

#include <gtk-3.0\gtk\gtk.h>
#include <glib-2.0\glib.h>

gboolean timeout_callback(gpointer data);

void update_statusbar();

void populate_list_store(GtkListStore* listStore, GQueue* items, gboolean clear);

#endif
