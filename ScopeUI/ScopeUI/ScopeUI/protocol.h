#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

/*
SERIAL COMMUNICATIONS PROTOCOL DESCRIPTION

DATA (SCOPE->UI)

SoftwareBuffer > HardwareBuffer
SoftwareBuffer > MaxFrame=8

:FRAME:FRAME:FRAME:

Where FRAME can be:
	TRIG - to signal the UI to move the scope positition to start of screen
	1234567812345678 - 64-bits of data: 32-bit float for ch1 & 32-bit float for ch2.

CONFIGURATION (UI->SCOPE)

int		4	preamble
int		1	trigger mode (none, single, auto) + trigger source (ch1,ch2,ext)
float	4	trigger level
int		1	ch1 gain
float	4	ch1 offset
int		1	ch2 gain
float	4	ch2 offset
float	4	sample rate
int		4	checksum

*/

#include <stdbool.h>

#include "..\..\..\common\common.h"

bool protocol_send_config(const ConfigMsg* msg);

void handle_receive_date(char* buffer, int size);

float* protocol_read_samples(int* numSamplesPerChannel, int triggerIndex);

ConfigMsg common_create_config(TriggerConfig trigger, float triggerLevel, byte ch1Gain, float ch1Offset, byte ch2Gain, float ch2Offset, float sampleRate);

#endif