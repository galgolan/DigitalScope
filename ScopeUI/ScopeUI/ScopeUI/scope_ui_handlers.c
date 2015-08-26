#include <gtk-3.0\gtk\gtk.h>
#include <math.h>

#include "common.h"
#include "scope.h"

G_MODULE_EXPORT
void on_window1_destroy(GtkWidget *object, gpointer user_data)
{
	gtk_main_quit();
}

G_MODULE_EXPORT
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	int w = 0;
	guint width, height;
	Scope* scope = screen_get();

	screen_fill_background(widget, cr);
	screen_draw_grid(widget, cr);

	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);
	
	// create samples
	float step = 2 * G_PI / width * 20;
	for (w = 0; w < width; ++w)
	{
		scope->channels[0].buffer->data[w] = sin(w*step);
		scope->channels[1].buffer->data[w] = -3 * (sin(w*step));
	}

	screen_draw_traces(widget, cr);	

	return FALSE;
}