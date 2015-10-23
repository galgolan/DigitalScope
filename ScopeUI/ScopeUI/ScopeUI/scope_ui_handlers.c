#include <gtk-3.0\gtk\gtk.h>
#include <math.h>
#include <string.h>

#include "scope.h"
#include "scope_ui_handlers.h"
#include "measurement.h"
#include "drawing.h"

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
	scopeUI.viewMeasurements = (GtkTreeView*)GET_GTK_WIDGET("viewMeasurements");
	scopeUI.addMeasurementDialog = (GtkDialog*)GET_GTK_WIDGET("dialogAddMeasurement");
	scopeUI.addMeasurementSource = (GtkComboBox*)GET_GTK_WIDGET("comboMeasurementSource");
	scopeUI.addMeasurementType = (GtkComboBox*)GET_GTK_WIDGET("comboMeasurementType");
	scopeUI.measurementTypesList = (GtkListStore*)GET_GTK_OBJECT("listMeasurementDefinitions");

	scopeUI.tracesList = (GtkListStore*)GET_GTK_OBJECT("liststoreTraces");
	scopeUI.treeviewTraces = (GtkTreeView*)GET_GTK_OBJECT("treeviewTraces");

	scopeUI.liststoreProbeRatio = (GtkListStore*)GET_GTK_OBJECT("liststoreProbeRatio");
	scopeUI.comboChannel1Probe = (GtkComboBox*)GET_GTK_OBJECT("comboChannel1Probe");
	scopeUI.comboChannel2Probe = (GtkComboBox*)GET_GTK_OBJECT("comboChannel2Probe");
}

void populate_list_store(GtkListStore* listStore, GQueue* items, gboolean clear)
{
	GQueue* values = g_queue_new();
	for (guint i = 0; i < g_queue_get_length(items); ++i)
	{
		g_queue_push_tail(values, GINT_TO_POINTER(i));
	}

	populate_list_store_values_int(listStore, items, values, clear);
	g_queue_free(values);
}

void populate_list_store_values_int(GtkListStore* listStore, GQueue* names, GQueue* values, gboolean clear)
{
	if (clear == TRUE)
		gtk_list_store_clear(listStore);

	for (guint i = 0; i < g_queue_get_length(names); ++i)
	{
		char* name = g_queue_peek_nth(names, i);
		int value = GPOINTER_TO_INT(g_queue_peek_nth(values, i));

		GtkTreeIter iter;
		gtk_list_store_append(listStore, &iter);
		gtk_list_store_set(listStore, &iter,
			0, value,
			1, name,
			-1);
	}
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

// return -1 if zero or more than one rows are selected
gint tree_view_get_selected_index(GtkTreeView* treeView, GtkListStore* listStore)
{
	gint index = -1;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(treeView);
	if (gtk_tree_selection_count_selected_rows(selection) != 1)
		return index;

	GtkTreeModel* model = GTK_TREE_MODEL(listStore);

	GList* selectedRows = gtk_tree_selection_get_selected_rows(selection, &model);
	GtkTreePath* path = (GtkTreePath*)g_list_nth_data(selectedRows, 0);
	gint* indices = gtk_tree_path_get_indices(path);
	if (indices == NULL)
		return index;

	index = indices[0];
	
	g_list_free_full(selectedRows, (GDestroyNotify)gtk_tree_path_free);

	return index;
}

// get selected item and remove it from the TreeView and Scope's measurements list
// we assume single selection
G_MODULE_EXPORT
void on_buttonRemoveMeasurement_clicked(GtkButton* button, gpointer user_data)
{
	Scope* scope = scope_get();
	ScopeUI* ui = common_get_ui();
	GtkTreeModel* model = GTK_TREE_MODEL(ui->listMeasurements);
		
	GtkTreeSelection* selection = gtk_tree_view_get_selection(ui->viewMeasurements);
	if (gtk_tree_selection_count_selected_rows(selection) != 1)
		return;
	
	// remove from scope
	gint selectedId = tree_view_get_selected_index(ui->viewMeasurements, ui->listMeasurements);
	g_queue_pop_nth(scope->measurements, selectedId);

	// remove from UI
	GtkTreeIter iter;
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		// TODO: handle error
		return;
	}
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
	if (traceId == -1)
		return;
	Trace* trace = scope_trace_get_nth(traceId);

	GQueue* allMeas = measurement_get_all();
	int measId = gtk_combo_box_get_active(ui->addMeasurementType);
	if (measId == -1)
		return;
	Measurement* meas = g_queue_peek_nth(allMeas, measId);
	scope_measurement_add(meas, trace);
	screen_add_measurement(meas->name, trace->name, 0, measId);
}

G_MODULE_EXPORT
void checkMathVisible_toggled(GtkToggleButton* btn, gpointer user_data)
{
	scope_trace_get_math()->visible = gtk_toggle_button_get_active(btn);
	drawing_request_redraw();
}

G_MODULE_EXPORT
void cursorsButton_toggle(GtkToggleButton* btn, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->cursors.visible = gtk_toggle_button_get_active(btn);
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_displayModeButton_toggled(GtkToggleButton* displayModeButton, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->display_mode = gtk_toggle_button_get_active(displayModeButton);
	drawing_request_redraw();
}

G_MODULE_EXPORT
void runButton_toggled(GtkToggleButton* runButton, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->state = gtk_toggle_button_get_active(runButton);
	drawing_redraw();	// make sure the display is most updated as possible
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
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_checkCh2Visible_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	scope_trace_get_nth(1)->visible = gtk_toggle_button_get_active(togglebutton);
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_change_scaleMath(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_math()->scale = (float)gtk_spin_button_get_value(spin_button);
	//update_statusbar();
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_math_source_changed(GtkComboBox *widget, gpointer user_data)
{
	int sourceChannel = gtk_combo_box_get_active(widget);
	Scope* scope = scope_get();
	scope->mathTraceDefinition.firstTrace = scope_trace_get_nth(sourceChannel);
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_comboChannel1Probe_changed(GtkComboBox *widget, gpointer user_data)
{
	GtkTreeIter iter;
	guint ratio;
	gtk_combo_box_get_active_iter(widget, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(scopeUI.liststoreProbeRatio), &iter,
		0, &ratio);

	Scope* scope = scope_get();
	AnalogChannel* ch = g_queue_peek_nth(scope->channels, 0);
	ch->probeRatio = ratio;
	
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_comboChannel2Probe_changed(GtkComboBox *widget, gpointer user_data)
{
	GtkTreeIter iter;
	guint ratio;
	gtk_combo_box_get_active_iter(widget, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(scopeUI.liststoreProbeRatio), &iter,
		0, &ratio);

	Scope* scope = scope_get();
	AnalogChannel* ch = g_queue_peek_nth(scope->channels, 1);
	ch->probeRatio = ratio;

	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_change_offsetMath(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_math()->offset = -1 * (int)gtk_spin_button_get_value(spin_button);
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_change_scale1(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(0)->scale = (float)gtk_spin_button_get_value(spin_button);
	update_statusbar();
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_change_offset1(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(0)->offset = -1 * (int)gtk_spin_button_get_value(spin_button);
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_change_scale2(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(1)->scale = (float)gtk_spin_button_get_value(spin_button);
	update_statusbar();
	drawing_request_redraw();
}

G_MODULE_EXPORT
void on_change_offset2(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(1)->offset = -1 * (int)gtk_spin_button_get_value(spin_button);
	drawing_request_redraw();
}

gboolean timeout_callback(gpointer data)
{
	drawing_redraw();
	return G_SOURCE_REMOVE;
}

G_MODULE_EXPORT
void sampleRateSpin_value_changed_cb(GtkSpinButton *spin_button, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->screen.dt = 1e-3f / (float)gtk_spin_button_get_value(spin_button);	// this is in KHz
	drawing_request_redraw();
	update_statusbar();
}

// main draw function
G_MODULE_EXPORT
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	drawing_copy_from_buffer(cr);
	return FALSE;
}

G_MODULE_EXPORT
void treeview_selection2_changed_cb(GtkTreeSelection *treeselection, gpointer user_data)
{
	ScopeUI* ui = common_get_ui();

	gint selectedId = tree_view_get_selected_index(ui->treeviewTraces, ui->tracesList);
	if (selectedId != -1)
	{
		// TODO: update selected trace in Scope
	}
}

G_MODULE_EXPORT
void on_drawing_area_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
	drawing_request_redraw();
}