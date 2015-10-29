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

#include "common.h"
#include "probecomp.h"

#define PROBE_COMP_PORT	GPIO_PORTM_BASE
#define PROBE_COMP_PIN	GPIO_PIN_5

#define PROBE_COMP_FREQ	1

volatile uint8_t value = HIGH;

uint32_t getTimerValue(uint32_t hertz)
{
	return 120000000 / hertz;
}

void configProbeCompensation()
{
	// set a digital pin to output
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
	GPIOPinTypeGPIOOutput(PROBE_COMP_PORT, PROBE_COMP_PIN);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	SysCtlDelay(1000);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	// configure 1KHz timer periodic
	//TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);
	//uint32_t timer_value = getTimerValue(PROBE_COMP_FREQ);
	uint32_t timer_value = 1200000;
	TimerLoadSet(TIMER0_BASE, TIMER_A, timer_value);

	// enable Timer0A interrupt
	IntEnable(INT_TIMER0A);  // Enable Timer 1A Interrupt
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);	// enable timer A timeout
	TimerEnable(TIMER0_BASE, TIMER_A);
	TimerLoadSet(TIMER0_BASE, TIMER_A, timer_value);
	// write
	//GPIOPinWrite(PROBE_COMP_PORT, PROBE_COMP_PIN, 1);
}

void probeCompISR(void)
{
	// clear status
	//uint32_t status = TimerIntStatus(TIMER0_BASE, FALSE);
	//TimerIntClear(TIMER0_BASE, TIMER_A);

	value = !value;
	GPIOPinWrite(PROBE_COMP_PORT, PROBE_COMP_PIN, value);

	// restore status
	//TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}
