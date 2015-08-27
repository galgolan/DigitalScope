/*
 * config.h
 *
 *  Created on: Aug 3, 2015
 *      Author: galgo
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "stdbool.h"

#define NUM_CHANNELS	2
#define BUFFER_SIZE		1024
#define VCC				3.3

typedef enum TriggerType
{
	TRIG_RISING,
	TRIG_FALLING,
	TRIG_BOTH,
	TRIG_NONE
} TriggerType;

typedef enum TriggerMode
{
	TRIG_MODE_SINGLE,
	TRIG_MODE_AUTO,
	TRIG_MODE_FREE_RUNNING
} TriggerMode;

typedef struct Trigger
{
	TriggerType type;
	uint32_t level;
	TriggerMode mode;
	// TODO: source. currently ch2
} Trigger;

typedef struct ChannelConfig
{
	bool active;
	int gain;		// a value from spi.h under: PGA constants
	// TODO: offset
} ChannelConfig;

typedef struct ScopeConfig
{
	Trigger trigger;
	ChannelConfig channels[NUM_CHANNELS];
	// TODO: sample rate
} ScopeConfig;

ScopeConfig* getConfig();

#endif /* CONFIG_H_ */
