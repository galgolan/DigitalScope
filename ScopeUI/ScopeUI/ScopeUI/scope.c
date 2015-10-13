#include <gtk-3.0\gtk\gtk.h>

#include <Windows.h>
#include <math.h>

#include "common.h"
#include "scope.h"
#include "measurement.h"
#include "scope_ui_handlers.h"

static Scope scope;

void channel_create(AnalogChannel* channel)
{
	channel->buffer = malloc(sizeof(SampleBuffer));
	channel->buffer->size = BUFFER_SIZE;
	channel->enabled = TRUE;
}

void trace_create(Trace* trace, cairo_pattern_t* pattern, SampleBuffer* samples, const char* name)
{
	trace->offset = 0;
	trace->trace_width = 1;
	trace->pattern = pattern;
	trace->samples = samples;
	trace->visible = TRUE;
	trace->scale = 1;
	trace->name = name;
}

void add_measurement(int i, Measurement* measurement, Trace* source)
{
	ScopeUI* ui = common_get_ui();

	// append to tree view
	GtkTreeIter iter;
	gtk_list_store_append(ui->listMeasurements, &iter);
	gtk_list_store_set(ui->listMeasurements, &iter,
		0, Measurement_Average.name,
		1, source->name,
		2, 0.0f,
		-1);

	// append to scope
	scope.measurements[i].measurement = measurement;
	scope.measurements[i].trace = source;
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
		guint timeout_id = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT, timeout_callback, NULL, NULL);
		lastDrawTs = GetTickCount();
	}
	else
	{
		// this can create a 10% error in fps, we are fine with this
		Sleep(refreshMs / 10);
	}
}

DWORD WINAPI serial_worker_thread(LPVOID param)
{
	// TODO: open serial port

	unsigned long long n = 0;	// sample counter for simulating signals

	while (TRUE)
	{
		float T = scope.screen.dt;		// create local copy of sample time

		// fill channels with samples
		for (int i = 0; i < BUFFER_SIZE; ++i)
		{
			scope.channels[0].buffer->data[i] = sin(100e3 * n * T);
			scope.channels[1].buffer->data[i] = sin(0.2*n*T) * - 3 * sin(5e3 * n * T);

			++n;
		}

		// signal screen redraw if enough time has passed	
		redraw_if_needed();
	}

	// TODO: close serial port
}

void screen_init()
{
	// setup screen
	scope.screen.background = cairo_pattern_create_rgb(0, 0, 0);
	scope.screen.dt = 1e-3;	// 1ms
	scope.screen.fps = 20;
//	scope.screen.dv = 1;		// 1v/div
	scope.screen.grid.linePattern = cairo_pattern_create_rgb(0.5, 0.5, 0.5);
	scope.screen.grid.horizontal = 8;
	scope.screen.grid.vertical = 14;
	scope.screen.grid.stroke_width = 1;

	// create traces for 2 analog channels
	scope.num_channels = 2;
	scope.channels = malloc(sizeof(AnalogChannel) * scope.num_channels);
	channel_create(&scope.channels[0]);
	channel_create(&scope.channels[1]);

	scope.screen.num_traces = scope.num_channels;
	scope.screen.traces = malloc(sizeof(Trace) * scope.screen.num_traces);
	trace_create(&scope.screen.traces[0], cairo_pattern_create_rgb(0, 1, 0), scope.channels[0].buffer, "CH1");	
	trace_create(&scope.screen.traces[1], cairo_pattern_create_rgb(1, 0, 0), scope.channels[1].buffer, "CH2");

	// add default measurements
	scope.num_channels = 2;
	scope.measurements = malloc(sizeof(MeasurementInstance) * scope.num_measurements);
	add_measurement(0, &Measurement_Average, &scope.screen.traces[0]);
	add_measurement(1, &Measurement_Average, &scope.screen.traces[1]);	

	// create serial worker thread
	long serialThreadId;
	HANDLE hSerialThread = CreateThread(NULL, 0, serial_worker_thread, NULL, 0, &serialThreadId);
	if (hSerialThread == INVALID_HANDLE_VALUE)
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

void trace_draw(const Trace* trace, GtkWidget *widget, cairo_t *cr)
{
	int i, height, width;
	height = gtk_widget_get_allocated_height(widget);
	width = gtk_widget_get_allocated_width(widget);

	for (i = 0; i < MIN(BUFFER_SIZE, width); ++i)
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
	for (i = 0; i < scope.screen.num_traces; ++i)
	{
		if (scope.screen.traces[i].visible)
		{
			trace_draw(&scope.screen.traces[i], widget, cr);
		}
	}
}