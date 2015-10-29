#include <stdbool.h>

#include "..\..\..\common\common.h"
#include "serial.h"
#include "protocol.h"
#include "config.h"

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