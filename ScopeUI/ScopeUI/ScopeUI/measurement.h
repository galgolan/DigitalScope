#ifndef _MEASUREMENT_H_
#define _MEASUREMENT_H_

extern Measurement Measurement_Average;
extern Measurement Measurement_Minimum;
extern Measurement Measurement_Maximum;
extern Measurement Measurement_PeakToPeak;
extern Measurement Measurement_RMS;

DWORD WINAPI measurement_worker_thread(LPVOID param);

#endif
