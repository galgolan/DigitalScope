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

MathTrace MathTrace_Fft_Amplitude = { .name = "FFT", .function = math_trace_fft_amplitude, .horizontal = UNITS_FREQUENCY, .vertical = UNITS_VRMS };
MathTrace MathTrace_Fft_Amplitude_Db = { .name = "FFT dB", .function = math_trace_fft_amplitude_db, .horizontal = UNITS_FREQUENCY, .vertical = UNITS_DECIBEL };

static kiss_fftr_cfg kiss_cfg;
static int nfft;

HANDLE hKissFftMutex = INVALID_HANDLE_VALUE;

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
	mathTrace->horizontalScale = math_get_frequency(1);

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

	hKissFftMutex = CreateMutexSimple();
	if (hKissFftMutex == INVALID_HANDLE_VALUE)
		return 1;
	
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
	int i;

	math_trace_fft_amplitude(first, result);

	// modify to db
	for (i = 0; i < nfft/2+1; ++i)
	{
		result->data[i] = 10 * log(result->data[i]);
	}
	for (; i < result->size; ++i)
	{
		result->data[i] = -1000;
	}
}

float math_get_frequency(int k)
{
	int bin = k % (nfft / 2 + 1);
	Scope* scope = scope_get();
	return (float)bin / scope->screen.dt / nfft;
}

void math_trace_fft_amplitude(const SampleBuffer* first, SampleBuffer* result)
{
	kiss_fft_cpx* fft = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);

	if (!WaitForMutex(hKissFftMutex, 200))
		return;

	kiss_fftr(kiss_cfg, first->data, fft);

	ReleaseMutex(hKissFftMutex);

	int i;
	// copy amplitude to result
	for (i = 0; i < nfft/2+1; ++i)
	{
		result->data[i] = sqrt(2) * sqrt(fft[i].i * fft[i].i + fft[i].r * fft[i].r) / nfft;	// results are Vrms
	}
	for (; i < result->size; ++i)
	{
		result->data[i] = 0;
	}

	free(fft);
}