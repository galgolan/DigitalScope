#include <math.h>

#include "common.h"
#include "scope.h"
#include "trace_math.h"

void math_trace_difference(const SampleBuffer* first, const SampleBuffer* second, SampleBuffer* result);
void math_trace_dft_amplitude(const SampleBuffer* first, const SampleBuffer* second, SampleBuffer* result);

MathTrace MathTrace_Difference = { .name = "Difference", .function = math_trace_difference };
MathTrace MathTrace_Dft_Amplitude = { .name = "DFT", .function = math_trace_dft_amplitude };

void math_update_trace()
{
	Scope* scope = scope_get();
	Trace* mathTrace = &scope->screen.traces[2];
	MathTraceInstance* mathInstance = &scope->mathTraceDefinition;

	if ((mathTrace->visible == TRUE) && (mathInstance != NULL) && (scope->state == SCOPE_STATE_RUNNING))
	{
		// perform calculation to update the samples
		mathInstance->mathTrace->function(mathInstance->firstTrace->samples, mathInstance->secondTrace->samples, mathTrace->samples);
	}
}

// second can be null
void math_trace_dft_amplitude(const SampleBuffer* first, const SampleBuffer* second, SampleBuffer* result)
{
	int N = BUFFER_SIZE;
	int k,n;
	const float* x = first->data;
	float PI2 = G_PI * 2;

	for (k = 0; k<N; ++k)
	{
		float xk_r = 0, xk_im = 0;
		for (n = 0; n < N; ++n)
		{
			xk_r += x[n] * (float)cos(n * k * PI2 / N);
			xk_im -= x[n] * (float)sin(n * k * PI2 / N);
		}

		// Power at Kth frequency bin (should we calculate in dB ?)
		result->data[k] = xk_r * xk_r + xk_im * xk_im;
	}
}

void math_trace_difference(const SampleBuffer* first, const SampleBuffer* second, SampleBuffer* result)
{
	int i;

	for (i = 0; i < BUFFER_SIZE; ++i)
	{
		result->data[i] = first->data[i] - second->data[i];
	}
}