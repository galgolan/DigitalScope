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
	1234567812345678 - 64-bits of data: 32-bit float for ch1 & 32-bit float for ch2 (bytes are in ASCII HEX little-endian)

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

#include <Windows.h>
#include <stdbool.h>

#include "..\..\..\common\common.h"

typedef struct ReceiveStats
{
	unsigned long long samples;
	unsigned long long triggers;
	unsigned long long malformed;
	unsigned long long bad;
} ReceiveStats;

typedef void(*pPosIncreaseFunc)();
typedef int(*pPosGetFunc)();

typedef void(*pHandleFrame)(float* samples, int count, bool trigger);

bool protocol_update_config(const ConfigMsg* msg);

bool protocol_init();

void protocol_cleanup();

void handle_receive_date(char* buffer, int size, float* samples0, float* samples1, pHandleFrame frameHandler);

float* protocol_read_samples(int* numSamplesPerChannel, int triggerIndex);

ConfigMsg common_create_config(TriggerConfig trigger, float triggerLevel, byte ch1Gain, float ch1Offset, byte ch2Gain, float ch2Offset, unsigned int sampleRate);

DWORD WINAPI protocol_config_updater_thread(LPVOID param);

#endif