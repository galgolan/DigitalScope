#include <gtk-3.0\gtk\gtk.h>
#include <math.h>
#include <string.h>

#include "scope.h"
#include "scope_ui_handlers.h"
#include "measurement.h"
#include "drawing.h"
#include "threads.h"
#include "trace_math.h"

static ScopeUI scopeUI;

static gboolean btn1 = FALSE;
static gboolean btn3 = FALSE;

ScopeUI* common_get_ui()
{
	return &scopeUI;
}

guint combobox_get_active_id(GtkComboBox* widget, GtkListStore* liststore)
{
	GtkTreeIter iter;
	guint id;
	gtk_combo_box_get_active_iter(widget, &iter);
	GtkTreeModel* model = GTK_TREE_MODEL(liststore);
	gtk_tree_model_get(model, &iter,
		0, &id,
		-1);

	return id;
}

void populate_ui(GtkBuilder* builder)
{
	scopeUI.window = (GtkWindow*)GET_GTK_OBJECT("window1");

	scopeUI.drawingArea = (GtkDrawingArea*)GET_GTK_OBJECT("drawingarea");
	scopeUI.statusBar = (GtkStatusbar*)GET_GTK_OBJECT("statusbar");

	scopeUI.listMeasurements = (GtkListStore*)GET_GTK_OBJECT("listMeasurements");
	scopeUI.viewMeasurements = (GtkTreeView*)GET_GTK_OBJECT("viewMeasurements");
	scopeUI.addMeasurementDialog = (GtkDialog*)GET_GTK_OBJECT("dialogAddMeasurement");
	scopeUI.addMeasurementSource = (GtkComboBox*)GET_GTK_OBJECT("comboMeasurementSource");
	scopeUI.addMeasurementType = (GtkComboBox*)GET_GTK_OBJECT("comboMeasurementType");
	scopeUI.measurementTypesList = (GtkListStore*)GET_GTK_OBJECT("listMeasurementDefinitions");

	scopeUI.liststoreMathTypes = (GtkListStore*)GET_GTK_OBJECT("liststoreMathTypes");
	scopeUI.comboMathType = (GtkComboBox*)GET_GTK_OBJECT("comboMathType");
	scopeUI.comboMathSource = (GtkComboBox*)GET_GTK_OBJECT("comboMathSource");

	scopeUI.tracesList = (GtkListStore*)GET_GTK_OBJECT("liststoreTraces");
	scopeUI.treeviewTraces = (GtkTreeView*)GET_GTK_OBJECT("treeviewTraces");

	scopeUI.liststoreProbeRatio = (GtkListStore*)GET_GTK_OBJECT("liststoreProbeRatio");
	scopeUI.comboChannel1Probe = (GtkComboBox*)GET_GTK_OBJECT("comboChannel1Probe");
	scopeUI.comboChannel2Probe = (GtkComboBox*)GET_GTK_OBJECT("comboChannel2Probe");

	scopeUI.liststoreCursorValues = (GtkListStore*)GET_GTK_OBJECT("liststoreCursorValues");
	scopeUI.treeviewCursorValues = (GtkTreeView*)GET_GTK_OBJECT("treeviewCursorValues");

	scopeUI.runButton = (GtkToggleButton*)GET_GTK_OBJECT("runButton");

	// set event masks
	gtk_widget_add_events(scopeUI.drawingArea, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON3_MOTION_MASK);
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

void populate_list_store_index_string_string(GtkListStore* listStore, GQueue* strings1, GQueue* strings2, gboolean clear)
{
	if (clear == TRUE)
		gtk_list_store_clear(listStore);

	for (guint i = 0; i < g_queue_get_length(strings1); ++i)
	{
		char* string1 = g_queue_peek_nth(strings1, i);
		char* string2 = g_queue_peek_nth(strings2, i);

		GtkTreeIter iter;
		gtk_list_store_append(listStore, &iter);
		gtk_list_store_set(listStore, &iter,
			0, i,
			1, string1,
			2, string2,
			-1);
	}
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
			1, (gpointer)name,
			-1);
	}
}

char* get_trace_scale_string_horizontal(const Trace* trace, char* name)
{
	Scope* scope = scope_get();
	float scale = scope_trace_get_horizontal_scale(trace);
	float perDiv = scale * ((float)scope->screen.width / scope->screen.grid.vertical);
	char* perDiv_s = formatNumber(perDiv, trace->horizontal);
	char* result = g_strdup_printf("   %s: %s/div", name, perDiv_s);
	free(perDiv_s);
	return result;
}

char* get_trace_scale_string_vertical(const Trace* trace, int ratio)
{
	Scope* scope = scope_get();
	float perDiv = (float)scope->screen.maxVoltage * 2 * ratio / scope->screen.grid.horizontal / trace->scale;
	char* perDiv_s = formatNumber(perDiv, trace->vertical);
	char* result = g_strdup_printf("   %s: %s/div", trace->name, perDiv_s);
	free(perDiv_s);
	return result;
}

char* concat(char* dst, char* src)
{
	char* backup = dst;
	dst = g_strconcat(dst, src, NULL);
	g_free(src);
	g_free(backup);
	return dst;
}

void update_statusbar()
{
	ScopeUI* ui = common_get_ui();

	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(ui->statusBar), "Statusbar example");

	const Trace* ch1 = scope_trace_get_nth(0);
	const Trace* ch2 = scope_trace_get_nth(1);
	const Trace* math = scope_trace_get_math();	
	const AnalogChannel* ch1_analog = scope_channel_get_nth(0);
	const AnalogChannel* ch2_analog = scope_channel_get_nth(1);

	gchar* msg = g_strdup_printf("");
	boolean chVisible = FALSE;
	if (ch1->visible)
	{
		char* scale = get_trace_scale_string_vertical(ch1, ch1_analog->probeRatio);
		msg = concat(msg, scale);
		chVisible = TRUE;
	}
	if (ch2->visible)
	{
		char* scale = get_trace_scale_string_vertical(ch2, ch2_analog->probeRatio);
		msg = concat(msg, scale);
		chVisible = TRUE;
	}
	if (math->visible)
	{
		char* scale = get_trace_scale_string_horizontal(math, "Math");
		msg = concat(msg, scale);
	}
	if (chVisible)
	{
		char* scale = get_trace_scale_string_horizontal(ch1, "Time");
		msg = concat(msg, scale);
	}

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

G_MODULE_EXPORT
void on_buttonSaveRef_clicked(GtkButton* button, gpointer user_data)
{
	Scope* scope = scope_get();
	if (scope->screen.selectedTrace != NULL)
	{
		// pause the scope		
		ScopeState oldState = scope->state;
		scope->state = SCOPE_STATE_PAUSED;

		scope_trace_save_ref(scope->screen.selectedTrace);

		scope->state = oldState;
	}
}

G_MODULE_EXPORT
void on_buttonDeleteRefs_clicked(GtkButton* button, gpointer user_data)
{
	Scope* scope = scope_get();
	
	int numAnalogChannels = g_queue_get_length(scope->channels);
	if (scope->screen.selectedTraceId < numAnalogChannels + 1)
		return;	// cant delete a system trace (analog/math)

	scope_trace_delete_ref(scope->screen.selectedTraceId);
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

	gint selectedId = tree_view_get_selected_index(ui->viewMeasurements, ui->listMeasurements);
	
	TRY_LOCK(, "cant acquire lock on traces", scope->screen.hTracesMutex, 100)
	TRY_LOCK(, "cant acquire lock on measurements", scope->hMeasurementsMutex, 100)

	// remove from scope
	MeasurementInstance* measurement = (MeasurementInstance*)g_queue_pop_nth(scope->measurements, selectedId);

	// remove from UI
	GtkTreeIter iter;
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		// TODO: handle error
	}
	if (!gtk_list_store_remove(ui->listMeasurements, &iter))
	{
		// TODO: handle error
	}

	ReleaseMutex(scope->hMeasurementsMutex);
	ReleaseMutex(scope->screen.hTracesMutex);

	free(measurement);
}

G_MODULE_EXPORT
void on_buttonAddMeasurement_clicked(GtkButton* button, gpointer user_data)
{
	Scope* scope = scope_get();
	ScopeUI* ui = common_get_ui();

	gtk_combo_box_set_active(ui->addMeasurementSource, 0);
	gtk_combo_box_set_active(ui->addMeasurementType, -1);

	GtkDialog* dlg = ui->addMeasurementDialog;
	gint response = gtk_dialog_run(dlg);
	gtk_widget_hide((GtkWidget*)dlg);
	if (response == 0)
		return;	// user clicked cancel
	
	int traceId = gtk_combo_box_get_active(ui->addMeasurementSource);
	if (traceId == -1)
		return;

	GQueue* allMeas = measurement_get_all();
	int measId = gtk_combo_box_get_active(ui->addMeasurementType);
	if (measId == -1)
		return;

	TRY_LOCK(, "cant acquire lock on traces", scope->screen.hTracesMutex, 100)
	TRY_LOCK(, "cant acquire lock on measurements", scope->hMeasurementsMutex, 100)

	Trace* trace = scope_trace_get_nth(traceId);
	Measurement* meas = g_queue_peek_nth(allMeas, measId);
	scope_measurement_add(meas, trace);
	screen_add_measurement(meas->name, trace->name, measId);

	ReleaseMutex(scope->hMeasurementsMutex);
	ReleaseMutex(scope->screen.hTracesMutex);
}

G_MODULE_EXPORT
void checkMathVisible_toggled(GtkToggleButton* btn, gpointer user_data)
{
	scope_trace_get_math()->visible = gtk_toggle_button_get_active(btn);
	update_statusbar();
}

G_MODULE_EXPORT
void on_checkbuttonCursorsVisible_toggled(GtkCheckButton* btn, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->cursors.visible = gtk_toggle_button_get_active((GtkToggleButton*)btn);
}

G_MODULE_EXPORT
void on_displayModeButton_toggled(GtkToggleButton* displayModeButton, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->display_mode = gtk_toggle_button_get_active(displayModeButton);
}

G_MODULE_EXPORT
void runButton_toggled(GtkToggleButton* runButton, gpointer user_data)
{
	Scope* scope = scope_get();
	bool running = gtk_toggle_button_get_active(runButton);
	if (running)
	{
		scope_build_and_send_config();
		// handle run event
		handle_scope_event(SCOPE_EVENT_RUN, NULL);
	}
	else
	{
		drawing_redraw();	// make sure the display is most updated as possible
		// handle pause event
		handle_scope_event(SCOPE_EVENT_PAUSE, NULL);
	}
}

// TODO: add finalize and remove timeout callback source

G_MODULE_EXPORT
void on_window1_destroy(GtkWidget *object, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->shuttingDown = TRUE;
	// TODO: wait for all the threads
	Sleep(1000);
	gtk_main_quit();
}

G_MODULE_EXPORT
void on_checkCh1Visible_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	scope_trace_get_nth(0)->visible = gtk_toggle_button_get_active(togglebutton);
	update_statusbar();
}

G_MODULE_EXPORT
void on_checkCh2Visible_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	scope_trace_get_nth(1)->visible = gtk_toggle_button_get_active(togglebutton);
	update_statusbar();
}

G_MODULE_EXPORT
void on_change_scaleMath(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_math()->scale = (float)gtk_spin_button_get_value(spin_button);
	//update_statusbar();
}

G_MODULE_EXPORT
void on_comboMathType_changed(GtkComboBox *widget, gpointer user_data)
{
	int mathType = gtk_combo_box_get_active(widget);
	GQueue* mathTypes = math_get_all();
	MathTrace* math = (MathTrace*)g_queue_peek_nth(mathTypes, mathType);
	scope_math_change(math);
	update_statusbar();
}

G_MODULE_EXPORT
void on_math_source_changed(GtkComboBox *widget, gpointer user_data)
{
	int sourceChannel = gtk_combo_box_get_active(widget);
	Scope* scope = scope_get();
	scope->mathTraceDefinition.firstTrace = scope_trace_get_nth(sourceChannel);
	update_statusbar();
}

G_MODULE_EXPORT
void on_comboChannel1Probe_changed(GtkComboBox *widget, gpointer user_data)
{
	GtkTreeIter iter;
	guint ratio;
	gtk_combo_box_get_active_iter(widget, &iter);
	GtkTreeModel* model = GTK_TREE_MODEL(scopeUI.liststoreProbeRatio);
	gtk_tree_model_get(model, &iter,
		0, &ratio,
		-1);

	Scope* scope = scope_get();
	AnalogChannel* ch = g_queue_peek_nth(scope->channels, 0);
	ch->probeRatio = ratio;
}

G_MODULE_EXPORT
void on_comboChannel2Probe_changed(GtkComboBox *widget, gpointer user_data)
{
	GtkTreeIter iter;
	guint ratio;
	gtk_combo_box_get_active_iter(widget, &iter);
	GtkTreeModel* model = GTK_TREE_MODEL(scopeUI.liststoreProbeRatio);
	gtk_tree_model_get(model, &iter,
		0, &ratio,
		-1);

	Scope* scope = scope_get();
	AnalogChannel* ch = g_queue_peek_nth(scope->channels, 1);
	ch->probeRatio = ratio;
}

G_MODULE_EXPORT
void on_change_offsetMath(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_math()->offset = -1 * (int)gtk_spin_button_get_value(spin_button);
}

G_MODULE_EXPORT
void on_change_scale1(GtkSpinButton *spin_button, gpointer user_data)
{
	int scale = (int)gtk_spin_button_get_value(spin_button);
	scope_trace_get_nth(0)->scale = (float)scale;
	update_statusbar();
	scope_build_and_send_config();
}

G_MODULE_EXPORT
void on_change_offset1(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(0)->offset = -1 * (int)gtk_spin_button_get_value(spin_button);
	scope_build_and_send_config();
}

G_MODULE_EXPORT
void on_change_scale2(GtkSpinButton *spin_button, gpointer user_data)
{
	int scale = (int)gtk_spin_button_get_value(spin_button);
	scope_trace_get_nth(1)->scale = (float)scale;
	update_statusbar();
	scope_build_and_send_config();
}

G_MODULE_EXPORT
void on_change_offset2(GtkSpinButton *spin_button, gpointer user_data)
{
	scope_trace_get_nth(1)->offset = -1 * (int)gtk_spin_button_get_value(spin_button);
	scope_build_and_send_config();
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
	update_statusbar();
	scope_build_and_send_config();
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
	Scope* scope = scope_get();

	gint selectedId = tree_view_get_selected_index(ui->treeviewTraces, ui->tracesList);
	if (selectedId != -1)
	{
		scope->screen.selectedTrace = scope_trace_get_nth(selectedId);
		scope->screen.selectedTraceId = selectedId;
	}
}

G_MODULE_EXPORT
void on_drawing_area_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->screen.width = allocation->width;
	scope->screen.height = allocation->height;
}

gboolean trigger_line_hide_callback(gpointer data)
{
	Scope* scope = scope_get();
	scope->screen.showTrigger = FALSE;

	return G_SOURCE_REMOVE;
}

G_MODULE_EXPORT
void on_spinbuttonTriggerLevel_value_changed(GtkSpinButton *spin_button, gpointer user_data)
{
	Scope* scope = scope_get();
	scope->trigger.level = (float)gtk_spin_button_get_value(spin_button);
	scope->screen.showTrigger = TRUE;
	scope_build_and_send_config();
	
	// set a timer for 3 secs
	g_timeout_add_seconds(3, trigger_line_hide_callback, NULL);
}

G_MODULE_EXPORT
void on_comboboxTriggerMode_changed(GtkComboBox *widget, gpointer user_data)
{
	Scope* scope = scope_get();
	guint activeItem = combobox_get_active_id(widget, (GtkListStore*)user_data);
	TriggerMode newMode = (TriggerMode)activeItem;
	scope->trigger.mode = newMode;
	handle_scope_event(SCOPE_EVENT_TRIGGER_MODE_CHANGE, &newMode);
	scope_build_and_send_config();
}

G_MODULE_EXPORT
void on_comboboxTriggerSource_changed(GtkComboBox *widget, gpointer user_data)
{
	Scope* scope = scope_get();
	guint activeItem = combobox_get_active_id(widget, (GtkListStore*)user_data);
	scope->trigger.source = (TriggerSource)activeItem;
	scope_build_and_send_config();
}

G_MODULE_EXPORT
void on_comboboxTriggerType_changed(GtkComboBox *widget, gpointer user_data)
{
	Scope* scope = scope_get();
	guint activeItem = combobox_get_active_id(widget, (GtkListStore*)user_data);
	scope->trigger.type = (TriggerType)activeItem;
	scope_build_and_send_config();
}

G_MODULE_EXPORT
void on_imagemenuitem5_activate(GtkMenuItem* item, gpointer data)
{
	on_window1_destroy((GtkWidget*)item, data);
}

G_MODULE_EXPORT
void on_imagemenuitem4_select(GtkMenuItem* item, gpointer data)
{
	// handle save trace as
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	gint res;

	
	Scope* scope = scope_get();
	ScopeState oldState = scope->state;	// backup current stat
	scope->state = SCOPE_STATE_PAUSED;	// pause the scope

	dialog = gtk_file_chooser_dialog_new("Save File",
		NULL,
		action,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE,
		GTK_RESPONSE_ACCEPT,
		NULL);
	chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
	gtk_file_chooser_set_current_name(chooser, "Untitled.png");

	GtkFileFilter* filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.png");
	gtk_file_chooser_set_filter(chooser, filter);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		char* filename = gtk_file_chooser_get_filename(chooser);		
		drawing_save_screen_as_image(filename);		
		g_free(filename);
	}

	gtk_widget_destroy(dialog);

	scope->state = oldState;	// restore scope state
}

G_MODULE_EXPORT
gboolean on_drawingarea_button_press_event(GtkWidget *widget, GdkEventButton  *event, gpointer   user_data)
{
	Scope* scope = scope_get();
	if (event->button == 1)
	{
		btn1 = TRUE;
		// put x1 & y1 cursors on this position
		scope_cursor_set(&(scope->cursors.x1), (int)event->x);
		scope_cursor_set(&(scope->cursors.y1), (int)event->y);
	}
	else if (event->button == 3)
	{
		btn3 = TRUE;
		// put x2 & y2 cursors on this position
		scope_cursor_set(&(scope->cursors.x2), (int)event->x);
		scope_cursor_set(&(scope->cursors.y2), (int)event->y);
	}
	
	return TRUE;
}

G_MODULE_EXPORT
gboolean on_drawingarea_button_release_event(GtkWidget *widget, GdkEventButton  *event, gpointer   user_data)
{
	if (event->button == 1)
		btn1 = FALSE;
	else if (event->button == 3)
		btn3 = FALSE;
	
	return TRUE;
}

G_MODULE_EXPORT
gboolean on_drawingarea_motion_notify_event(GtkWidget *widget, GdkEventMotion  *event, gpointer   user_data)
{
	Scope* scope = scope_get();
	if ((btn1) && (scope->cursors.visible))
	{
		// put x1 & y1 cursors on this position
		scope_cursor_set(&(scope->cursors.x1), (int)event->x);
		scope_cursor_set(&(scope->cursors.y1), (int)event->y);
	}
	else if ((btn3) && (scope->cursors.visible))
	{
		// put x2 & y2 cursors on this position
		scope_cursor_set(&(scope->cursors.x2), (int)event->x);
		scope_cursor_set(&(scope->cursors.y2), (int)event->y);
	}
	
	return TRUE;
}