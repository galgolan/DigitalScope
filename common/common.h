#ifndef _COMMON_H_
#define _COMMON_H_

#define CONFIG_PREAMBLE	0xABCDABCDABCDABCD
#define FRAME_START		':'
#define TRIGGER_FRAME	"TRIG"
#define MAX_FRAME_SIZE	24

typedef unsigned char byte;
typedef unsigned long long uint64;

typedef struct ConfigMsg
{
	uint64 preamble;
	byte trigger;		// bitfield: TriggerConfig
	float triggerLevel;	// volts
	byte ch1_gain;
	float ch1_offset;	// volts
	byte ch2_gain;
	float ch2_offset;	// volts
	unsigned int sample_rate;	// sample rate [Hz]
	uint64	checksum;
} ConfigMsg;

typedef enum TriggerConfig
{
	TRIGGER_CFG_MODE_NONE		=	0x00,

	TRIGGER_CFG_SRC_CH1			=	0x01,
	TRIGGER_CFG_SRC_CH2			=	0x02,
	TRIGGER_CFG_MODE_SINGLE		=	0x04,
	TRIGGER_CFG_MODE_AUTO		=	0x08,
	TRIGGER_CFG_TYPE_RAISING	=	0x10,
	TRIGGER_CFG_TYPE_FALLING	=	0x20,
	TRIGGER_CFG_TYPE_BOTH		=	0x40,

	// used in SINGLE mode to tell the scope to arm the trigger now
	TRIGGER_CFG_ARM_NOW			=	0x80
} TriggerConfig;

#endif

