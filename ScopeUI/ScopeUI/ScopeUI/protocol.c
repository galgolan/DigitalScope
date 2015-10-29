#include <stdbool.h>
#include <string.h>
#include <glib-2.0\glib.h>
#include <stdlib.h>

#include "..\..\..\common\common.h"
#include "serial.h"
#include "protocol.h"
#include "config.h"

typedef enum State
{
	STATE_WAITING_SOF,
	STATE_WAITING_EOF
} State;

int calc_padding_size(int msgSize)
{
	int fifo = config_get_int("serial", "fifo");
	int totalSize = fifo * (msgSize / fifo + (msgSize % fifo == 0 ? 0 : 1));
	int padding = totalSize - msgSize;
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

void parse_frame(char* frame, int size)
{
	bool trig = FALSE;

	if (strncmp(frame, TRIGGER_FRAME, size) == 0)
	{
		// its a trigger frame
		trig = TRUE;
	}
	else
	{
		// its a data frame
		char ch1Encoded[9];
		char ch2Encoded[9];

		strncpy(ch1Encoded, frame, 8);
		strncpy(ch2Encoded, frame + 8, 8);

		ch1Encoded[8] = '\0';
		ch2Encoded[8] = '\0';

		char* end;
		float ch1 = strtof(ch1Encoded, &end);
		float ch2 = strtof(ch2Encoded, &end);
	}
}

void handle_receive_date(char* buffer, int size)
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
			handle_receive_date(sof+1, size - garbageBytes-1);
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
			strncpy(frameBuffer + pos, buffer, size);
			pos += size;
			if (pos >= MAX_FRAME_SIZE)
			{
				// this is bad
			}
		}
		else
		{
			// found EOF, accumulate until EOF
			int bytesToCopy = eof - buffer;
			strncpy(frameBuffer + pos, buffer, bytesToCopy);
			pos += bytesToCopy;
			if (pos >= MAX_FRAME_SIZE)
			{
				// this is bad
			}
			parse_frame(frameBuffer, pos);
			pos = 0;
			state = STATE_WAITING_SOF;
			handle_receive_date(buffer + bytesToCopy, size - bytesToCopy);
		}
	}
	break;	
	}
}

float* protocol_read_samples(int* numSamplesPerChannel, int triggerIndex)
{

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