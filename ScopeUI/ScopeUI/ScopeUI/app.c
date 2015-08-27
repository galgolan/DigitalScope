#include <gtk/gtk.h>

#include "app.h"
#include "window.h"

struct _ScopeApp
{
	GtkApplication parent;
};

struct _ScopeAppClass
{
	GtkApplicationClass parent_class;
};

G_DEFINE_TYPE(ScopeApp, scope_app, GTK_TYPE_APPLICATION);

static void scope_app_init(ScopeApp* app)
{

}

// start app without command line args
static void scope_app_activate(GApplication *app)
{
	// create the window
	ScopeAppWindow* window = scope_app_window_new(SCOPE_APP(app));
	gtk_window_present(GTK_WINDOW(window));
}

// start app with command line args
static void scope_app_open(GApplication *app, GFile **files, gint n_files, const gchar *hint)
{
	ScopeAppWindow* window = scope_app_window_new(SCOPE_APP(app));
	/*
	GList* windows;
	ScopeAppWindow* window;
	int i;

	windows = gtk_application_get_windows(GTK_APPLICATION(app));
	if (windows)
		window = SCOPE_APP_WINDOW(windows->data);
	else
		window = scope_app_window_new(SCOPE_APP(app));
		*/

	//ScopeAppWindow* window = scope_app_window_new(SCOPE_APP(app));
	scope_app_window_open(window);
	gtk_window_present(GTK_WINDOW(window));
}

static void scope_app_class_init(ScopeAppClass* class)
{
	G_APPLICATION_CLASS(class)->activate = scope_app_activate;
	G_APPLICATION_CLASS(class)->open = scope_app_open;
}

ScopeApp* scope_app_new(void)
{
	return g_object_new(SCOPE_APP_TYPE,
		"application-id", "com.galgo.scopeapp",
		"flags", G_APPLICATION_HANDLES_COMMAND_LINE,
		NULL);
}