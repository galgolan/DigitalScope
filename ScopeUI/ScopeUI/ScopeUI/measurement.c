#include <math.h>
#include <string.h>

#include <Windows.h>

#include "scope.h"
#include "measurement.h"
#include "scope_ui_handlers.h"
#include "drawing.h"
#include "config.h"
#include "trace_math.h"
#include "threads.h"

// measurement prototypes
float measure_avg(SampleBuffer* samples);
float measure_min(SampleBuffer* samples);
float measure_max(SampleBuffer* samples);
float measure_vpp(SampleBuffer* samples);
float measure_amplitude(SampleBuffer* samples);
float measure_rms(SampleBuffer* samples);
float measure_rise_time(SampleBuffer* samples);
float measure_fall_time(SampleBuffer* samples);
float measure_dutyCycle(SampleBuffer* samples);
float measure_high(SampleBuffer* samples);
float measure_low(SampleBuffer* samples);
float measure_frequency(SampleBuffer* samples);

// measurements
Measurement Measurement_Average = { .name = "Mean", .measure = measure_avg, .units = UNITS_VOLTAGE };
Measurement Measurement_Minimum = { .name = "Minimum", .measure = measure_min, .units = UNITS_VOLTAGE };
Measurement Measurement_Maximum = { .name = "Maximum", .measure = measure_max, .units = UNITS_VOLTAGE };
Measurement Measurement_PeakToPeak = { .name = "Peak-To-Peak", .measure = measure_vpp, .units = UNITS_VOLTAGE };
Measurement Measurement_Amplitude = { .name = "Amplitude", .measure = measure_amplitude, .units = UNITS_VOLTAGE };
Measurement Measurement_RMS = { .name = "Vrms", .measure = measure_rms, .units = UNITS_VOLTAGE };
Measurement Measurement_DutyCycle = { .name = "Duty Cycle", .measure = measure_dutyCycle, .units = UNITS_PERCENT };
Measurement Measurement_RiseTime = { .name = "Rise Time", .measure = measure_rise_time, .units = UNITS_TIME };
Measurement Measurement_FallTime = { .name = "Fall Time", .measure = measure_fall_time, .units = UNITS_TIME };
Measurement Measurement_High = { .name = "High", .measure = measure_high, .units = UNITS_VOLTAGE };
Measurement Measurement_Low = { .name = "Low", .measure = measure_low, .units = UNITS_VOLTAGE };
Measurement Measurement_Frequency = { .name = "Frequency", .measure = measure_frequency, .units = UNITS_FREQUENCY };

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
		g_queue_push_tail(allMeasurements, &Measurement_Amplitude);
		g_queue_push_tail(allMeasurements, &Measurement_RMS);
		g_queue_push_tail(allMeasurements, &Measurement_RiseTime);
		g_queue_push_tail(allMeasurements, &Measurement_FallTime);
		g_queue_push_tail(allMeasurements, &Measurement_DutyCycle);
		g_queue_push_tail(allMeasurements, &Measurement_High);
		g_queue_push_tail(allMeasurements, &Measurement_Low);
		g_queue_push_tail(allMeasurements, &Measurement_Frequency);

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
		char* valueString = formatNumber((float)msg->value, msg->measurement->measurement->units);
		gtk_list_store_set(scopeUI->listMeasurements, &iter,
			2, valueString,
			-1);

		free(valueString);

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
		units = trace->horizontal;
		
		// value of the cursor is number of pixels from the left * horizontalScale
		value = cursor->position * scope_trace_get_horizontal_scale(trace);
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
		value = (cursor1->position - cursor2->position) * scope_trace_get_horizontal_scale(trace);
		units = trace->horizontal;
	}
	else // horizontal
	{
		value = inverse_translate(cursor1->position, trace) - inverse_translate(cursor2->position, trace);
		units = trace->vertical;
	}

	return formatNumber(value, units);
}

GQueue* measurements_dispatcher()
{
	Scope* scope = scope_get();
	GQueue* msgs = g_queue_new();

	if (WaitForMutex(scope->hMeasurementsMutex, 100))
	{
		// iterate over all measurements and update them
		for (guint i = 0; i < g_queue_get_length(scope->measurements); ++i)
		{
			MeasurementInstance* meas = scope_measurement_get_nth(i);
			AddMeasurementMessage* msg = process_measurement(meas);
			g_queue_push_tail(msgs, msg);
		}

		ReleaseMutex(scope->hMeasurementsMutex);
	}

	return msgs;
}

DWORD WINAPI measurement_worker_thread(LPVOID param)
{
	DWORD updateInterval = config_get_int("display", "meas_refresh");

	Scope* scope = scope_get();
	
	while (!scope->shuttingDown)
	{
		if (scope->display_mode == DISPLAY_MODE_WAVEFORM)
		{
			GQueue* msgs = measurements_dispatcher();

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

float average(float* samples, int size)
{
	float avg = 0;
	int i;

	for (i = 0; i < size; ++i)
	{
		avg += samples[i] / size;
	}

	return avg;
}

float measure_avg(SampleBuffer* samples)
{
	return average(samples->data, samples->size);
}

// assigns to each point in samples its closest center
// each index K in assignments represents the center of the Kth point in samples.
// returns if the assignment was changed or not
bool updateAssignments(SampleBuffer* samples, float* assignments, float high, float low)
{
	int i;
	bool updated = false;

	for (i = 0; i < samples->size; ++i)
	{
		float p = samples->data[i];
		float a = fabsf(p - high) < fabsf(p - low) ? high : low;
		if (assignments[i] != a)
		{
			updated = true;
			assignments[i] = a;
		}
	}
	
	return updated;
}

void calculateNewCenters(SampleBuffer* samples, float* assignments, float* high, float* low)
{
	int i;
	int highCount = 0, lowCount = 0;
	int highSum = 0, lowSum = 0;

	for (i = 0; i < samples->size; ++i)
	{
		if (assignments[i] == *high)
		{
			highCount++;
			highSum += samples->data[i];
		}
		else
		{
			lowCount++;
			lowSum += samples->data[i];
		}
	}

	*low = (float)lowSum / lowCount;
	*high = (float)highSum / highCount;
}

void findMinMax(SampleBuffer* samples, float* min, float* max)
{
	float t_max = samples->data[0];
	float t_min = samples->data[0];
	int i;

	for (i = 1; i < samples->size; ++i)
	{
		if (samples->data[i] > t_max)
			t_max = samples->data[i];
		if (samples->data[i] < t_min)
			t_min = samples->data[i];
	}

	if (max != NULL) *max = t_max;
	if (min != NULL) *min = t_min;
}

// 1D K-means to find high and low values.
// This assumes the signal is composed of two discrete voltage levels with minor additive noise.
void findHighLow(SampleBuffer* samples, float* high, float* low)
{
	float t_high, t_low;
	findMinMax(samples, &t_low, &t_high);	// initial guess for centers
	bool improved = false;
	float* assignments = (float*)malloc(sizeof(float) * samples->size);

	do
	{
		improved = updateAssignments(samples, assignments, t_high, t_low);
		if (improved)
		{
			// calcualte new centers
			calculateNewCenters(samples, assignments, &t_high, &t_low);
		}
	} while (improved);

	if (high != NULL) *high = t_high;
	if (low != NULL) *low = t_low;

	free(assignments);
}

float measure_max(SampleBuffer* samples)
{
	float max;
	findMinMax(samples, NULL, &max);
	return max;
}

float measure_min(SampleBuffer* samples)
{
	float min;
	findMinMax(samples, &min, NULL);
	return min;
}

float measure_vpp(SampleBuffer* samples)
{
	float min, max;
	findMinMax(samples, &min, &max);
	return max - min;
}

float measure_amplitude(SampleBuffer* samples)
{
	float vpp = measure_vpp(samples);
	return vpp / 2;
}

// returns the average number of samples it takes
// the signal to rise from 10% to 90% of high value
float measure_rise_time(SampleBuffer* samples)
{
	float lowValue, highValue;
	findHighLow(samples, &highValue, &lowValue);
	float low = 0.1 * highValue, high = 0.9 * highValue;
	int i;

	int riseCount = 0;
	int riseSamples = 0;
	
	bool counting = FALSE;
	float lastSample = samples->data[0];
	int count = 0;
	for (i = 1; i < samples->size; ++i)
	{
		float sample = samples->data[i];
		if (counting)
		{
			count++;

			// see if we crossed/reached high
			if ((lastSample < high) && (sample >= high))
			{
				counting = FALSE;
				riseCount++;
				riseSamples += count;
				count = 0;
			}
		}
		else
		{
			// see if we crossed low
			if ((lastSample <= low) && (sample > low))
			{
				counting = TRUE;
				if ((lastSample < high) && (sample >= high))
				{
					counting = FALSE;
					riseCount++;
					riseSamples += 1;
				}
			}
		}

		lastSample = sample;
	}

	if (riseCount > 0)
	{
		Scope* scope = scope_get();
		return scope->screen.dt * (float)riseSamples / riseCount;
	}

	return 0;
}

// returns the average number of samples it takes
// the signal to fall from 90% to 10% of high value
float measure_fall_time(SampleBuffer* samples)
{
	float highValue;
	findHighLow(samples, &highValue, NULL);
	float low = 0.1 * highValue, high = 0.9 * highValue;
	int i;

	int fallCount = 0;
	int fallSamples = 0;

	bool counting = FALSE;
	float lastSample = samples->data[0];
	int count = 0;
	for (i = 1; i < samples->size; ++i)
	{
		float sample = samples->data[i];
		if (counting)
		{
			count++;

			// see if we crossed/reached low on a way down
			if ((lastSample > low) && (sample <= low))
			{
				counting = FALSE;
				fallCount++;
				fallSamples += count;
				count = 0;
			}
		}
		else
		{
			// see if we crossed high on a way down
			if ((lastSample >= high) && (sample < high))
			{
				counting = TRUE;
				if ((lastSample > low) && (sample <= low))
				{
					counting = FALSE;
					fallCount++;
					fallSamples += 1;
				}
			}
		}

		lastSample = sample;
	}

	if (fallCount > 0)
	{
		Scope* scope = scope_get();
		return scope->screen.dt * (float)fallSamples / fallCount;
	}

	return 0;
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

float* digitize(SampleBuffer* samples)
{
	float* assignments = (float*)malloc(sizeof(float) * samples->size);
	float high, low;
	int i;

	findHighLow(samples, &high, &low);
	updateAssignments(samples, assignments, high, low);
	for (i = 0; i < samples->size; ++i)
	{
		assignments[i] = assignments[i] == high ? 1 : 0;
	}

	return assignments;
}

float measure_dutyCycle(SampleBuffer* samples)
{
	float* digitized = digitize(samples);
	float avg = average(digitized, samples->size);
	free(digitized);
	return avg * 100;
}

float measure_high(SampleBuffer* samples)
{
	float high;
	findHighLow(samples, &high, NULL);
	return high;
}

float measure_low(SampleBuffer* samples)
{
	float low;
	findHighLow(samples, NULL, &low);
	return low;
}

// we ignore the DC component here
float measure_frequency(SampleBuffer* samples)
{
	int i, max_index = 1;
	float max;

	// compute DFT
	SampleBuffer* result = sample_buffer_create(samples->size);
	math_trace_fft_amplitude(samples, result);
	
	max = result->data[1];
	// find best bin
	for (i = 1; i < result->size/2+1; ++i)
	{
		if (result->data[i] > max)
		{
			max = result->data[i];
			max_index = i;
		}
	}

	free(result->data);

	float f = math_get_frequency(max_index);
	return f;
}