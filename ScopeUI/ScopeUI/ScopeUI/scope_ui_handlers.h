#ifndef _SCOPE_UI_HANDLERS_H_
#define _SCOPE_UI_HANDLERS_H_

#include <gtk-3.0\gtk\gtk.h>
#include <glib-2.0\glib.h>

#define GET_GTK_WIDGET(name) GTK_WIDGET(gtk_builder_get_object(builder, name));
#define GET_GTK_OBJECT(name) gtk_builder_get_object(builder, name);

typedef struct ScopeUI
{
	GtkWindow* window;

	GtkDrawingArea* drawingArea;
	GtkStatusbar* statusBar;

	GtkListStore* listMeasurements;
	GtkTreeView* viewMeasurements;

	// measurements
	GtkDialog* addMeasurementDialog;
	GtkComboBox* addMeasurementSource;
	GtkComboBox* addMeasurementType;
	GtkListStore* measurementTypesList;

	// math
	GtkComboBox* comboMathSource;
	GtkComboBox* comboMathType;
	GtkListStore* liststoreMathTypes;

	// tracelist
	GtkListStore* tracesList;
	GtkTreeView* treeviewTraces;

	GtkListStore* liststoreProbeRatio;
	GtkComboBox* comboChannel1Probe;
	GtkComboBox* comboChannel2Probe;

	// cursors
	GtkListStore* liststoreCursorValues;
	GtkTreeView* treeviewCursorValues;

	GtkToggleButton* runButton;
} ScopeUI;

ScopeUI* common_get_ui();

void populate_ui(GtkBuilder* builder);

gboolean timeout_callback(gpointer data);

void update_statusbar();

void populate_list_store(GtkListStore* listStore, GQueue* items, gboolean clear);

void populate_list_store_values_int(GtkListStore* listStore, GQueue* names, GQueue* values, gboolean clear);

void populate_list_store_index_string_string(GtkListStore* listStore, GQueue* strings1, GQueue* strings2, gboolean clear);

#endif
