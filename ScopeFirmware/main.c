#include <stdint.h>
#include <stdbool.h>

// board definitions
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"

// include these for working with ROM functions
//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"

#include "driverlib/sysctl.h"
//#include "driverlib/interrupt.h"

// peripherals
#include "driverlib/uart.h"
#include "driverlib/comp.h"
#include "driverlib/adc.h"

// utilities
#include "utils/uartstdio.h"

#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{

}
#endif

//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

void configUART()
{
	// UART0: PA.0 - RX, PA.1 - TX

	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// always use PinType after PinConfigure
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	UARTStdioConfig(0, 115200, g_ui32SysClock);
}

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
	// ch1 - A3 (PE0)
	// ch2 - A4 (PD7)

	// enable required ports
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

	// enable ADC0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	// configure adc pins
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_7);

	// configure adc
	ADCReferenceSet(ADC0_BASE, ADC_REF_INT);

	// configure sequencer with 1 step: sample from CH3
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH3);
	ADCSequenceEnable(ADC0_BASE, 0);
}

uint32_t sampleAdc()
{
	uint32_t value;

	//
	// Trigger the sample sequence.
	//
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
	ADCSequenceDataGet(ADC0_BASE, 0, &value);

	return value;

}

// config SPI2
void configSpi()
{
	// pga1 cs - pin 11
	// pga2 cs - pin 13
	// dac cs - pin 12
}

/*
 * main.c
 */
int main(void)
{
	//
	// Run from the PLL at 120 MHz.
	//
	g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
				SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
				SYSCTL_CFG_VCO_480), 120000000);

	configUART();
	configComparator();
	configAdc();
	
	while(1)
	{
		SysCtlDelay(g_ui32SysClock / 100);
		bool comp = ComparatorValueGet(COMP_BASE, 0);
		UARTprintf("COMP0=%d\n", comp);

		uint32_t value = sampleAdc();
		UARTprintf("A3=%d\n", value);
	}

	return 0;
}
