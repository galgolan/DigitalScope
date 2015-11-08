#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <glib-2.0\glib.h>
#include <stdlib.h>

#include "..\..\..\common\common.h"
#include "serial.h"
#include "protocol.h"
#include "config.h"
#include "scope.h"
#include "threads.h"

typedef enum State
{
	STATE_WAITING_SOF,
	STATE_WAITING_EOF
} State;

typedef enum FrameType
{
	FRAME_TYPE_TRIGGER,
	FRAME_TYPE_DATA,
	FRAME_TYPE_MALFORMED
} FrameType;

typedef struct ParseResult
{
	FrameType frameType;
	float samples[2];
} ParseResult;

static ReceiveStats receiveStats = { .samples = 0, .triggers = 0, .malformed = 0, .bad = 0 };

// the most updated config msg as written by the UI thread
static ConfigMsg configMsg;
static bool shouldWriteConfig = FALSE;
HANDLE hConfigMsgMutex = INVALID_HANDLE_VALUE;

union {
	long long ll;
	struct
	{
		float f1;
		float f2;
	} floats;
} dword_64_bit;

bool protocol_update_config(const ConfigMsg* msg)
{
	TRY_LOCK(FALSE, "cant aquire lock on config copy", hConfigMsgMutex, 2000)

	configMsg = *msg;	// create a local clone of the config msg
	shouldWriteConfig = TRUE;	// mark as updated

	return ReleaseMutex(hConfigMsgMutex);
}


int calc_padding_size(int msgSize)
{
	int fifo = config_get_int("serial", "fifo");
	int totalSize = fifo * (msgSize / fifo + (msgSize % fifo == 0 ? 0 : 1));
	int padding = totalSize - msgSize;
	return padding;
}

bool protocol_init()
{
	hConfigMsgMutex = CreateMutexSimple();
	return TRUE;
}

void protocol_cleanup()
{
	CloseHandleHelper(hConfigMsgMutex);
}

bool protocol_send_config(const ConfigMsg* msg)
{
	// send config
	if (!serial_write((const char*)msg, sizeof(ConfigMsg)))
		return FALSE;

	int paddingSize = calc_padding_size(sizeof(ConfigMsg));
	if (paddingSize > 0)
	{
		// send padding
		char* padding = (char*)malloc(paddingSize);
		if (!serial_write(padding, paddingSize))
			return FALSE;
	}

	return TRUE;
}

DWORD WINAPI protocol_config_updater_thread(LPVOID param)
{
	DWORD sleepInterval = config_get_int("hardware", "configUpdateInterval");
	Scope* scope = scope_get();
	ConfigMsg msg;

	while (!scope->shuttingDown)
	{
		bool updateConfig = FALSE;
		if (shouldWriteConfig)
		{
			TRY_LOCK(FALSE, "cant aquire lock on config copy", hConfigMsgMutex, 2000)

			if (shouldWriteConfig)	// TODO: compare bytes to see if actually changed
			{
				updateConfig = TRUE;
				msg = configMsg;	// create local copy so we wont have to hold the lock
			}

			ReleaseMutex(hConfigMsgMutex);

			if (updateConfig)
			{
				protocol_send_config(&msg);
				shouldWriteConfig = FALSE;
			}
		}

		Sleep(sleepInterval);
	}

	return 0;
}

ParseResult parse_frame(char* frame, int size)
{
	ParseResult result;
	if (size < 4)
	{
		result.frameType = FRAME_TYPE_MALFORMED;
		return result;
	}

	if (strncmp(frame, TRIGGER_FRAME, size) == 0)
	{
		// its a trigger frame
		result.frameType = FRAME_TYPE_TRIGGER;
	}
	else
	{
		if (size != 16)
		{
			// bad frame
			result.frameType = FRAME_TYPE_MALFORMED;
		}
		else
		{
			result.frameType = FRAME_TYPE_DATA;
			//frame[16] = '\0';
			//dword_64_bit.ll = strtoll(frame, NULL, 16);
			_snscanf(frame, 16, "%08X%08X", &result.samples[0], &result.samples[1]);	// TODO: change to more efficient function
			//result.samples[0] = dword_64_bit.floats.f2;
			//result.samples[1] = dword_64_bit.floats.f1;
		}
	}

	return result;
}

bool copyBytes(char* dst, char* src, int count, int* pos)
{
	if (count + *pos >= MAX_FRAME_SIZE-1)
	{
		return FALSE;
	}
	if (*pos < 0)
	{
		return FALSE;
	}
	if (count < 0)
	{
		return FALSE;
	}
	for (int i = 0; i < count; ++i)
	{
		dst[*pos + i] = src[i];
	}
	*pos = *pos + count;
	return TRUE;
}

void handle_receive_date(char* buffer, int size, float* samples0, float* samples1, pHandleFrame frameHandler)
{
	static State state = STATE_WAITING_SOF;
	
	static char frameBuffer[MAX_FRAME_SIZE];	// holds frames while they are being re-constructed
	static int pos = 0;							// position in frameBuffer

	if (size == 0)
		return;

	if (size < 0)
		return;

	switch (state)
	{
	// we are waiting for a frame to start
	case STATE_WAITING_SOF:
	{
		char* sof = strchr(buffer, FRAME_START);
		if (sof == NULL)
		{
			// buffer does not contain SOF, throw it all
		}
		else
		{
			// frame is starting, change state
			state = STATE_WAITING_EOF;
			int garbageBytes = sof - buffer;
			handle_receive_date(sof+1, size - garbageBytes-1, samples0, samples1, frameHandler);
		}
	}
	break;

	// we are accumulating frame data into frameBuffer
	case STATE_WAITING_EOF:
	{
		char* eof = strchr(buffer, FRAME_START);
		if (eof == NULL)
		{
			// buffer does not contain EOF, accumulate all
			bool copied = copyBytes(frameBuffer, buffer, size, &pos);
			if (pos >= MAX_FRAME_SIZE)
			{
				// this is bad
				pos = 0;
				frameBuffer[0] = '\0';
				++receiveStats.bad;
			}
		}
		else
		{
			// found EOF, accumulate until EOF
			int bytesToCopy = eof - buffer;
			bool copied = copyBytes(frameBuffer, buffer, bytesToCopy, &pos);
			if (pos >= MAX_FRAME_SIZE)
			{
				// this is bad
				pos = 0;
				frameBuffer[0] = '\0';
				++receiveStats.bad;
			}
			if (!copied)
			{
				receiveStats.bad++;
			}
			else
			{
				ParseResult result = parse_frame(frameBuffer, pos);

				if (result.frameType == FRAME_TYPE_TRIGGER)
				{
					++receiveStats.triggers;
					frameHandler(NULL, 0, TRUE);
				}
				else if (result.frameType == FRAME_TYPE_DATA)
				{
					frameHandler(result.samples, 2, FALSE);
				}
				else // FRAME_TYPE_MALFORMED
				{
					// bad frame, ignore
					++receiveStats.malformed;
				}

				pos = 0;
				frameBuffer[0] = '\0';
				state = STATE_WAITING_SOF;
				if (size - bytesToCopy > 0)
				{
					// some bytes are left
					handle_receive_date(buffer + bytesToCopy, size - bytesToCopy, samples0, samples1, frameHandler);
				}
			}
		}
	}
	break;	
	}
}

float* protocol_read_samples(int* numSamplesPerChannel, int triggerIndex)
{
	return NULL;
}

uint64 calc_checksum(const ConfigMsg* msg)
{
	return 0x00;
}

ConfigMsg common_create_config(TriggerConfig trigger, float triggerLevel, byte ch1Gain, float ch1Offset, byte ch2Gain, float ch2Offset, unsigned int sampleRate)
{
	ConfigMsg msg;

	msg.preamble = CONFIG_PREAMBLE;

	msg.ch1_gain = ch1Gain;
	msg.ch1_offset = ch1Offset;
	msg.ch2_gain = ch2Gain;
	msg.ch2_offset = ch2Offset;
	msg.sample_rate = sampleRate;
	msg.trigger = (byte)trigger;
	msg.triggerLevel = triggerLevel;

	msg.checksum = calc_checksum(&msg);

	return msg;
}