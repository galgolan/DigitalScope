#include <Windows.h>

#include "scope.h"
#include "drawing.h"
#include "scope_ui_handlers.h"
#include "config.h"
#include "threads.h"

#define LOW_FPS	15

static cairo_surface_t* drawing_surface = NULL;
static cairo_t*	drawing_context = NULL;

static HANDLE hSurfaceMutex = INVALID_HANDLE_VALUE;

int drawing_get_width()
{
	return cairo_image_surface_get_width(cairo_get_target(drawing_context));
}

int drawing_get_height()
{
	return cairo_image_surface_get_height(cairo_get_target(drawing_context));
}

float inverse_translate(int y, const Trace* trace)
{
	Scope* scope = scope_get();
	int height = scope->screen.height;
	int max = scope->screen.maxVoltage;
	float v = (trace->offset + (float)height / 2.0f - y) * (2.0f * max / height / trace->scale);
	return v;
}

int translate(float value, const Trace* trace)
{
	Scope* scope = scope_get();
	int height = scope->screen.height;
	int max = scope->screen.maxVoltage;
	int y = (int)(trace->offset - height * trace->scale / 2.0f / max * (value - (float)max / trace->scale));
	return y;
}

// returns value1-value2
float inverse_translate_diff(int value1, int value2, const Trace* trace, int gridLines, int height)
{
	float v1= inverse_translate(value1, trace);
	float v2 = inverse_translate(value2, trace);
	return v1 - v2;
}

void screen_draw_horizontal_line(int y, cairo_pattern_t* pattern, double stroke_width, bool dashes)
{
	int width = drawing_get_width();

	cairo_set_source(drawing_context, pattern);
	cairo_set_line_width(drawing_context, stroke_width);

	cairo_move_to(drawing_context, 0, y);
	cairo_line_to(drawing_context, width, y);

	if (dashes)
	{
		double dashes[1] = { 1.0 };
		cairo_set_dash(drawing_context, dashes, 1, 0);
	}

	cairo_stroke(drawing_context);
	cairo_set_dash(drawing_context, NULL, 0, 0);
}

void screen_draw_vertical_line(int x, cairo_pattern_t* pattern, double stroke_width, bool dashes)
{
	int height = drawing_get_height();

	cairo_set_source(drawing_context, pattern);
	cairo_set_line_width(drawing_context, stroke_width);

	cairo_move_to(drawing_context, x, 0);
	cairo_line_to(drawing_context, x, height);

	if (dashes)
	{
		double dashes[1] = { 1.0 };
		cairo_set_dash(drawing_context, dashes, 1, 0);
	}
	cairo_stroke(drawing_context);
	cairo_set_dash(drawing_context, NULL, 0, 0);
}

void drawing_resize(int width, int height)
{
	if (drawing_context != NULL)
		cairo_destroy(drawing_context);

	drawing_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	drawing_context = cairo_create(drawing_surface);
}

void drawing_buffer_init()
{
	static gboolean first = TRUE;
	
	Scope* scope = scope_get();
	int window_width = scope->screen.width;
	int window_height = scope->screen.height;

	if ((first == TRUE)
		|| (window_width != drawing_get_width())
		|| (window_height != drawing_get_height())
		)
	{
		// prepare surface for drawing
		drawing_resize(window_width, window_height);
	}

	first = FALSE;
}

// paints the dest context from the buffer
void drawing_copy_from_buffer(cairo_t* dest)
{
	cairo_set_source_surface(dest, drawing_surface, 0, 0);
	cairo_paint(dest);
}

// fires a draw event
void drawing_redraw()
{
	Scope* scope = scope_get();
	ScopeUI* ui = common_get_ui();
	GdkWindow* window = gtk_widget_get_window((GtkWidget*)ui->drawingArea);

	GdkRectangle rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = scope->screen.width;
	rect.height = scope->screen.height;
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

	int zero = translate(0, trace);
	cairo_move_to(drawing_context, 0, zero - triangleHeight / 2);
	cairo_line_to(drawing_context, triangleWidth, zero);
	cairo_line_to(drawing_context, 0, zero + triangleHeight / 2);

	cairo_set_source(drawing_context, trace->pattern);
	cairo_fill(drawing_context);
}

void draw_cursors()
{
	Scope* scope = scope_get();
	if (scope->cursors.visible == FALSE)
		return;

	cairo_pattern_t* cursorPattern = cairo_pattern_create_rgb(1, 1, 1);	// TODO: read from css

	screen_draw_horizontal_line(scope->cursors.y1.position, cursorPattern, 1, FALSE);
	screen_draw_horizontal_line(scope->cursors.y2.position, cursorPattern, 1, FALSE);
	screen_draw_vertical_line(scope->cursors.x1.position, cursorPattern, 1, FALSE);
	screen_draw_vertical_line(scope->cursors.x2.position, cursorPattern, 1, FALSE);
}

void screen_draw_grid()
{
	Scope* scope = scope_get();
	int width = drawing_get_width();
	int height = drawing_get_height();

	int i;

	// horizontal lines
	for (i = 1; i < scope->screen.grid.horizontal; ++i)
	{
		int y = i * height / scope->screen.grid.horizontal;
		screen_draw_horizontal_line(y, scope->screen.grid.linePattern, scope->screen.grid.stroke_width, TRUE);		
	}

	// vertical lines
	for (i = 1; i < scope->screen.grid.vertical; ++i)
	{
		int x = i * width / scope->screen.grid.vertical;
		screen_draw_vertical_line(x, scope->screen.grid.linePattern, scope->screen.grid.stroke_width, TRUE);
	}
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
		translate(x, xTrace),
		translate(y, yTrace));
	for (i = 1; i < scope->bufferSize; ++i)
	{
		x = xChannel->buffer->data[i];
		y = yChannel->buffer->data[i];

		cairo_line_to(drawing_context,
			translate(x, xTrace),
			translate(y, yTrace));
	}

	cairo_pattern_t* pattern = cairo_pattern_create_rgb(0, 0, 1);	// TODO: read from css
	cairo_set_source(drawing_context, pattern);
	cairo_set_line_width(drawing_context, 1);	// TODO: use style
	cairo_stroke(drawing_context);
}

void draw_trigger_line()
{
	Scope* scope = scope_get();

	if (scope->screen.showTrigger)
	{
		const Trace* sourceTrace;
		
		switch (scope->trigger.source)
		{
		case TRIGGER_SOURCE_CH1:
			sourceTrace = scope_trace_get_nth(0);
			break;
		case TRIGGER_SOURCE_CH2:
			sourceTrace = scope_trace_get_nth(1);
			break;
		}

		int y = translate(scope->trigger.level, sourceTrace);
		screen_draw_horizontal_line(y, sourceTrace->pattern, 1, TRUE);
	}
}

void trace_draw(const Trace* trace)
{
	int i;
	int width = drawing_get_width();
	int height = drawing_get_height();
	Scope* scope = scope_get();

	for (i = 0; i < MIN(scope->bufferSize, width); ++i)
	{
		int y = translate(trace->samples->data[i], trace);
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

	TRY_LOCK(, "cant aquire lock on traces", scope->screen.hTracesMutex, 100)

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

	ReleaseMutex(scope->screen.hTracesMutex);
}

// draws to the internal buffer
bool drawing_update_buffer()
{
	TRY_LOCK(FALSE, "cant aquire lock on drawing buffer", hSurfaceMutex, 100);

	drawing_buffer_init();

	screen_fill_background();
	screen_draw_grid();
	screen_draw_traces();
	draw_cursors();
	draw_trigger_line();

	cairo_surface_flush(drawing_surface);

	ReleaseMutex(hSurfaceMutex);

	return TRUE;
}

DWORD WINAPI drawing_worker_thread(LPVOID param)
{
	Scope* scope = scope_get();
	DWORD drawingInterval = (DWORD)(1.0f / scope->screen.fps * 1000);
	
	hSurfaceMutex = CreateMutexSimple();
	if (hSurfaceMutex == INVALID_HANDLE_VALUE)
	{
		// handle error
		return 1;
	}

	while (!scope->shuttingDown)
	{
		drawing_update_buffer();
		guint sourceId = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE, timeout_callback, NULL, NULL);
		Sleep(drawingInterval);
	}

	return 0;
}

bool drawing_save_screen_as_image(char* filename)
{
	TRY_LOCK(FALSE, "cant aquire lock on drawing buffer", hSurfaceMutex, 200)	
	cairo_status_t status = cairo_surface_write_to_png(drawing_surface, filename);
	ReleaseMutex(hSurfaceMutex);

	if (status != CAIRO_STATUS_SUCCESS)
	{
		// something went wrong
		return FALSE;
	}

	return TRUE;
}