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

#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

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
	configComparator();

	// ch1 - A3 (PE0)
	// ch2 - A4 (PD7)

	// enable ADC0
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	// enable required ports
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

	SysCtlDelay(1000);

	ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_EIGHTH, 24);

	// configure adc pins
	ROM_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);
	ROM_GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_7);
	SysCtlDelay(1000);
	// configure adc
	ROM_ADCReferenceSet(ADC0_BASE, ADC_REF_INT);
	SysCtlDelay(1000);

	// configure sequencer with 1 step: sample from CH3
	ROM_ADCSequenceDisable(ADC0_BASE, 0);
	ROM_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
	ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH3);
	ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH4);
	ROM_ADCSequenceEnable(ADC0_BASE, 0);
}



void sampleAdc(uint32_t* samples)
{
	//
	// Trigger the sample sequence.
	//
	ROM_ADCIntClear(ADC0_BASE, 0);
	ROM_ADCProcessorTrigger(ADC0_BASE, 0);

	//
	// Wait until the sample sequence has completed.
	//
	while(!ROM_ADCIntStatus(ADC0_BASE, 0, false))
	{
	}

	//
	// Read the value from the ADC.
	//
	ADCSequenceDataGet(ADC0_BASE, 0, samples);

	//return samples;
}
