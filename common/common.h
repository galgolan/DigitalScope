#ifndef _COMMON_H_
#define _COMMON_H_

#define CONFIG_PREAMBLE	0xABCDABCDABCDABCD

typedef unsigned char byte;
typedef unsigned long long uint64;

typedef struct ConfigMsg
{
	uint64 preamble;
	byte trigger;		// bitfield: TriggerConfig
	byte ch1_gain;
	float ch1_offset;
	byte ch2_gain;
	float ch2_offset;
	float	sample_rate;
	uint64	checksum;
} ConfigMsg;

typedef enum TriggerConfig
{
	TRIGGER_CFG_SRC_CH1		=	0x00,
	TRIGGER_CFG_SRC_CH2 = 0x01,
	TRIGGER_CFG_MODE_NONE = 0x02,
	TRIGGER_CFG_MODE_SINGLE = 0x04,
	TRIGGER_CFG_MODE_AUTO = 0x08
} TriggerConfig;

#endif