#ifndef _TRACE_MATH_H_
#define _TRACE_MATH_H

#include <Windows.h>
#include <glib-2.0\glib.h>

extern MathTrace MathTrace_Fft_Amplitude;
extern MathTrace MathTrace_Fft_Amplitude_Db;

// a queue of MathTrace
GQueue* math_get_all();

DWORD WINAPI math_worker_thread(LPVOID param);

#endif