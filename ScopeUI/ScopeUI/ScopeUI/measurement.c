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

typedef struct AddMeasurementMessage
{
	const MeasurementInstance* measurement;
	double value;	
} AddMeasurementMessage;

gboolean clear_measurements_callback(gpointer data)
{
	screen_clear_measurements();

	return G_SOURCE_REMOVE;
}

gboolean add_measurement_callback(gpointer data)
{
	AddMeasurementMessage* msg = (AddMeasurementMessage*)data;
	screen_add_measurement(msg->measurement->measurement->name, msg->measurement->trace->name, msg->value);

	free(msg);

	return G_SOURCE_REMOVE;
}

void process_measurement(const MeasurementInstance* measurement)
{
	SampleBuffer* data = measurement->trace->samples;
	float result = measurement->measurement->measure(data);

	// add results to UI
	AddMeasurementMessage* msg = malloc(sizeof(AddMeasurementMessage));
	// TODO: check mem allocation
	msg->measurement = measurement;
	msg->value = result;

	guint source_id = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT, add_measurement_callback, msg, NULL);
}

DWORD WINAPI measurement_worker_thread(LPVOID param)
{
	Scope* scope = scope_get();

	while (TRUE)
	{
		// signal the UI to clear the list
		guint source_id = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT, clear_measurements_callback, NULL, NULL);

		// iterate over all measurements and update them
		for (int i = 0; i < g_queue_get_length(scope->measurements); ++i)
		{
			MeasurementInstance* meas = scope_measurement_get_nth(i);
			process_measurement(meas);
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