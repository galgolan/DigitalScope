#ifndef _FORMATTING_H_
#define _FORMATTING_H_

typedef enum Units
{
	UNITS_VOLTAGE,
	UNITS_FREQUENCY,
	UNITS_TIME,
	UNITS_DECIBEL,
	UNITS_PERCENT
} Units;

// returns a string representing the given value in the specified units
// allocates a new string on each invocation, caller is responsible for freeing the string
char* formatNumber(float value, Units units);

#endif