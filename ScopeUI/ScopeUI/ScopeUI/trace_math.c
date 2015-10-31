#include <Windows.h>
#include <math.h>

#include "scope.h"
#include "trace_math.h"
#include "config.h"

void math_trace_difference(const SampleBuffer* first, const SampleBuffer* second, SampleBuffer* result);
void math_trace_dft_amplitude(const SampleBuffer* first, const SampleBuffer* second, SampleBuffer* result);

MathTrace MathTrace_Difference = { .name = "Difference", .function = math_trace_difference };
MathTrace MathTrace_Dft_Amplitude = { .name = "DFT", .function = math_trace_dft_amplitude };

void math_update_trace()
{
	Scope* scope = scope_get();
	Trace* mathTrace = scope_trace_get_math();
	MathTraceInstance* mathInstance = &scope->mathTraceDefinition;

	if ((mathTrace->visible == TRUE) && (mathInstance != NULL) && (scope->display_mode == DISPLAY_MODE_WAVEFORM))
	{
		// perform calculation to update the samples
		mathInstance->mathTrace->function(mathInstance->firstTrace->samples, mathInstance->secondTrace->samples, mathTrace->samples);
	}
}

DWORD WINAPI math_worker_thread(LPVOID param)
{
	int mathRefreshIntervalMs = config_get_int("display", "math_refresh");
	Scope* scope = scope_get();
	while (!scope->shuttingDown)
	{
		math_update_trace();
		Sleep(mathRefreshIntervalMs);
	}
	return 0;
}

// second can be null
void math_trace_dft_amplitude(const SampleBuffer* first, const SampleBuffer* second, SampleBuffer* result)
{
	Scope* scope = scope_get();
	int width = scope->screen.width;
	int N = MIN(width,scope->bufferSize);
	int k,n;
	const float* x = first->data;
	double PI2N = G_PI * 2 / N;

	for (k = 0; k<N; ++k)
	{
		float xk_r = 0, xk_im = 0;
		for (n = 0; n < N; ++n)
		{
			xk_r += x[n] * (float)cos(n * k * PI2N);
			xk_im -= x[n] * (float)sin(n * k * PI2N);
		}

		// Power at Kth frequency bin (should we calculate in dB ?)
		//result->data[k] = 10 * log ( xk_r * xk_r + xk_im * xk_im);
		result->data[k] = xk_r * xk_r + xk_im * xk_im;
	}
}

void math_trace_difference(const SampleBuffer* first, const SampleBuffer* second, SampleBuffer* result)
{
	int i;
	Scope* scope = scope_get();
	int width = scope->screen.width;
	for (i = 0; i < MIN(width,scope->bufferSize); ++i)
	{
		result->data[i] = first->data[i] - second->data[i];
	}
}