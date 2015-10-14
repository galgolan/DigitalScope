#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdbool.h>

#define BUFFER_SIZE	2048		// TODO: match this to the Scope's buffer ?

typedef struct SampleBuffer
{
	float data[BUFFER_SIZE];
	int size;
} SampleBuffer;

typedef float(CalcFunc)(SampleBuffer* samples);

typedef enum CursorType
{
	CURSOR_TYPE_HORIZONTAL,
	CURSOR_TYPE_VERTICAL
} CursorType;

typedef struct Cursor
{
	CursorType type;
	int position;
} Cursor;

typedef struct Cursors
{
	Cursor x1;
	Cursor x2;
	Cursor y1;
	Cursor y2;

	bool visible;
} Cursors;

typedef struct Measurement
{
	CalcFunc* measure;
	char* name;
} Measurement;

typedef struct Grid
{
	cairo_pattern_t* linePattern;
	float stroke_width;
	int horizontal;
	int vertical;
} Grid;

typedef struct Trace
{
	cairo_pattern_t* pattern;
	SampleBuffer* samples;
	float trace_width;
	int offset;
	bool visible;
	float scale;
	char* name;
} Trace;

typedef struct MeasurementInstance
{
	Measurement* measurement;
	Trace* trace;
} MeasurementInstance;

typedef struct Screen
{
	cairo_pattern_t* background;

	float dt;	// samples/sec
	//float dv;	// volts/div

	Grid grid;

	Trace* traces;
	int num_traces;
	short fps;
} Screen;

// describes an analog channel
typedef struct AnalogChannel
{
	bool enabled;
	SampleBuffer* buffer;
} AnalogChannel;

typedef enum TriggerMode
{
	TRIGGER_FREERUNNING,
	TRIGGER_SINGLE,
	TRIGGER_AUTO
} TriggerMode;

typedef enum TriggerSource
{
	TRIGGER_SOURCE_CH1,
	TRIGGER_SOURCE_CH2
} TriggerSource;

typedef enum TriggerType
{
	TRIGGER_TYPE_RAISING,
	TRIGGER_TYPE_FALLING,
	TRIGGER_TYPE_BOTH
} TriggerType;

typedef struct Trigger
{
	TriggerMode mode;
	TriggerType type;
	TriggerSource source;
	float level;
} Trigger;

typedef enum ScopeState
{
	SCOPE_STATE_RUNNING = 1,
	SCOPE_STATE_PAUSED = 0
} ScopeState;

typedef enum DisplayMode
{
	DISPLAY_MODE_WAVEFORM,
	DISPLAY_MODE_XY
} DisplayMode;

typedef struct Scope
{
	Screen screen;
	AnalogChannel* channels;
	int num_channels;
	Trigger trigger;
	MeasurementInstance* measurements;
	int num_measurements;
	ScopeState state;	
	Cursors cursors;
	DisplayMode display_mode;
} Scope;

void trace_draw(const Trace* trace, GtkWidget *widget, cairo_t *cr);

void screen_init();

Scope* scope_get();

void screen_draw_grid(GtkWidget *widget, cairo_t *cr);

void screen_draw_traces(GtkWidget *widget, cairo_t *cr);

void screen_fill_background(GtkWidget *widget, cairo_t *cr);

void screen_add_measurement(char* name, char* source, double value);

void screen_clear_measurements();

void draw_cursors(GtkWidget *widget, cairo_t *cr);

// redraws the screen immediately if the FPS is low. Ignores the redraw if the FPS is high.
void request_redraw();

// forces a redraw of the scope screen
void force_redraw();

#endif
