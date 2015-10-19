#include <gtk-3.0\gtk\gtk.h>
#include <math.h>
#include <string.h>

#include "common.h"
#include "scope.h"
#include "scope_ui_handlers.h"
#include "measurement.h"

#define LOW_FPS	15

void request_redraw()
{
	Scope* scope = scope_get();
	if ((scope->screen.fps < LOW_FPS) || (scope->state == SCOPE_STATE_PAUSED))
		force_redraw();
}

void force_redraw()
{
	ScopeUI* ui = common_get_ui();
	GdkWindow* window = gtk_widget_get_window(ui->drawingArea);

	// TODO: what about persistency (not clearing the display) ?

	GdkRectangle rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = gtk_widget_get_allocated_width(ui->drawingArea);
	rect.height = gtk_widget_get_allocated_height(ui->drawingArea);
	gdk_window_invalidate_rect(window, &rect, FALSE);
}

void update_statusbar()
{
	// CH1: xv/div, CH2: yv/div, Time: 1ms/div

	Scope* scope = scope_get();
	ScopeUI* ui = common_get_ui();

	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(ui->statusBar), "Statusbar example");

	gchar* msg = g_strdup_printf("CH1: %.1fv/div, CH2: %.1fv/div, Time: %.3fs/div",
		scope_trace_get_nth(0)->scale,
		scope_trace_get_nth(1)->scale,
		scope->screen.dt
		);

	guint remove = gtk_statusbar_push(GTK_STATUSBAR(ui->statusBar), context_id, msg);
	g_free(msg);
}

G_MODULE_EXPORT
void on_buttonRemoveMeasurement_clicked(GtkButton* button, gpointer user_data)
{
	Scope* scope = scope_get();
	ScopeUI* ui = common_get_ui();

	// get selected item and remove it from the TreeView and Scope's measurements list
	// we assume single selection
	GtkTreeSelection* selection = gtk_tree_view_get_selection(ui->viewMeasurements);
	GtkTreeModel* model = GTK_TREE_MODEL(ui->listMeasurements);
	GtkTreeIter iter;
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;	// nothing is selected

	if (!gtk_list_store_remove(ui->listMeasurements, &iter))
	{
		// TODO: handle error
	}
}

G_MODULE_EXPORT
void on_buttonAddMeasurement_clicked(GtkButton* button, gpointer user_data)
{
	Scope* scope = scope_get();
	ScopeUI* ui = common_get_ui();

	gtk_combo_box_set_active(ui->addMeasurementSource, -1);
	gtk_combo_box_set_active(ui->addMeasurementType, -1);

	GtkDialog* dlg = ui->addMeasurementDialog;
	gint response = gtk_dialog_run(dlg);
	gtk_widget_hide((GtkWidget*)dlg);
	if (response == 0)
		return;	// user clicked cancel
	
	int traceId = gtk_combo_box_get_active(ui->addMeasurementSource);
	Trace* trace = scope_trace_get_nth(traceId);

	GQueue* allMeas = measurement_get_all();
	int measId = gtk_combo_box_get_active(ui->addMeasurementType);
	Measurement* meas = g_queue_peek_nth(allMeas, measId);
	scope_measurement_add(meas, trace);
	screen_add_measurement(meas->name, trace->name, 0);
}

G_MODULE_EXPORT
void checkMathVisible_toggled(GtkToggleButton* btn, gpointer user_data)
{
	scope_trace_get_math()->visible = gtk_toggle_button_get_active(btn);
	request_redraw();
}

G_MODULE_EXPORT
void cursorsButton_toggle(GtkToggleButton* btn, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->cursors.visible = gtk_toggle_button_get_active(btn);
	request_redraw();
}

G_MODULE_EXPORT
void on_displayModeButton_toggled(GtkToggleButton* displayModeButton, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->display_mode = gtk_toggle_button_get_active(displayModeButton);
	request_redraw();
}

G_MODULE_EXPORT
void runButton_toggled(GtkToggleButton* runButton, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->state = gtk_toggle_button_get_active(runButton);
	force_redraw();	// make sure the display is most updated as possible
}

// TODO: add finalize and remove timeout callback source

G_MODULE_EXPORT
void on_window1_destroy(GtkWidget *object, gpointer user_data)
{
	gtk_main_quit();
}

G_MODULE_EXPORT
void on_checkCh1Visible_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	scope_trace_get_nth(0)->visible = gtk_toggle_button_get_active(togglebutton);
	request_redraw();
}

G_MODULE_EXPORT
void on_checkCh2Visible_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	scope_trace_get_nth(1)->visible = gtk_toggle_button_get_active(togglebutton);
	request_redraw();
}

G_MODULE_EXPORT
void on_change_scaleMath(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_math()->scale = (float)gtk_spin_button_get_value(spin_button);
	//update_statusbar();
	request_redraw();
}

G_MODULE_EXPORT
void on_math_source_changed(GtkComboBox *widget, gpointer user_data)
{
	int sourceChannel = gtk_combo_box_get_active(widget);
	Scope* scope = scope_get();
	scope->mathTraceDefinition.firstTrace = scope_trace_get_nth(sourceChannel);
	request_redraw();
}

G_MODULE_EXPORT
void on_change_offsetMath(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_math()->offset = -1 * (int)gtk_spin_button_get_value(spin_button);
	request_redraw();
}

G_MODULE_EXPORT
void on_change_scale1(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(0)->scale = (float)gtk_spin_button_get_value(spin_button);
	update_statusbar();
	request_redraw();
}

G_MODULE_EXPORT
void on_change_offset1(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(0)->offset = -1 * (int)gtk_spin_button_get_value(spin_button);
	request_redraw();
}

G_MODULE_EXPORT
void on_change_scale2(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(1)->scale = (float)gtk_spin_button_get_value(spin_button);
	update_statusbar();
	request_redraw();
}

G_MODULE_EXPORT
void on_change_offset2(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(1)->offset = -1 * (int)gtk_spin_button_get_value(spin_button);
	request_redraw();
}

gboolean timeout_callback(gpointer data)
{
	force_redraw();

	return G_SOURCE_REMOVE;
}

G_MODULE_EXPORT
void sampleRateSpin_value_changed_cb(GtkSpinButton *spin_button, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->screen.dt = 1e-3f / (float)gtk_spin_button_get_value(spin_button);	// this is in KHz
	request_redraw();
	update_statusbar();
}

// main draw function
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

	screen_draw_traces(widget, cr);
	draw_cursors(widget, cr);

	return FALSE;
}