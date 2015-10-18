#include <gtk-3.0\gtk\gtk.h>
#include <glib-2.0\glib.h>

#include "common.h"
#include "scope.h"
#include "serial.h"
#include "config.h"

#define GET_GTK_WIDGET(name) GTK_WIDGET(gtk_builder_get_object(builder, name));
#define GET_GTK_OBJECT(name) gtk_builder_get_object(builder, name);

static ScopeUI scopeUI;

ScopeUI* common_get_ui()
{
	return &scopeUI;
}

void populate_ui(GtkBuilder* builder)
{
	scopeUI.drawingArea = GET_GTK_WIDGET("drawingarea");
	scopeUI.statusBar = GET_GTK_WIDGET("statusbar");
	scopeUI.listMeasurements = (GtkListStore*)GET_GTK_OBJECT("listMeasurements");
	//scopeUI.viewMeasurements = GET_GTK_OBJECT("treeview1");
}

int main(int argc, char *argv[])
{
	GtkBuilder      *builder;
	GtkWidget       *window;
	
	GtkCssProvider *provider;
	GdkDisplay *display;
	GdkScreen *screen;

	GError* error = NULL;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "ui.xml", NULL);
	
	gtk_builder_connect_signals(builder, NULL);

	window = GET_GTK_WIDGET("window1");
	gtk_widget_show(window);
		
	provider = gtk_css_provider_new();
	display = gdk_display_get_default();
	screen = gdk_display_get_default_screen(display);

	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	if (!gtk_css_provider_load_from_path(provider, "style.css", &error))
	{
		g_debug("%s", error->message);
		return 1;
	}

	config_open();
	
	populate_ui(builder);
	screen_init();
	// TODO: set default values for controls
	
	g_object_unref(G_OBJECT(builder));
	gtk_main();

	config_close();

	return 0;
}