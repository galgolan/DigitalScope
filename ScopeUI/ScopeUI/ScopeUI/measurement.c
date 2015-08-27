#include <math.h>
#include <string.h>

#include "common.h"
#include "scope.h"
#include "measurement.h"

// measurement prototypes
float measure_avg(SampleBuffer* samples);

// measurements
Measurement Measurement_Average = { .name = "Average", .measure = measure_avg };

// calculation funtions
float measure_avg(SampleBuffer* samples)
{
	float avg = 0;
	int i;
	
	for (i = 0; i < samples->size; ++i)
	{
		avg += samples->data[i] / samples->size;
	}

	return avg;
}