#include <gtk-3.0\gtk\gtk.h>

#include "common.h"
#include "scope.h"
#include "serial.h"

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
	scopeUI.listMeasurements = GET_GTK_OBJECT("listMeasurements");
	//scopeUI.viewMeasurements = GET_GTK_OBJECT("treeview1");
}

int main(int argc, char *argv[])
{
	GtkBuilder      *builder;
	GtkWidget       *window;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "ui.xml", NULL);
	//window = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
	window = GET_GTK_WIDGET("window1");
	GtkWidget* statusbar = GET_GTK_WIDGET("statusbar");
	gtk_builder_connect_signals(builder, NULL);

	gtk_widget_show(window);

	populate_ui(builder);
	g_object_unref(G_OBJECT(builder));

	screen_init(builder);

	gtk_main();

	return 0;
}