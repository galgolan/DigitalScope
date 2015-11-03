#include <string.h>
#include <math.h>

#include "formatting.h"

#define BUFFER_SIZE	32

void formatVolts(float value, char* string)
{
	int mantisa = abs((int)value);
	if (mantisa / 1000 > 0)
	{
		sprintf(string, "%.2fKV", value/1000);
	}
	else if (mantisa / 1 > 0)
	{
		sprintf(string, "%.2fV", value);
	}
	else if ((int)(fabsf(value) * 1000) > 0)
	{
		sprintf(string, "%.2fmV", value*1000);
	}
	else if ((int)(fabsf(value) * 1000000) > 0)
	{
		sprintf(string, "%.2fuV", value*1000000);
	}
	else
		sprintf(string, "%.2fV", value);
}

void formatTime(float value, char* string)
{
	int mantisa = abs((int)value);
	if (mantisa / 1 > 0)
	{
		sprintf(string, "%.2fs", value);
	}
	else if ((int)(fabsf(value) * 1000) > 0)
	{
		sprintf(string, "%.2fms", value * 1000);
	}
	else if ((int)(fabsf(value) * 1000000) > 0)
	{
		sprintf(string, "%.2fus", value * 1000000);
	}
	else if ((int)(fabsf(value) * 1000000000) > 0)
	{
		sprintf(string, "%.2fns", value * 1000000000);
	}
	else
		sprintf(string, "%fs", value);
}

void formatFrequency(float value, char* string)
{
	long long mantisa = llabs((long long)value);
	if (mantisa / 1000000000 > 0)
	{
		sprintf(string, "%.2fGHz", value / 1000000000);
	}
	else if (mantisa / 1000000 > 0)
	{
		sprintf(string, "%.2fMHz", value / 1000000);
	}
	else if (mantisa / 1000 > 0)
	{
		sprintf(string, "%.2fKHz", value / 1000);
	}
	else
		sprintf(string, "%fHz", value);
}

char* formatNumber(float value, Units units)
{
	char* string = (char*)malloc(BUFFER_SIZE * sizeof(char));

	switch (units)
	{
	case UNITS_PERCENT:
		sprintf(string, "%.1f%%", value);
		break;
	case UNITS_VOLTAGE:
		formatVolts(value, string);
		break;
	case UNITS_FREQUENCY:
		formatFrequency(value, string);
		break;
	case UNITS_TIME:
		formatTime(value, string);
		break;
	case UNITS_DECIBEL:
		sprintf(string, "%.2f", value);
		break;
	default:
		sprintf("%.3f", value);
	}

	return string;
}