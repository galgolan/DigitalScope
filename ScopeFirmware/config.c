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

#include "stdbool.h"
#include "common.h"

#include "config.h"

static ScopeConfig config;

ScopeConfig* getConfig()
{
	return &config;
}
