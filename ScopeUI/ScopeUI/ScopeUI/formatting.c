#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "formatting.h"

#define BUFFER_SIZE	32

#define KILO	1000
#define MEGA	1000000
#define GIGA	1000000000

void formatVolts(float value, char* string)
{
	int mantisa = abs((int)value);
	if (mantisa / KILO > 0)
	{
		sprintf(string, "%.2fKV", value / KILO);
	}
	else if (mantisa / 1 > 0)
	{
		sprintf(string, "%.2fV", value);
	}
	else if ((int)(fabsf(value) * KILO) > 0)
	{
		sprintf(string, "%.2fmV", value * KILO);
	}
	else if ((int)(fabsf(value) * MEGA) > 0)
	{
		sprintf(string, "%.2fuV", value * MEGA);
	}
	else
		sprintf(string, "%.2fV", value);
}

void formatVoltsRms(float value, char* string)
{
	int mantisa = abs((int)value);
	if (mantisa / KILO > 0)
	{
		sprintf(string, "%.2fKVrms", value / KILO);
	}
	else if (mantisa / 1 > 0)
	{
		sprintf(string, "%.2fVrms", value);
	}
	else if ((int)(fabsf(value) * KILO) > 0)
	{
		sprintf(string, "%.2fmVrms", value * KILO);
	}
	else if ((int)(fabsf(value) * MEGA) > 0)
	{
		sprintf(string, "%.2fuVrms", value * MEGA);
	}
	else
		sprintf(string, "%.2fVrms", value);
}

void formatTime(float value, char* string)
{
	int mantisa = abs((int)value);
	if (mantisa / 1 > 0)
	{
		sprintf(string, "%.2fs", value);
	}
	else if ((int)(fabsf(value) * KILO) > 0)
	{
		sprintf(string, "%.2fms", value * KILO);
	}
	else if ((int)(fabsf(value) * MEGA) > 0)
	{
		sprintf(string, "%.2fus", value * MEGA);
	}
	else if ((int)(fabsf(value) * GIGA) > 0)
	{
		sprintf(string, "%.2fns", value * GIGA);
	}
	else
		sprintf(string, "%fs", value);
}

void formatFrequency(float value, char* string)
{
	long long mantisa = llabs((long long)value);
	if (mantisa / GIGA > 0)
	{
		sprintf(string, "%.2fGHz", value / GIGA);
	}
	else if (mantisa / MEGA > 0)
	{
		sprintf(string, "%.2fMHz", value / MEGA);
	}
	else if (mantisa / KILO > 0)
	{
		sprintf(string, "%.2fKHz", value / KILO);
	}
	else if (mantisa / 1 > 0)
	{
		sprintf(string, "%.2fHz", value);
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
	case UNITS_VRMS:
		formatVoltsRms(value, string);
		break;
	case UNITS_FREQUENCY:
		formatFrequency(value, string);
		break;
	case UNITS_TIME:
		formatTime(value, string);
		break;
	case UNITS_DECIBEL:
		sprintf(string, "%.2fdB", value);
		break;
	default:
		sprintf(string, "%.3f", value);
	}

	return string;
}