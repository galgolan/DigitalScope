#include "common.h"
#include "scope.h"
#include "drawing.h"

#define LOW_FPS	15

static cairo_surface_t* drawing_surface = NULL;
static cairo_t*	drawing_context = NULL;

void drawing_resize(int width, int height)
{
	cairo_destroy(drawing_context);
	drawing_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	drawing_context = cairo_create(drawing_surface);
}

void drawing_buffer_init()
{
	static gboolean first = TRUE;
	if (first == FALSE)
		return;

	ScopeUI* ui = common_get_ui();

	// prepare surface for drawing
	int width = gtk_widget_get_allocated_width(ui->drawingArea);
	int  height = gtk_widget_get_allocated_height(ui->drawingArea);

	drawing_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	drawing_context = cairo_create(drawing_surface);

	first = FALSE;
}

int drawing_get_width()
{
	return cairo_image_surface_get_width(cairo_get_target(drawing_context));
}

int drawing_get_height()
{
	int height = cairo_image_surface_get_height(cairo_get_target(drawing_context));
}


// paints the dest context from the buffer
void drawing_copy_from_buffer(cairo_t* dest)
{
	cairo_set_source_surface(dest, drawing_surface, 0, 0);
	cairo_paint(dest);
}

// fires a draw event if needed
void drawing_request_redraw()
{
	Scope* scope = scope_get();
	// TODO: implement a dirty flag
	if ((scope->screen.fps < LOW_FPS) || (scope->state == SCOPE_STATE_PAUSED))
		drawing_redraw();
}

// fires a draw event
void drawing_redraw()
{
	ScopeUI* ui = common_get_ui();
	GdkWindow* window = gtk_widget_get_window(ui->drawingArea);

	GdkRectangle rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = gtk_widget_get_allocated_width(ui->drawingArea);
	rect.height = gtk_widget_get_allocated_height(ui->drawingArea);
	drawing_update_buffer();
	gdk_window_invalidate_rect(window, &rect, FALSE);
}

void screen_fill_background()
{
	int width = drawing_get_width();
	int height = drawing_get_height();

	Scope* scope = scope_get();

	cairo_rectangle(drawing_context, 0, 0, width, height);
	cairo_set_source(drawing_context, scope->screen.background);
	cairo_fill(drawing_context);
}

void draw_ground_marker(const Trace* trace)
{
	int width = drawing_get_width();
	int height = drawing_get_height();

	int triangleHeight = 9, triangleWidth = 8;

	cairo_move_to(drawing_context, 0, height / 2 + trace->offset - triangleHeight / 2);
	cairo_line_to(drawing_context, triangleWidth, height / 2 + trace->offset);
	cairo_line_to(drawing_context, 0, height / 2 + trace->offset + triangleHeight / 2);

	cairo_set_source(drawing_context, trace->pattern);
	cairo_fill(drawing_context);
}

void draw_cursor(const Cursor* cursor)
{
	cairo_pattern_t* cursorPattern = cairo_pattern_create_rgb(1, 1, 1);	// TODO: read from css
	int width = drawing_get_width();
	int height = drawing_get_height();

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

	cairo_move_to(drawing_context, src_x, src_y);
	cairo_line_to(drawing_context, dst_x, dst_y);
	cairo_set_source(drawing_context, cursorPattern);
	cairo_set_line_width(drawing_context, 1);	// TODO: use style
	cairo_stroke(drawing_context);
}

void draw_cursors()
{
	Scope* scope = scope_get();
	if (scope->cursors.visible == FALSE)
		return;

	draw_cursor(&scope->cursors.x1);
	draw_cursor(&scope->cursors.x2);
	draw_cursor(&scope->cursors.y1);
	draw_cursor(&scope->cursors.y2);
}

void screen_draw_grid()
{
	Scope* scope = scope_get();
	int width = drawing_get_width();
	int height = drawing_get_height();

	int i;

	// set grid line style
	cairo_set_source(drawing_context, scope->screen.grid.linePattern);
	cairo_set_line_width(drawing_context, scope->screen.grid.stroke_width);

	// horizontal lines
	for (i = 0; i < scope->screen.grid.horizontal; ++i)
	{
		int y = i * height / scope->screen.grid.horizontal;
		cairo_move_to(drawing_context, 0, y);
		cairo_line_to(drawing_context, width, y);
		cairo_stroke(drawing_context);
	}

	// vertical lines
	for (i = 0; i < scope->screen.grid.vertical; ++i)
	{
		int x = i * width / scope->screen.grid.vertical;
		cairo_line_to(drawing_context, x, 0);
		cairo_line_to(drawing_context, x, height);
		cairo_stroke(drawing_context);
	}
}

int translate(float value, const Trace* trace, int gridLines, int height, int width)
{
	int c = (int)(height / 2 + trace->offset - (value / trace->scale) * (height / gridLines));
	return c;
}

void screen_draw_xy()
{
	int i;
	int width = drawing_get_width();
	int height = drawing_get_height();

	// channel1 - x, channel2 - y
	AnalogChannel* xChannel = scope_channel_get_nth(0);
	AnalogChannel* yChannel = scope_channel_get_nth(1);
	Trace* xTrace = scope_trace_get_nth(0);
	Trace* yTrace = scope_trace_get_nth(1);
	Scope* scope = scope_get();

	float x = xChannel->buffer->data[0];
	float y = yChannel->buffer->data[0];

	cairo_move_to(drawing_context,
		translate(x, xTrace, scope->screen.grid.horizontal, height, width),
		translate(y, yTrace, scope->screen.grid.vertical, height, width));
	for (i = 1; i < scope->bufferSize; ++i)
	{
		x = xChannel->buffer->data[i];
		y = yChannel->buffer->data[i];

		cairo_line_to(drawing_context,
			translate(x, xTrace, scope->screen.grid.horizontal, height, width),
			translate(y, yTrace, scope->screen.grid.vertical, height, width));
	}

	cairo_pattern_t* pattern = cairo_pattern_create_rgb(0, 0, 1);	// TODO: read from css
	cairo_set_source(drawing_context, pattern);
	cairo_set_line_width(drawing_context, 1);	// TODO: use style
	cairo_stroke(drawing_context);
}

void trace_draw(const Trace* trace)
{
	int i;
	int width = drawing_get_width();
	int height = drawing_get_height();
	Scope* scope = scope_get();

	for (i = 0; i < MIN(scope->bufferSize, width); ++i)
	{
		// y = offset - (sample/scale * height/grid.h)
		int y = (int)(height / 2 + trace->offset - (trace->samples->data[i] / trace->scale)*(height / scope->screen.grid.horizontal));
		cairo_line_to(drawing_context, i, y);
	}

	cairo_set_source(drawing_context, trace->pattern);
	cairo_set_line_width(drawing_context, trace->trace_width);
	cairo_stroke(drawing_context);

	draw_ground_marker(trace);
}

void screen_draw_traces()
{
	int i;
	Scope* scope = scope_get();
	int numTraces = g_queue_get_length(scope->screen.traces);

	if (scope->display_mode == DISPLAY_MODE_WAVEFORM)
	{
		for (i = 0; i < numTraces; ++i)
		{
			Trace* trace = scope_trace_get_nth(i);	// TODO: use iterator
			if (trace->visible)
			{
				trace_draw(trace);
			}
		}
	}
	else if (scope->display_mode == DISPLAY_MODE_XY)
	{
		screen_draw_xy();
	}
}

// draws to the internal buffer
void drawing_update_buffer()
{
	drawing_buffer_init();

	screen_fill_background();
	screen_draw_grid();

	screen_draw_traces();
	draw_cursors();

	cairo_surface_flush(drawing_surface);
}