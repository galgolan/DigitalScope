#include <gtk-3.0\gtk\gtk.h>

#include "common.h"
#include "scope.h"

static Scope scope;

void channel_create(AnalogChannel* channel)
{
	channel->buffer = malloc(sizeof(SampleBuffer));
	channel->enabled = TRUE;
	channel->scale = 1;
}

void trace_create(Trace* trace, cairo_pattern_t* pattern, SampleBuffer* samples)
{
	trace->offset = 0;
	trace->trace_width = 1;
	trace->pattern = pattern;
	trace->samples = samples;
	trace->visible = TRUE;
}

void screen_init()
{
	// setup screen
	scope.screen.background = cairo_pattern_create_rgb(0, 0, 0);
	scope.screen.dt = 1e-3;	// 1ms
	scope.screen.dv = 1;		// 1v/div
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
	trace_create(&scope.screen.traces[0], cairo_pattern_create_rgb(1, 0, 0), scope.channels[0].buffer);	
	trace_create(&scope.screen.traces[1], cairo_pattern_create_rgb(0, 1, 0), scope.channels[1].buffer);
}

Scope* screen_get()
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

	// horizontal lines
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

void trace_draw(const Trace* trace, GtkWidget *widget, cairo_t *cr)
{
	int i, height, width;
	height = gtk_widget_get_allocated_height(widget);
	width = gtk_widget_get_allocated_width(widget);

	for (i = 0; i < MIN(BUFFER_SIZE, width); ++i)
	{
		int y = (int)(trace->samples->data[i] * 10 + height / 2 + trace->offset);
		cairo_line_to(cr, i, y);
	}

	cairo_set_source(cr, trace->pattern);
	cairo_set_line_width(cr, trace->trace_width);
	cairo_stroke(cr);
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