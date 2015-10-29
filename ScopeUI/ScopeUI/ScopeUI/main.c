#include <gtk-3.0\gtk\gtk.h>
#include <glib-2.0\glib.h>

#include "scope.h"
#include "serial.h"
#include "config.h"
#include "scope_ui_handlers.h"

bool testHandleBuffer()
{
	char buffer[128];
	sprintf(buffer, "%04x", 0.5f);
	char* b1 = ".............:TRIG:12341234:";
	handle_receive_date(b1, strlen(b1));
}

void controls_set_default_values(GtkBuilder* builder)
{
	GList* keys = config_get_keys("controls");
	while (keys != NULL)
	{
		char* key = (char*)keys->data;

		GtkWidget* widget = GET_GTK_WIDGET(key);

		keys = keys->next;
	}

	g_list_free_full(keys, g_strfreev);
}

int main(int argc, char *argv[])
{
	testHandleBuffer();

	return;
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

	//controls_set_default_values(builder);
	screen_init();
	// TODO: set default values for controls
	
	g_object_unref(G_OBJECT(builder));
	gtk_main();

	config_close();

	return 0;
}