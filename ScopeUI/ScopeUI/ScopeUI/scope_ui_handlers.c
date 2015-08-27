#include <gtk-3.0\gtk\gtk.h>
#include <math.h>
#include <string.h>

#include "common.h"
#include "scope.h"

void force_redraw(GtkWidget* drawingArea)
{
	GtkWindow* window = gtk_widget_get_window(drawingArea);

	GdkRectangle rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = gtk_widget_get_allocated_width(drawingArea);
	rect.height = gtk_widget_get_allocated_height(drawingArea);
	gdk_window_invalidate_rect(window, &rect, FALSE);
}

void update_measurements()
{

}

void update_statusbar()
{
	// CH1: xv/div, CH2: yv/div, Time: 1ms/div

	Scope* scope = scope_get();
	ScopeUI* ui = common_get_ui();

	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(ui->statusBar), "Statusbar example");

	gchar* msg = g_strdup_printf("CH1: %.1fv/div, CH2: %.1fv/div, Time: %.3fs/div",
		scope->screen.traces[0].scale,
		scope->screen.traces[1].scale,
		scope->screen.dt
		);

	guint remove = gtk_statusbar_push(GTK_STATUSBAR(ui->statusBar), context_id, msg);
	g_free(msg);
}

G_MODULE_EXPORT
void on_button1_pressed(GtkButton *button, gpointer user_data)
{
	
}

G_MODULE_EXPORT
void on_window1_destroy(GtkWidget *object, gpointer user_data)
{
	gtk_main_quit();
}

G_MODULE_EXPORT
void on_checkCh1Visible_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->screen.traces[0].visible = gtk_toggle_button_get_active(togglebutton);
	force_redraw((GtkWidget*)user_data);
}

G_MODULE_EXPORT
void on_checkCh2Visible_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->screen.traces[1].visible = gtk_toggle_button_get_active(togglebutton);
	force_redraw((GtkWidget*)user_data);
}


G_MODULE_EXPORT
void on_change_scale1(GtkSpinButton *spin_button, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->screen.traces[0].scale = gtk_spin_button_get_value(spin_button);
	update_statusbar();
	force_redraw((GtkWidget*)user_data);
}

G_MODULE_EXPORT
void on_change_offset1(GtkSpinButton *spin_button, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->screen.traces[0].offset = -1 * gtk_spin_button_get_value(spin_button);
	force_redraw((GtkWidget*)user_data);
}

G_MODULE_EXPORT
void on_change_scale2(GtkSpinButton *spin_button, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->screen.traces[1].scale = gtk_spin_button_get_value(spin_button);
	update_statusbar();
	force_redraw((GtkWidget*)user_data);
}

G_MODULE_EXPORT
void on_change_offset2(GtkSpinButton *spin_button, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->screen.traces[1].offset = -1 * gtk_spin_button_get_value(spin_button);
	force_redraw((GtkWidget*)user_data);
}

G_MODULE_EXPORT
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	gint w = 0;
	guint width, height;
	Scope* scope = scope_get();

	screen_fill_background(widget, cr);
	screen_draw_grid(widget, cr);

	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);
	
	// create samples
	float step = 2 * G_PI / (float)width * 20;
	for (w = 0; w < width; ++w)
	{
		scope->channels[0].buffer->data[w] = sin(w*step);
		scope->channels[1].buffer->data[w] = -3 * (sin(w*step));
	}

	screen_draw_traces(widget, cr);	

	return FALSE;
}