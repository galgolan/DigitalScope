/*
 * probecomp.c
 *
 *  Created on: Oct 26, 2015
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

#include "driverlib/timer.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"

#include "probecomp.h"
#include "scope_common.h"

static volatile uint8_t value = HIGH;

#define PROBE_COMP_HERTZ	1000

void configProbeCompensation(uint32_t ui32SysClock)
{
	// set a digital pin to output
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
	GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, GPIO_PIN_5);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	// configure 1KHz timer periodic
	unsigned long timer_value = ui32SysClock / PROBE_COMP_HERTZ / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, timer_value);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntEnable(INT_TIMER0A);
	TimerEnable(TIMER0_BASE, TIMER_A);
}

void probeCompISR(void)
{
	// clear status
	uint32_t status = TimerIntStatus(TIMER0_BASE, false);
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	value = !value;
	GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, value);
	GPIOPinWrite(GPIO_PORTM_BASE, GPIO_PIN_5, value << 5);
}
