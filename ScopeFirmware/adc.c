/*
 * adc.c
 *
 *  Created on: Jun 6, 2015
 *      Author: t-galgo
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

#include "driverlib/adc.h"
#include "driverlib/comp.h"
#include "driverlib/interrupt.h"

#include "stdbool.h"
#include "common.h"
#include "adc.h"
#include "uart.h"
#include "calc.h"
#include "config.h"

//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"

double samples_ch1[BUFFER_SIZE];
double samples_ch2[BUFFER_SIZE];
volatile uint32_t index = 0;
volatile bool ready = false;
uint32_t samples[2];

uint32_t getTriggerType(TriggerType type)
{
	switch(type)
	{
	case TRIG_RISING:
		return COMP_TRIG_RISE;
	case TRIG_FALLING:
		return COMP_TRIG_FALL;
	case TRIG_BOTH:
		return COMP_TRIG_BOTH;
	default:
		return COMP_TRIG_NONE;
	}
}

void configComparator(Trigger trigger)
{
	uint32_t type = getTriggerType(trigger.type);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);
	GPIOPinConfigure(GPIO_PL2_C0O);

	GPIOPinTypeComparator(GPIO_PORTC_BASE, GPIO_PIN_7);		// neg input (PC7)
	GPIOPinTypeComparator(GPIO_PORTC_BASE, GPIO_PIN_6);		// pos input / internal reference
	GPIOPinTypeComparator(GPIO_PORTL_BASE, GPIO_PIN_2);		// out

	ComparatorRefSet(COMP_BASE, trigger.level);
	ComparatorConfigure(COMP_BASE, 0, type | COMP_ASRCP_REF | COMP_OUTPUT_INVERT);

	SysCtlDelay(1000);
}

void configAdc()
{
	ScopeConfig* config = getConfig();
	Trigger trigger = config->trigger;

	ready = false;
	index = 0;

	if(trigger.mode != TRIG_MODE_FREE_RUNNING)
		configComparator(trigger);

	// ch1 - A3 (PE0)
	// ch2 - A4 (PD7)

	// enable ADC0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	// enable required ports
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

	SysCtlDelay(1000);

	ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_EIGHTH, 24);

	// configure adc pins
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_7);
	SysCtlDelay(1000);
	// configure adc
	ADCReferenceSet(ADC0_BASE, ADC_REF_INT);
	SysCtlDelay(1000);

	// configure sequencer with 1 step: sample from CH3
	ADCSequenceDisable(ADC0_BASE, 0);

	if(trigger.mode == TRIG_MODE_FREE_RUNNING)
		ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
	else
		ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_COMP0, 0);

	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH3);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH4);
	ADCSequenceEnable(ADC0_BASE, 0);

	// enable ADC0 sequencer 0 interrupt
	ADCIntClear(ADC0_BASE, 0);
	ADCIntEnable(ADC0_BASE, 0);
	if(trigger.mode != TRIG_MODE_FREE_RUNNING)
		IntEnable(INT_ADC0SS0);
	IntMasterEnable();
}

void triggerAdc()
{
	// Trigger the sample sequence manualy
	ADCIntClear(ADC0_BASE, 0);
	ADCProcessorTrigger(ADC0_BASE, 0);
}

void sampleAdc(uint32_t* samples)
{
	triggerAdc();

	// Wait until the sample sequence has completed.
	while(!ADCIntStatus(ADC0_BASE, 0, false))
	{
	}

	// Read the value from the ADC.
	ADCSequenceDataGet(ADC0_BASE, 0, samples);
}

void AdcISR(void)
{
	ADCComparatorIntDisable(ADC0_BASE, 0);

	// Read the value from the ADC.
	ADCSequenceDataGet(ADC0_BASE, 0, samples);

	ADCComparatorIntClear(ADC0_BASE, 1);
	IntPendClear(INT_ADC0SS0);
	ADCIntClear(ADC0_BASE, 0);

	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);

	// TODO: remove floating point calculations from here avoid corruption of the stack
	// we can configure the FPU to save the stack but this will increase ISR latency
	samples_ch1[index] = calcCh1Input(samples[0]);
	samples_ch2[index] = calcCh2Input(samples[1]);
	++index;

	if(index >= BUFFER_SIZE)
	{
		// aquisition completed
		ready = true;
		index = 0;
	}
	else
	{
		triggerAdc();
	}

	//ADCComparatorIntEnable(ADC0_BASE, 0);
}
