#include <math.h>
#include <string.h>

#include <Windows.h>

#include "common.h"
#include "scope.h"
#include "measurement.h"

// measurement prototypes
float measure_avg(SampleBuffer* samples);
float measure_min(SampleBuffer* samples);
float measure_max(SampleBuffer* samples);
float measure_vpp(SampleBuffer* samples);
float measure_rms(SampleBuffer* samples);

// measurements
Measurement Measurement_Average = { .name = "Average", .measure = measure_avg };
Measurement Measurement_Minimum = { .name = "Minimum", .measure = measure_min };
Measurement Measurement_Maximum = { .name = "Maximum", .measure = measure_max };
Measurement Measurement_PeakToPeak = { .name = "Vpp", .measure = measure_vpp };
Measurement Measurement_RMS = { .name = "Vrms", .measure = measure_rms };

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

gboolean update_measurement_callback(gpointer data)
{
	ScopeUI* scopeUI = common_get_ui();
	GQueue* msgs = (GQueue*)data;
	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(scopeUI->listMeasurements), &iter);

	for (guint i = 0; i < g_queue_get_length(msgs); ++i)
	{
		if (!gtk_list_store_iter_is_valid(scopeUI->listMeasurements, &iter))
			break;

		AddMeasurementMessage* msg = (AddMeasurementMessage*)g_queue_peek_nth(msgs, i);
		gtk_list_store_set(scopeUI->listMeasurements, &iter,
			2, msg->value,
			-1);

		if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(scopeUI->listMeasurements), &iter))
			break;
	}

	g_queue_free_full(msgs, free);

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

DWORD WINAPI measurement_worker_thread(LPVOID param)
{
	Scope* scope = scope_get();
	

	while (TRUE)
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
		}

		Sleep(300);
	}
}

// ********** calculation funtions *********************

float measure_avg(SampleBuffer* samples)
{
	float avg = 0;
	int i;
	
	for (i = 0; i < samples->size; ++i)
	{
		avg += samples->data[i] / samples->size;
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