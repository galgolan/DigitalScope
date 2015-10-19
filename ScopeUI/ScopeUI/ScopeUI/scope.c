#define _CRT_SECURE_NO_WARNINGS

#include <gtk-3.0\gtk\gtk.h>
#include <glib-2.0\glib.h>

#include <Windows.h>
#include <math.h>
#include <stdio.h>

#include "common.h"
#include "scope.h"
#include "measurement.h"
#include "trace_math.h"
#include "scope_ui_handlers.h"
#include "config.h"

static Scope scope;

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

	return channel;
}

AnalogChannel* scope_channel_get_nth(int n)
{
	return (AnalogChannel*)g_queue_peek_nth(scope.channels, n);
}

Trace* scope_trace_add_new(cairo_pattern_t* pattern, SampleBuffer* samples, const char* name, int offset)
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
		guint source_id = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT, timeout_callback, NULL, NULL);
		lastDrawTs = GetTickCount();
	}
	else
	{
		// this can create a 10% error in fps, we are fine with this
		Sleep((DWORD)refreshMs / 10);
	}
}

DWORD WINAPI serial_worker_thread(LPVOID param)
{
	// TODO: open serial port

	unsigned long long n = 0;	// sample counter for simulating signals

	AnalogChannel* ch1 = scope_channel_get_nth(0);
	AnalogChannel* ch2 = scope_channel_get_nth(1);

	while (TRUE)
	{
		float T = scope.screen.dt;		// create local copy of sample time

		if (scope.state == SCOPE_STATE_PAUSED)
		{
			Sleep(100);
			continue;
		}

		Sleep(10);	// throttle down to simulate serial port

		// fill channels with samples
		for (int i = 0; i < scope.bufferSize; ++i)
		{
			//ch2->buffer->data[i] = sin(0.2*n*T) * - 3 * sin(5e3 * n * T);
			ch1->buffer->data[i] = (float)sin(100e3 * n * T) >= 0.0f ? 1.0f : 0.0f;	// square wave 100KHz
			ch2->buffer->data[i] = 2 * (float)sin(200e3 * n * T + G_PI/4);	// cosine 200KHz

			++n;
		}

		math_update_trace();	// TODO: move to other thread

		// signal screen redraw if enough time has passed	
		redraw_if_needed();
	}

	// TODO: close serial port
}

void cursors_init()
{
	scope.cursors.visible = FALSE;
	scope.cursors.x1.type = CURSOR_TYPE_HORIZONTAL;
	scope.cursors.x2.type = CURSOR_TYPE_HORIZONTAL;
	scope.cursors.y1.type = CURSOR_TYPE_VERTICAL;
	scope.cursors.y2.type = CURSOR_TYPE_VERTICAL;

	scope.cursors.x1.position = 100;
	scope.cursors.x2.position = 100;
	scope.cursors.y1.position = 100;
	scope.cursors.y2.position = 100;
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
		g_queue_push_tail(traceNames, t->name);
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
		int offset = offsetIt == NULL ? 0 : GPOINTER_TO_INT(offsetIt->data);
		scope_trace_add_new(tracePatterns[i], scope_channel_get_nth(i)->buffer, traceName, offset);
		offsetIt = offsetIt->next;
	}
	
	// create the math trace
	Trace* mathTrace = scope_trace_add_new(tracePatterns[numChannels], sample_buffer_create(scope.bufferSize), "Math", GPOINTER_TO_INT(offsetIt->data));
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

void screen_draw_grid(GtkWidget *widget, cairo_t *cr)
{
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);

	int i;

	// set grid line style
	cairo_set_source(cr, scope.screen.grid.linePattern);
	cairo_set_line_width(cr, scope.screen.grid.stroke_width);

	// horizontal lines
	for (i = 0; i < scope.screen.grid.horizontal; ++i)
	{
		int y = i * height / scope.screen.grid.horizontal;
		cairo_move_to(cr, 0, y);
		cairo_line_to(cr, width, y);
		cairo_stroke(cr);
	}

	// vertical lines
	for (i = 0; i < scope.screen.grid.vertical; ++i)
	{
		int x = i * width / scope.screen.grid.vertical;
		cairo_line_to(cr, x, 0);
		cairo_line_to(cr, x, height);		
		cairo_stroke(cr);
	}
}

void screen_fill_background(GtkWidget *widget, cairo_t *cr)
{
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);

	cairo_rectangle(cr, 0, 0, width, height);
	cairo_set_source(cr, scope.screen.background);
	cairo_fill(cr);
}

void draw_ground_marker(const Trace* trace, GtkWidget* widget, cairo_t* cr)
{
	int height = gtk_widget_get_allocated_height(widget);
	int width = gtk_widget_get_allocated_width(widget);

	int triangleHeight = 9, triangleWidth = 8;

	cairo_move_to(cr, 0, height / 2 + trace->offset - triangleHeight / 2);
	cairo_line_to(cr, triangleWidth, height / 2 + trace->offset);
	cairo_line_to(cr, 0, height / 2 + trace->offset + triangleHeight / 2);
	
	cairo_set_source(cr, trace->pattern);
	cairo_fill(cr);
}

void draw_cursor(const Cursor* cursor, GtkWidget *widget, cairo_t *cr)
{
	cairo_pattern_t* cursorPattern = cairo_pattern_create_rgb(1, 1, 1);	// TODO: read from css
	int height = gtk_widget_get_allocated_height(widget);
	int width = gtk_widget_get_allocated_width(widget);

	double src_x, src_y, dst_x, dst_y;

	if (cursor->type == CURSOR_TYPE_HORIZONTAL)
	{
		src_x = cursor->position;
		src_y = 0;
		dst_x = src_x;
		dst_y = height;
	}
	else
	{
		src_x = 0;
		src_y = cursor->position;
		dst_x = width;
		dst_y = src_y;
	}

	cairo_move_to(cr, src_x, src_y);
	cairo_line_to(cr, dst_x, dst_y);
	cairo_set_source(cr, cursorPattern);
	cairo_set_line_width(cr, 1);	// TODO: use style
	cairo_stroke(cr);
}

void draw_cursors(GtkWidget *widget, cairo_t *cr)
{
	if (scope.cursors.visible == FALSE)
		return;

	draw_cursor(&scope.cursors.x1, widget, cr);
	draw_cursor(&scope.cursors.x2, widget, cr);
	draw_cursor(&scope.cursors.y1, widget, cr);
	draw_cursor(&scope.cursors.y2, widget, cr);
}

int translate(float value, const Trace* trace, int gridLines, int height, int width)
{
	int c = (int)(height / 2 + trace->offset - (value / trace->scale) * (height / gridLines));
	return c;
}

void screen_draw_xy(GtkWidget *widget, cairo_t *cr)
{
	int i;
	int height = gtk_widget_get_allocated_height(widget);
	int width = gtk_widget_get_allocated_width(widget);

	// channel1 - x, channel2 - y
	AnalogChannel* xChannel = scope_channel_get_nth(0);
	AnalogChannel* yChannel = scope_channel_get_nth(1);
	Trace* xTrace = scope_trace_get_nth(0);
	Trace* yTrace = scope_trace_get_nth(1);

	float x = xChannel->buffer->data[0];
	float y = yChannel->buffer->data[0];

	cairo_move_to(cr,
		translate(x, xTrace, scope.screen.grid.horizontal, height, width),
		translate(y, yTrace, scope.screen.grid.vertical, height, width));
	for (i = 1; i < scope.bufferSize; ++i)
	{
		x = xChannel->buffer->data[i];
		y = yChannel->buffer->data[i];
				
		cairo_line_to(cr,
			translate(x, xTrace, scope.screen.grid.horizontal, height, width),
			translate(y, yTrace, scope.screen.grid.vertical, height, width));
	}

	cairo_pattern_t* pattern = cairo_pattern_create_rgb(0, 0, 1);	// TODO: read from css
	cairo_set_source(cr, pattern);
	cairo_set_line_width(cr, 1);	// TODO: use style
	cairo_stroke(cr);
}

void trace_draw(const Trace* trace, GtkWidget *widget, cairo_t *cr)
{
	int i, height, width;
	height = gtk_widget_get_allocated_height(widget);
	width = gtk_widget_get_allocated_width(widget);

	for (i = 0; i < MIN(scope.bufferSize, width); ++i)
	{
		// y = offset - (sample/scale * height/grid.h)
		int y = (int)(height/2 + trace->offset - (trace->samples->data[i] / trace->scale)*(height / scope.screen.grid.horizontal));
		cairo_line_to(cr, i, y);
	}

	cairo_set_source(cr, trace->pattern);
	cairo_set_line_width(cr, trace->trace_width);
	cairo_stroke(cr);

	draw_ground_marker(trace, widget, cr);
}

void screen_draw_traces(GtkWidget *widget, cairo_t *cr)
{
	int i;
	int numTraces = g_queue_get_length(scope.screen.traces);

	if (scope.display_mode == DISPLAY_MODE_WAVEFORM)
	{
		for (i = 0; i < numTraces; ++i)
		{
			Trace* trace = scope_trace_get_nth(i);	// TODO: use iterator
			if (trace->visible)
			{
				trace_draw(trace, widget, cr);
			}
		}
	}
	else if (scope.display_mode == DISPLAY_MODE_XY)
	{
		screen_draw_xy(widget, cr);
	}
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