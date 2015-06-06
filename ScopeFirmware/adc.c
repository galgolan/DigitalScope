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
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#include "driverlib/adc.h"
#include "driverlib/comp.h"
#include "common.h"
#include "adc.h"

void configComparator()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);
	GPIOPinConfigure(GPIO_PD0_C0O);

	GPIOPinTypeComparator(GPIO_PORTC_BASE, GPIO_PIN_7);		// neg input
	GPIOPinTypeComparator(GPIO_PORTC_BASE, GPIO_PIN_6);		// pos input
	GPIOPinTypeComparator(GPIO_PORTD_BASE, GPIO_PIN_0);		// out

	ComparatorConfigure(COMP_BASE, 0, COMP_TRIG_NONE | COMP_ASRCP_PIN0 | COMP_OUTPUT_NORMAL);
}

void configAdc()
{
	//configComparator();

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
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH3);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH4);
	ADCSequenceEnable(ADC0_BASE, 0);
}

static uint32_t value[2];

uint32_t* sampleAdc()
{
	//
	// Trigger the sample sequence.
	//
	ADCIntClear(ADC0_BASE, 0);
	ADCProcessorTrigger(ADC0_BASE, 0);

	//
	// Wait until the sample sequence has completed.
	//
	while(!ADCIntStatus(ADC0_BASE, 0, false))
	{
	}

	//
	// Read the value from the ADC.
	//
	ADCSequenceDataGet(ADC0_BASE, 0, value);

	return value;
}
