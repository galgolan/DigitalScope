/*
 * config.c
 *
 *  Created on: Aug 3, 2015
 *      Author: galgo
 */

#include <stdint.h>
#include <stdbool.h>

// board definitions
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#include "driverlib/interrupt.h"

#include "config.h"
#include "scope_common.h"
#include "../common/common.h"
#include "adc.h"
#include "spi.h"
#include "calc.h"

static ScopeConfig config;

ScopeConfig* getConfig()
{
	return &config;
}

void updateConfig(ConfigMsg* msg)
{
	ScopeConfig* config = getConfig();

	if(isValidGain(msg->ch1_gain))
		config->channels[0].gain = msg->ch1_gain;
	if(isValidGain(msg->ch2_gain))
		config->channels[1].gain = msg->ch2_gain;

	config->channels[0].offset = calcOffsetFromVin(1, msg->ch1_offset);
	config->channels[1].offset = calcOffsetFromVin(2, msg->ch2_offset);

	// trigger mode
	if(msg->trigger == TRIGGER_CFG_MODE_NONE)
		config->trigger.mode = TRIG_MODE_FREE_RUNNING;
	else if (msg->trigger & TRIGGER_CFG_MODE_SINGLE)
		config->trigger.mode = TRIG_MODE_SINGLE;
	else if (msg->trigger & TRIGGER_CFG_MODE_AUTO)
		config->trigger.mode = TRIG_MODE_AUTO;

	// trigger source
	if(msg->trigger & TRIGGER_CFG_SRC_CH1)
		config->trigger.source = TRIG_SRC_CH1;
	else if(msg->trigger & TRIGGER_CFG_SRC_CH2)
		config->trigger.source = TRIG_SRC_CH2;

	// trigger level
	config->trigger.level = msg->triggerLevel;

	// trigger type
	if(msg->trigger & TRIGGER_CFG_TYPE_RAISING)
		config->trigger.type = TRIG_RISING;
	else if(msg->trigger & TRIGGER_CFG_TYPE_FALLING)
		config->trigger.type = TRIG_FALLING;
	else if(msg->trigger & TRIGGER_CFG_TYPE_BOTH)
			config->trigger.type = TRIG_BOTH;

	if((msg->sample_rate >= 90) && (msg->sample_rate <= 1e6))
	{
		config->sampleRate = msg->sample_rate;
	}
}
