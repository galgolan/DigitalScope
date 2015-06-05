#include <gtk/gtk.h>

#include "app.h"
#include "window.h"

struct _ScopeAppWindow
{
	GtkApplicationWindow parent;
};

struct _ScopeAppWindowClass
{
	GtkApplicationWindowClass parent_class;
};

G_DEFINE_TYPE(ScopeAppWindow, scope_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void scope_app_window_init(ScopeAppWindow* win)
{
	//gtk_widget_init_template(GTK_WIDGET(win));
}

static void scope_app_window_class_init(ScopeAppWindowClass* class)
{
	// load template to window class instance
}

ScopeAppWindow* scope_app_window_new(ScopeApp* app)
{
	return g_object_new(SCOPE_APP_WINDOW_TYPE, "application", app, NULL);
}

void scope_app_window_open(ScopeAppWindow* win)
{

}