#ifndef _TRACE_MATH_H_
#define _TRACE_MATH_H

#include <Windows.h>

extern MathTrace MathTrace_Difference;
extern MathTrace MathTrace_Dft_Amplitude;

DWORD WINAPI math_worker_thread(LPVOID param);

#endif