#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdbool.h>
#include <gtk-3.0\gtk\gtk.h>
#include <glib-2.0\glib.h>
#include <Windows.h>

#include "formatting.h"

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
	Units units;
} Measurement;

typedef struct Grid
{
	cairo_pattern_t* linePattern;
	float stroke_width;
	int horizontal;		// number of horizontal grid lines, excluding screen edges
	int vertical;		// number of vertical grid lines, excluding screen edges
} Grid;

typedef struct Trace
{
	cairo_pattern_t* pattern;
	SampleBuffer* samples;
	float trace_width;
	int offset;		// units are pixels from the middle of the screen (positive means down)
	bool visible;
	float scale;
	const char* name;

	bool ownsSampleBuffer;

	Units horizontal;
	Units vertical;

	float horizontalScale;	// used only when horizontal != UNITS_TIME
} Trace;

typedef struct MeasurementInstance
{
	Measurement* measurement;
	Trace* trace;
} MeasurementInstance;

typedef void(MathFunc)(const SampleBuffer* first, SampleBuffer* result);

typedef struct MathTrace
{
	MathFunc* function;
	char* name;

	Units horizontal;
	Units vertical;
} MathTrace;

typedef struct MathTraceInstance
{
	MathTrace* mathTrace;
	Trace* firstTrace;
} MathTraceInstance;

typedef struct Screen
{
	cairo_pattern_t* background;
	float dt;	// seconds/pixel in the horizontal axis
	Grid grid;
	GQueue* traces;	// contains Trace*
	HANDLE hTracesMutex;	// syncs access to traces list
	short fps;		// frames/sec
	int width;		// pixels
	int height;		// pixels
	int maxVoltage;	// maximum positive voltage which can be displayed with gain=1, offset=0

	Trace* selectedTrace;
	int selectedTraceId;
	bool showTrigger;
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
	TRIGGER_MODE_NONE = 0,
	TRIGGER_MODE_SINGLE = 1,
	TRIGGER_MODE_AUTO = 2
} TriggerMode;

typedef enum TriggerSource
{
	TRIGGER_SOURCE_CH1 = 0,
	TRIGGER_SOURCE_CH2 = 1
} TriggerSource;

typedef enum TriggerType
{
	TRIGGER_TYPE_RAISING = 0,
	TRIGGER_TYPE_FALLING = 1,
	TRIGGER_TYPE_BOTH = 2
} TriggerType;

typedef struct Trigger
{
	TriggerMode mode;
	TriggerType type;
	TriggerSource source;
	float level;
} Trigger;

typedef enum ScopeEvent
{
	// received a data frame
	SCOPE_EVENT_DATA,
	// received a trigger frame
	SCOPE_EVENT_TRIGGER,
	// run button was pressed
	SCOPE_EVENT_RUN,
	// pause button was pressed
	SCOPE_EVENT_PAUSE,
	// end of buffer was reached
	SCOPE_EVENT_EOF,
	// trigger mode was changed by user
	SCOPE_EVENT_TRIGGER_MODE_CHANGE
} ScopeEvent;

typedef enum ScopeState
{
	// ignore incoming data and triggers, dont send config updates
	SCOPE_STATE_PAUSED = 0,
	// ignore incoming data
	SCOPE_STATE_WAIT = 1,	
	// all as usual
	SCOPE_STATE_CAPTURE = 2
} ScopeState;

typedef enum DisplayMode
{
	DISPLAY_MODE_WAVEFORM = 0,
	DISPLAY_MODE_XY = 1
} DisplayMode;

typedef struct Scope
{
	Screen screen;
	GQueue* channels;		// contains AnalogChannel*
	Trigger trigger;
	GQueue* measurements;	// contains MeasurementInstance*
	HANDLE hMeasurementsMutex;	// syncs access to measurements list
	ScopeState state;	
	Cursors cursors;
	DisplayMode display_mode;
	MathTraceInstance mathTraceDefinition;
	int bufferSize;
	int posInBuffer;
	bool shuttingDown;	// signals all the threads to terminate gracefully
} Scope;

SampleBuffer* sample_buffer_create(int size);

void screen_init();

Scope* scope_get();

void screen_add_measurement(const char* name, const char* source, guint id);

void screen_clear_measurements();

bool scope_build_and_send_config();

float scope_trace_get_horizontal_scale(const Trace* trace);

MeasurementInstance* scope_measurement_get_nth(int n);

MeasurementInstance* scope_measurement_add(Measurement* measurement, Trace* source);

AnalogChannel* scope_channel_get_nth(int n);

Trace* scope_trace_get_nth(int n);

Trace* scope_trace_get_math();

void scope_trace_save_ref(const Trace* trace);

void scope_trace_delete_ref(int index);

// moves the trace to the next position
// returns true when eof was reached
bool scope_screen_next_pos();

void scope_cursor_set(Cursor* cursor, int position);

void handle_scope_event(ScopeEvent event, void* args);

void scope_math_change(MathTrace* math);

#endif
