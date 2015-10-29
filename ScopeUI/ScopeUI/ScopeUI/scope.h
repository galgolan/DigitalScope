#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdbool.h>
#include <gtk-3.0\gtk\gtk.h>
#include <glib-2.0\glib.h>

typedef struct SampleBuffer
{
	float* data;
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
	char* name;
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
	const char* name;
} Trace;

typedef struct MeasurementInstance
{
	Measurement* measurement;
	Trace* trace;
} MeasurementInstance;

typedef void(MathFunc)(const SampleBuffer* first, const SampleBuffer* second, SampleBuffer* result);

typedef struct MathTrace
{
	MathFunc* function;
	char* name;
} MathTrace;

typedef struct MathTraceInstance
{
	MathTrace* mathTrace;
	Trace* firstTrace;
	Trace* secondTrace;
} MathTraceInstance;

typedef struct Screen
{
	cairo_pattern_t* background;

	float dt;	// sec (sample period time)
	//float dv;	// volts/div

	Grid grid;

	GQueue* traces;	// contains Trace*
	short fps;

	int width;
	int height;
} Screen;

// describes an analog channel
typedef struct AnalogChannel
{
	bool enabled;
	int probeRatio;
	SampleBuffer* buffer;
} AnalogChannel;

typedef enum TriggerMode
{
	TRIGGER_MODE_NONE,
	TRIGGER_MODE_SINGLE,
	TRIGGER_MODE_AUTO
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
	DISPLAY_MODE_WAVEFORM = 0,
	DISPLAY_MODE_XY = 1
} DisplayMode;

typedef struct Scope
{
	Screen screen;
	GQueue* channels;	// contains AnalogChannel*
	Trigger trigger;
	GQueue* measurements;	// contains MeasurementInstance*
	ScopeState state;	
	Cursors cursors;
	DisplayMode display_mode;
	MathTraceInstance mathTraceDefinition;
	int bufferSize;
	int posInBuffer;
} Scope;


void screen_init();

Scope* scope_get();

void screen_add_measurement(const char* name, const char* source, double value, guint id);

void screen_clear_measurements();

bool scope_build_and_send_config();

MeasurementInstance* scope_measurement_get_nth(int n);

MeasurementInstance* scope_measurement_add(Measurement* measurement, Trace* source);

AnalogChannel* scope_channel_get_nth(int n);

Trace* scope_trace_get_nth(int n);

Trace* scope_trace_get_math();

// moves the trace to the next position
void scope_screen_next_pos();

#endif
