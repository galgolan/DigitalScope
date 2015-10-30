#define _CRT_SECURE_NO_WARNINGS

#include <gtk-3.0\gtk\gtk.h>
#include <glib-2.0\glib.h>

#include <Windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scope.h"
#include "measurement.h"
#include "trace_math.h"
#include "scope_ui_handlers.h"
#include "config.h"
#include "serial.h"
#include "../../../common/common.h"
#include "protocol.h"

static Scope scope;

bool scope_build_and_send_config()
{
	TriggerConfig trigCfg = TRIGGER_CFG_MODE_NONE;
	switch (scope.trigger.mode)
	{
	case TRIGGER_MODE_NONE:
		trigCfg |= TRIGGER_CFG_MODE_NONE;
		break;
	case TRIGGER_MODE_SINGLE:
		trigCfg |= TRIGGER_CFG_MODE_SINGLE;
		break;
	case TRIGGER_MODE_AUTO:
		trigCfg |= TRIGGER_CFG_MODE_AUTO;
		break;		
	}

	switch (scope.trigger.source)
	{
	case TRIGGER_SOURCE_CH1:
		trigCfg |= TRIGGER_CFG_SRC_CH1;
		break;
	case TRIGGER_SOURCE_CH2:
		trigCfg |= TRIGGER_CFG_SRC_CH2;
		break;
	}

	switch (scope.trigger.type)
	{
	case TRIGGER_TYPE_RAISING:
		trigCfg |= TRIGGER_CFG_TYPE_RAISING;
		break;
	case TRIGGER_TYPE_FALLING:
		trigCfg |= TRIGGER_CFG_TYPE_FALLING;
		break;
	case TRIGGER_TYPE_BOTH:
		trigCfg |= TRIGGER_CFG_TYPE_BOTH;
		break;
	}

	AnalogChannel* ch1 = scope_channel_get_nth(0);
	AnalogChannel* ch2 = scope_channel_get_nth(1);
	Trace* traceCh1 = scope_trace_get_nth(0);
	Trace* traceCh2 = scope_trace_get_nth(1);

	ConfigMsg msg = common_create_config(trigCfg, scope.trigger.level, (byte)traceCh1->scale, traceCh1->offset, (byte)traceCh2->scale, traceCh2->offset, (float)1 / scope.screen.dt);
	return protocol_send_config(&msg);
}

SampleBuffer* sample_buffer_create(int size)
{
	SampleBuffer* buffer = (SampleBuffer*)malloc(sizeof(SampleBuffer));
	buffer->size = size;
	buffer->data = (float*)malloc(sizeof(float) * size);
	// TODO: use memset to zero all data
	return buffer;
}

AnalogChannel* scope_channel_add_new()
{
	AnalogChannel* channel = (AnalogChannel*)malloc(sizeof(AnalogChannel));
	g_queue_push_tail(scope.channels, channel);

	channel->buffer = sample_buffer_create(scope.bufferSize);
	channel->enabled = TRUE;
	channel->probeRatio = 1;

	return channel;
}

AnalogChannel* scope_channel_get_nth(int n)
{
	return (AnalogChannel*)g_queue_peek_nth(scope.channels, n);
}

Trace* scope_trace_add_new(cairo_pattern_t* pattern, SampleBuffer* samples, const char* name, float offset)
{
	Trace* trace = (Trace*)malloc(sizeof(Trace));
	g_queue_push_tail(scope.screen.traces, trace);

	trace->offset = offset;
	trace->trace_width = 1;	// TODO: use style
	trace->pattern = pattern;	// TODO: use style
	trace->samples = samples;
	trace->visible = TRUE;
	trace->scale = 1;
	trace->name = name;

	return trace;
}

Trace* scope_trace_get_nth(int n)
{
	return (Trace*)g_queue_peek_nth(scope.screen.traces, n);
}

guint scope_trace_get_length()
{
	return g_queue_get_length(scope.screen.traces);
}

MeasurementInstance* scope_measurement_get_nth(int n)
{
	return (MeasurementInstance*)g_queue_peek_nth(scope.measurements, n);
}

// returns the math trace
Trace* scope_trace_get_math()
{
	return (Trace*)g_queue_peek_nth(scope.screen.traces, g_queue_get_length(scope.channels));
}

MeasurementInstance* scope_measurement_add(Measurement* measurement, Trace* source)
{
	MeasurementInstance* instance = (MeasurementInstance*)malloc(sizeof(MeasurementInstance));
	g_queue_push_tail(scope.measurements, instance);

	instance->measurement = measurement;
	instance->trace = source;

	return instance;
}

// signal screen redraw if enough time has passed	
void redraw_if_needed()
{
	static DWORD lastDrawTs = 0;
	float refreshMs = 1 / (float)scope.screen.fps * 1000;	// the number of milliseconds to wait between redrawing the screen

	// signal screen redraw if enough time has passed	
	DWORD elapsedMs = GetTickCount() - lastDrawTs;
	if (elapsedMs > refreshMs)
	{
		guint source_id = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE, timeout_callback, NULL, NULL);
		lastDrawTs = GetTickCount();
	}
	else
	{
		// this can create a 10% error in fps, we are fine with this
		//Sleep((DWORD)refreshMs / 10);
	}
}

void serial_worker_demo(AnalogChannel* ch1, AnalogChannel* ch2, float T)
{
	Sleep(10);	// throttle down to simulate serial port

	// fill channels with samples
	for (int i = 0; i < scope.bufferSize; ++i)
	{
		int n = scope.posInBuffer;
		//ch2->buffer->data[i] = ch2->probeRatio * sin(0.2*n*T) * - 3 * sin(5e3 * n * T);
		ch1->buffer->data[i] = (float)sin(100e3 * n * T) >= 0.0f ? ch1->probeRatio : 0.0f;	// square wave 100KHz
		ch2->buffer->data[i] = ch2->probeRatio * 2 * (float)sin(200e3 * n * T + G_PI/4);	// cosine 200KHz

		scope_screen_next_pos();
	}
}

float convert_sample(char* encodedSample)
{
	return (float)atof(encodedSample);
}

int get_pos_in_buffer()
{
	return scope.posInBuffer;
}

void serial_worker_read(char* buffer, int bufferSize, AnalogChannel* ch1, AnalogChannel* ch2)
{
	int bytesRead = serial_read(buffer, bufferSize);
	if (bytesRead == -1)
	{
		// error
		return;
	}

	handle_receive_date(buffer, bytesRead, ch1->buffer->data, ch2->buffer->data, scope_screen_next_pos, get_pos_in_buffer);
}

DWORD WINAPI serial_worker_thread(LPVOID param)
{
	gboolean demoMode = config_get_bool("test", "demo");

	if (!demoMode)
		if (serial_open() == FALSE)
			return 1;

	unsigned long long n = 0;	// sample counter for simulating signals

	static int pos = 0;

	AnalogChannel* ch1 = scope_channel_get_nth(0);
	AnalogChannel* ch2 = scope_channel_get_nth(1);

	int bufferSize = 10 * 5;
	char* buffer = (char*)malloc(sizeof(char) * bufferSize);
	
	while (TRUE)
	{
		
		float T = scope.screen.dt;		// create local copy of sample time

		if (scope.state == SCOPE_STATE_PAUSED)
		{
			Sleep(100);
			continue;
		}

		if (demoMode)
			serial_worker_demo(ch1, ch2, T);
		else
			serial_worker_read(buffer, bufferSize, ch1, ch2);

		math_update_trace();	// TODO: move to other thread

		// signal screen redraw if enough time has passed	
		redraw_if_needed();
	}

	free(buffer);

	if (demoMode)
		return 0;

	return serial_close();
}

void populate_cursors_list()
{
	ScopeUI* ui = common_get_ui();
	GQueue* cursors = g_queue_new();
	GQueue* values = g_queue_new();
	g_queue_push_tail(cursors, scope.cursors.x1.name);
	g_queue_push_tail(cursors, scope.cursors.x2.name);
	g_queue_push_tail(cursors, scope.cursors.y1.name);
	g_queue_push_tail(cursors, scope.cursors.y2.name);
	//g_queue_push_tail(cursors, "dx");
	//g_queue_push_tail(cursors, "dy");
	for (guint i = 0; i < g_queue_get_length(cursors); ++i)
		g_queue_push_tail(values, "0[v]");

	populate_list_store_index_string_string(ui->liststoreCursorValues, cursors, values, TRUE);

	g_queue_free(cursors);
	g_queue_free(values);
}

void cursors_init()
{
	scope.cursors.visible = FALSE;

	Cursor x1 = { .type = CURSOR_TYPE_HORIZONTAL, .name = "x1", .position = 100 };
	scope.cursors.x1 = x1;
	Cursor x2 = { .type = CURSOR_TYPE_HORIZONTAL, .name = "x2", .position = 200 };
	scope.cursors.x2 = x2;
	Cursor y1 = { .type = CURSOR_TYPE_VERTICAL, .name = "y1", .position = 100 };
	scope.cursors.y1 = y1;
	Cursor y2 = { .type = CURSOR_TYPE_VERTICAL, .name = "y2", .position = 200 };
	scope.cursors.y2 = y2;
	
	populate_cursors_list();
}

void populate_probe_list_store()
{
	ScopeUI* ui = common_get_ui();

	GList* ratios = config_get_int_list("hardware", "probeRatios");
	GList* it = ratios;
	
	GQueue* names = g_queue_new();
	GQueue* values = g_queue_new();

	for (guint i = 0; i < g_list_length(ratios); ++i)
	{
		int value = GPOINTER_TO_INT(it->data);
		char* name = malloc(sizeof(char) * 10);
		sprintf(name, "1:%d", value);

		g_queue_push_tail(values, GINT_TO_POINTER(value));		
		g_queue_push_tail(names, name);

		it = it->next;
	}

	populate_list_store_values_int(ui->liststoreProbeRatio, names, values, TRUE);

	g_queue_free(names);
	g_queue_free(values);
	g_list_free(ratios);

	gtk_combo_box_set_active(ui->comboChannel1Probe, 0);
	gtk_combo_box_set_active(ui->comboChannel2Probe, 0);
}

void populate_measurements_combo()
{
	ScopeUI* ui = common_get_ui();

	// populate available measurement types
	GQueue* meas = measurement_get_all();
	GQueue* measNames = g_queue_new();
	for (guint i = 0; i < g_queue_get_length(meas); ++i)
	{
		Measurement* m = g_queue_peek_nth(meas, i);
		g_queue_push_tail(measNames, m->name);
	}
	populate_list_store(ui->measurementTypesList, measNames, TRUE);
	g_queue_free(measNames);
}

void populate_traces_list()
{
	ScopeUI* ui = common_get_ui();

	GQueue* traceNames = g_queue_new();
	for (guint i = 0; i < scope_trace_get_length(); ++i)
	{
		Trace* t = scope_trace_get_nth(i);
		g_queue_push_tail(traceNames, (gpointer)t->name);
	}
	populate_list_store(ui->tracesList, traceNames, TRUE);
	g_queue_free(traceNames);
}

void screen_init()
{
	GError* error = NULL;

	scope.bufferSize = config_get_int("config", "bufferSize");

	scope.state = SCOPE_STATE_RUNNING;
	scope.display_mode = DISPLAY_MODE_WAVEFORM;
	cursors_init();

	// setup screen
	scope.screen.background = cairo_pattern_create_rgb(0, 0, 0);
	scope.screen.dt = 1e-3f;	// 1ms
	scope.screen.fps = config_get_int("display", "fps");
//	scope.screen.dv = 1;		// 1v/div
	scope.screen.grid.linePattern = cairo_pattern_create_rgb(0.5, 0.5, 0.5);
	scope.screen.grid.horizontal = config_get_int("display", "gridlinesHorizontal");
	scope.screen.grid.vertical = config_get_int("display", "gridlinesVertical");
	scope.screen.grid.stroke_width = 1;

	// create analog channels
	scope.channels = g_queue_new();
	int numChannels = config_get_int("hardware", "channels");
	for (int i = 0; i < numChannels; ++i)
		scope_channel_add_new();
	
	populate_probe_list_store();

	// create traces for the analog channels
	scope.screen.traces = g_queue_new();
	GList* offsets = config_get_int_list("display", "defaultOffset");
	cairo_pattern_t* tracePatterns[] = {
		cairo_pattern_create_rgb(0, 1, 0),
		cairo_pattern_create_rgb(1, 0, 0),
		cairo_pattern_create_rgb(0, 0, 1) };

	GList* offsetIt = offsets;
	int count = g_list_length(offsets);
	for (int i = 0; i < numChannels; ++i)
	{
		// generate trace name
		char* traceName = (char*)malloc(sizeof(char) * 10);
		sprintf(traceName, "CH%d", i+1);
		float offset = offsetIt == NULL ? 0.0f : (float)GPOINTER_TO_INT(offsetIt->data);
		scope_trace_add_new(tracePatterns[i], scope_channel_get_nth(i)->buffer, traceName, offset);
		offsetIt = offsetIt->next;
	}
	
	// create the math trace
	Trace* mathTrace = scope_trace_add_new(tracePatterns[numChannels], sample_buffer_create(scope.bufferSize), "Math", (float)GPOINTER_TO_INT(offsetIt->data));
	scope.mathTraceDefinition.mathTrace = &MathTrace_Dft_Amplitude;
	scope.mathTraceDefinition.firstTrace = scope_trace_get_nth(0);
	scope.mathTraceDefinition.secondTrace = scope_trace_get_nth(1);
	mathTrace->visible = FALSE;
	g_list_free(offsets);

	populate_traces_list();
	
	// init measurements
	populate_measurements_combo();
	scope.measurements = g_queue_new();

	// create serial worker thread
	long serialThreadId;
	HANDLE hSerialThread = CreateThread(NULL, 0, serial_worker_thread, NULL, 0, &serialThreadId);
	if (hSerialThread == INVALID_HANDLE_VALUE)
	{
		// TODO: handle error
	}

	long measurementThreadId;
	HANDLE hMeasurementThread = CreateThread(NULL, 0, measurement_worker_thread, NULL, 0, &measurementThreadId);
	if (hMeasurementThread == INVALID_HANDLE_VALUE)
	{
		// TODO: handle error
	}

	update_statusbar();
}

Scope* scope_get()
{
	return &scope;
}

void screen_clear_measurements()
{
	ScopeUI* ui = common_get_ui();
	gtk_list_store_clear(ui->listMeasurements);
}

void screen_add_measurement(const char* name, const char* source, double value, guint id)
{
	ScopeUI* ui = common_get_ui();

	// append to tree view
	GtkTreeIter iter;
	gtk_list_store_append(ui->listMeasurements, &iter);
	gtk_list_store_set(ui->listMeasurements, &iter,
		0, name,
		1, source,
		2, value,
		3, id
		-1);
}

void scope_screen_next_pos()
{
	scope.posInBuffer++;
	if (scope.posInBuffer > scope.bufferSize)
		scope.posInBuffer = 0;
}