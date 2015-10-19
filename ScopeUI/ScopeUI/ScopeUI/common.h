#ifndef _COMMON_H_
#define _COMMON_H_

#include <gtk-3.0\gtk\gtk.h>

typedef struct ScopeUI
{
	GtkWidget* drawingArea;
	GtkWidget* statusBar;

	GtkListStore* listMeasurements;
	GtkTreeView* viewMeasurements;

	// measurements
	GtkDialog* addMeasurementDialog;
	GtkComboBox* addMeasurementSource;
	GtkComboBox* addMeasurementType;
	GtkListStore* measurementTypesList;

	// tracelist
	GtkListStore* tracesList;
	GtkTreeView* treeviewTraces;
} ScopeUI;

ScopeUI* common_get_ui();

#endif
