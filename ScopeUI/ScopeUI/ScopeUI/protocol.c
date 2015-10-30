#define _CRT_SECURE_NO_WARNINGS

#include <stdbool.h>
#include <string.h>
#include <glib-2.0\glib.h>
#include <stdlib.h>
#include <stdio.h>

#include "..\..\..\common\common.h"
#include "serial.h"
#include "protocol.h"
#include "config.h"

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

int calc_padding_size(int msgSize)
{
	int fifo = config_get_int("serial", "fifo");
	int totalSize = fifo * (msgSize / fifo + (msgSize % fifo == 0 ? 0 : 1));
	int padding = totalSize - msgSize;
	return padding;
}

bool protocol_send_config(const ConfigMsg* msg)
{
	// send config
	if (!serial_write((const char*)msg, sizeof(*msg)))
		return FALSE;

	int paddingSize = calc_padding_size(sizeof(*msg));
	// send padding
	char* padding = (char*)malloc(paddingSize);
	if (!serial_write(padding, paddingSize))
		return FALSE;

	return TRUE;
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
			_snscanf(frame, 16, "%08X%08X", &result.samples[0], &result.samples[1]);
		}
	}

	return result;
}

void copyBytes(char* dst, char* src, int count, int* pos)
{
	memcpy(dst + *pos, src, count);
	*pos = *pos + count;
}

void handle_receive_date(char* buffer, int size, float* samples0, float* samples1, pPosIncreaseFunc posIncFunc, pPosGetFunc posGetFunc)
{
	static State state = STATE_WAITING_SOF;
	
	static char frameBuffer[MAX_FRAME_SIZE];	// holds frames while they are being re-constructed
	static int pos = 0;							// position in frameBuffer

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
			handle_receive_date(sof+1, size - garbageBytes-1, samples0, samples1, posIncFunc, posGetFunc);
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
			copyBytes(frameBuffer, buffer, size, &pos);
			if (pos >= MAX_FRAME_SIZE)
			{
				// this is bad
				pos = 0;
				++receiveStats.bad;
			}
		}
		else
		{
			// found EOF, accumulate until EOF
			int bytesToCopy = eof - buffer;
			copyBytes(frameBuffer, buffer, bytesToCopy, &pos);
			if (pos >= MAX_FRAME_SIZE)
			{
				// this is bad
				pos = 0;
				++receiveStats.bad;
			}
			ParseResult result = parse_frame(frameBuffer, pos);
			
			if (result.frameType == FRAME_TYPE_TRIGGER)
			{
				// TODO: implement
				++receiveStats.triggers;
			}
			else if(result.frameType == FRAME_TYPE_DATA)
			{
				int posInSamples = posGetFunc();
				samples0[posInSamples] = result.samples[0];
				samples1[posInSamples] = result.samples[1];
				posIncFunc();
			}
			else // FRAME_TYPE_MALFORMED
			{
				// bad frame, ignore
				++receiveStats.malformed;
			}

			pos = 0;
			state = STATE_WAITING_SOF;
			handle_receive_date(buffer + bytesToCopy, size - bytesToCopy, samples0, samples1, posIncFunc, posGetFunc);
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

ConfigMsg common_create_config(TriggerConfig trigger, float triggerLevel, byte ch1Gain, float ch1Offset, byte ch2Gain, float ch2Offset, float sampleRate)
{
	ConfigMsg msg;

	msg.preamble = CONFIG_PREAMBLE;

	msg.ch1_gain = ch1Gain;
	msg.ch1_offset = ch1Offset;
	msg.ch2_gain = ch2Gain;
	msg.ch2_offset = ch2Offset;
	msg.sample_rate = sampleRate;
	msg.trigger = (byte)trigger;

	msg.checksum = calc_checksum(&msg);

	return msg;
}