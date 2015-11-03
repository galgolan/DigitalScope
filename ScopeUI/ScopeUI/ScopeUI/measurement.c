#include <math.h>
#include <string.h>

#include <Windows.h>

#include "scope.h"
#include "measurement.h"
#include "scope_ui_handlers.h"
#include "drawing.h"
#include "config.h"

// measurement prototypes
float measure_avg(SampleBuffer* samples);
float measure_min(SampleBuffer* samples);
float measure_max(SampleBuffer* samples);
float measure_vpp(SampleBuffer* samples);
float measure_rms(SampleBuffer* samples);

// measurements
Measurement Measurement_Average = { .name = "Average", .measure = measure_avg, .units = UNITS_VOLTAGE };
Measurement Measurement_Minimum = { .name = "Minimum", .measure = measure_min, .units = UNITS_VOLTAGE };
Measurement Measurement_Maximum = { .name = "Maximum", .measure = measure_max, .units = UNITS_VOLTAGE };
Measurement Measurement_PeakToPeak = { .name = "Vpp", .measure = measure_vpp, .units = UNITS_VOLTAGE };
Measurement Measurement_RMS = { .name = "Vrms", .measure = measure_rms, .units = UNITS_VOLTAGE };

GQueue* measurement_get_all()
{
	static gboolean ready = FALSE;
	static GQueue* allMeasurements = NULL;

	if (ready == FALSE)
	{
		allMeasurements = g_queue_new();

		// build list
		g_queue_push_tail(allMeasurements, &Measurement_Average);
		g_queue_push_tail(allMeasurements, &Measurement_Minimum);
		g_queue_push_tail(allMeasurements, &Measurement_Maximum);
		g_queue_push_tail(allMeasurements, &Measurement_PeakToPeak);
		g_queue_push_tail(allMeasurements, &Measurement_RMS);

		ready = TRUE;
	}

	return allMeasurements;
}

typedef struct AddMeasurementMessage
{
	const MeasurementInstance* measurement;
	double value;	
} AddMeasurementMessage;

typedef struct UpdateCursorValueMessage
{
	char* x1_value;
	char* x2_value;
	char* y1_value;
	char* y2_value;
	char* dx_value;
	char* dy_value;
} UpdateCursorValueMessage;

gboolean update_measurement_callback(gpointer data)
{
	ScopeUI* scopeUI = common_get_ui();
	GQueue* msgs = (GQueue*)data;
	GtkTreeIter iter;
	GtkTreeModel* model = GTK_TREE_MODEL(scopeUI->listMeasurements);
	gtk_tree_model_get_iter_first(model, &iter);

	for (guint i = 0; i < g_queue_get_length(msgs); ++i)
	{
		if (!gtk_list_store_iter_is_valid(scopeUI->listMeasurements, &iter))
			break;

		AddMeasurementMessage* msg = (AddMeasurementMessage*)g_queue_peek_nth(msgs, i);
		gtk_list_store_set(scopeUI->listMeasurements, &iter,
			2, msg->value,			// TODO: change value to string with number formatting
			-1);

		if (!gtk_tree_model_iter_next(model, &iter))
			break;
	}

	g_queue_free_full(msgs, free);

	return G_SOURCE_REMOVE;
}

gboolean update_cursor_values_callback(gpointer data)
{
	ScopeUI* scopeUI = common_get_ui();
	UpdateCursorValueMessage* msg = (UpdateCursorValueMessage*)data;
	
	// update all 6 cursors
	GtkTreeIter iter;
	GtkTreeModel* model = GTK_TREE_MODEL(scopeUI->liststoreCursorValues);
	gtk_tree_model_get_iter_first(model, &iter);
	
	char* values[6] = { msg->x1_value, msg->x2_value, msg->y1_value, msg->y2_value, msg->dx_value, msg->dy_value };

	for (int i = 0; i < 6; ++i)
	{
		gtk_list_store_set(scopeUI->liststoreCursorValues, &iter,
			2, values[i],
			-1);

		free(values[i]);

		if (!gtk_tree_model_iter_next(model, &iter))
			break;
	}

	free(msg);

	return G_SOURCE_REMOVE;
}

AddMeasurementMessage* process_measurement(const MeasurementInstance* measurement)
{
	SampleBuffer* data = measurement->trace->samples;
	float result = measurement->measurement->measure(data);

	// add results to UI
	AddMeasurementMessage* msg = malloc(sizeof(AddMeasurementMessage));
	msg->measurement = measurement;
	msg->value = result;

	return msg;
}

char* calculate_cursor_value(const Cursor* cursor)
{
	Scope* scope = scope_get();
	Trace* trace = scope->screen.selectedTrace;

	float value;
	Units units;
	if (cursor->type == CURSOR_TYPE_VERTICAL)
	{
		// value of the cursor is number of pixels from the left * dt
		value = cursor->position * scope->screen.dt;
		units = trace->horizontal;
	}
	else // horizontal
	{
		value = inverse_translate(cursor->position, trace);
		units = trace->vertical;
	}

	return formatNumber(value, units);
}

char* calculate_cursor_value_diff(const Cursor* cursor1, const Cursor* cursor2)
{
	Scope* scope = scope_get();
	Trace* trace = scope->screen.selectedTrace;

	float value;
	Units units;
	if (cursor1->type == CURSOR_TYPE_VERTICAL)
	{
		// value of the cursor is number of pixels from the left * dt
		// TODO: use screen.dt for time and frequency bins for frequency
		value = (cursor1->position - cursor2->position) * scope->screen.dt;
		units = trace->horizontal;
	}
	else // horizontal
	{
		value = inverse_translate(cursor1->position, trace) - inverse_translate(cursor2->position, trace);
		units = trace->vertical;
	}

	return formatNumber(value, units);
}

DWORD WINAPI measurement_worker_thread(LPVOID param)
{
	DWORD updateInterval = config_get_int("display", "meas_refresh");

	Scope* scope = scope_get();
	
	while (!scope->shuttingDown)
	{
		if (scope->display_mode == DISPLAY_MODE_WAVEFORM)
		{
			GQueue* msgs = g_queue_new();

			// iterate over all measurements and update them
			for (guint i = 0; i < g_queue_get_length(scope->measurements); ++i)
			{
				MeasurementInstance* meas = scope_measurement_get_nth(i);
				AddMeasurementMessage* msg = process_measurement(meas);
				g_queue_push_tail(msgs, msg);
			}

			guint source_id = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT, update_measurement_callback, msgs, NULL);


			if ((scope->cursors.visible) && (scope->screen.selectedTrace != NULL))
			{
				// update cursor values
				UpdateCursorValueMessage* msg = (UpdateCursorValueMessage*)malloc(sizeof(UpdateCursorValueMessage));
				msg->x1_value = calculate_cursor_value(&(scope->cursors.x1));
				msg->x2_value = calculate_cursor_value(&(scope->cursors.x2));
				msg->y1_value = calculate_cursor_value(&(scope->cursors.y1));
				msg->y2_value = calculate_cursor_value(&(scope->cursors.y2));
				msg->dx_value = calculate_cursor_value_diff(&(scope->cursors.x1), &(scope->cursors.x2));
				msg->dy_value = calculate_cursor_value_diff(&(scope->cursors.y1), &(scope->cursors.y2));

				guint source_id2 = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT, update_cursor_values_callback, msg, NULL);
			}			
		}

		Sleep(updateInterval);
	}

	return 0;
}

// ********** calculation funtions *********************

float measure_avg(SampleBuffer* samples)
{
	float avg = 0;
	int i;
	
	for (i = 0; i < samples->size; ++i)
	{
		avg += samples->data[i] / (float)samples->size;
	}

	return avg;
}

float measure_max(SampleBuffer* samples)
{
	float max = samples->data[0];
	int i;

	for (i = 1; i < samples->size; ++i)
	{
		if (samples->data[i] > max)
			max = samples->data[i];
	}

	return max;
}

float measure_min(SampleBuffer* samples)
{
	float min = samples->data[0];
	int i;

	for (i = 1; i < samples->size; ++i)
	{
		if (samples->data[i] < min)
			min = samples->data[i];
	}

	return min;
}

float measure_vpp(SampleBuffer* samples)
{
	float min = measure_min(samples);
	float max = measure_max(samples);
	return max - min;
}

float measure_rms(SampleBuffer* samples)
{
	float sum_squares = 0;
	int i;

	for (i = 0; i < samples->size; ++i)
	{
		sum_squares += samples->data[i] * samples->data[i];
	}

	return (float)sqrt(sum_squares / samples->size);
}