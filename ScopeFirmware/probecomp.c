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

volatile uint8_t value = HIGH;

void configProbeCompensation()
{
	// set a digital pin to output
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_3);
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	// configure 1KHz timer periodic
	unsigned long timer_value = (SysCtlClockGet() / 1) - 1;
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
	GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_3, value);
}
