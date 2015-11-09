#include <Windows.h>
#include <math.h>

#include "scope.h"
#include "trace_math.h"
#include "config.h"
#include "threads.h"

#include "../../../kiss_fft130/kiss_fft.h"
#include "../../../kiss_fft130/tools/kiss_fftr.h"

void math_trace_fft_amplitude(const SampleBuffer* first, SampleBuffer* result);
void math_trace_fft_amplitude_db(const SampleBuffer* first, SampleBuffer* result);

MathTrace MathTrace_Fft_Amplitude = { .name = "FFT", .function = math_trace_fft_amplitude, .horizontal = UNITS_FREQUENCY, .vertical = UNITS_VOLTAGE };	// TODO: change to normalized units (%)
MathTrace MathTrace_Fft_Amplitude_Db = { .name = "FFT dB", .function = math_trace_fft_amplitude_db, .horizontal = UNITS_FREQUENCY, .vertical = UNITS_DECIBEL };

static kiss_fftr_cfg kiss_cfg;
static int nfft;

GQueue* math_get_all()
{
	static gboolean ready = FALSE;
	static GQueue* allFunctions = NULL;

	if (ready == FALSE)
	{
		allFunctions = g_queue_new();

		// build list
		g_queue_push_tail(allFunctions, &MathTrace_Fft_Amplitude);
		g_queue_push_tail(allFunctions, &MathTrace_Fft_Amplitude_Db);

		ready = TRUE;
	}

	return allFunctions;
}

void math_update_trace()
{
	Scope* scope = scope_get();
	Trace* mathTrace = scope_trace_get_math();
	
	MathTraceInstance* mathInstance = &scope->mathTraceDefinition;

	if ((mathTrace->visible == TRUE) && (mathInstance != NULL) && (scope->display_mode == DISPLAY_MODE_WAVEFORM))
	{
		// perform calculation to update the samples
		mathInstance->mathTrace->function(mathInstance->firstTrace->samples, mathTrace->samples);
	}
}

DWORD WINAPI math_worker_thread(LPVOID param)
{
	int mathRefreshIntervalMs = config_get_int("display", "math_refresh");
	Scope* scope = scope_get();

	nfft = scope->bufferSize % 2 == 0 ? scope->bufferSize : scope->bufferSize - 1;
	kiss_cfg = kiss_fftr_alloc(nfft, false, NULL, NULL);
	
	while (!scope->shuttingDown)
	{
		math_update_trace();
		Sleep(mathRefreshIntervalMs);
	}

	free(kiss_cfg);

	return 0;
}

void math_trace_fft_amplitude_db(const SampleBuffer* first, SampleBuffer* result)
{
	math_trace_fft_amplitude(first, result);

	// modify to db
	for (int i = 0; i < nfft; ++i)
	{
		result->data[i] = 10 * log(result->data[i]);
	}
}

void math_trace_fft_amplitude(const SampleBuffer* first, SampleBuffer* result)
{
	kiss_fft_cpx* fft = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
	kiss_fftr(kiss_cfg, first->data, fft);

	// copy amplitude to result
	for (int i = 0; i < nfft; ++i)
	{
		result->data[i] = sqrt(fft[i].i * fft[i].i + fft[i].r * fft[i].r);
	}

	free(fft);
}