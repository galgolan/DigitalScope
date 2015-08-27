/*
 * uart.c
 *
 *  Created on: Jun 6, 2015
 *      Author: t-galgo
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// board definitions
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"

// include these for working with ROM functions
//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"

#include "driverlib/sysctl.h"
//#include "driverlib/interrupt.h"

// peripherals
#include "driverlib/uart.h"

// utilities
#include "utils/uartstdio.h"

#include "uart.h"
#include "adc.h"

void twoChannelPrint(char* buffer, double ch1, double ch2)
{
	sprintf(buffer, "%.2f,%.2f\n", ch1, ch2);
}

void oneChannelPrint(char* buffer, double value)
{
	sprintf(buffer, "%.2f\n", value);
}

void outputDebugMany(double* ch1, double* ch2, uint32_t n)
{
	ScopeConfig* config = getConfig();
	char buffer[24];
	int i;
	for(i=0; i<n; ++i)
	{
		if(config->channels[0].active && config->channels[1].active)
			twoChannelPrint(buffer, ch1[i], ch2[i]);
		else if(config->channels[0].active && !config->channels[1].active)
			oneChannelPrint(buffer, ch1[i]);
		else if(!config->channels[0].active && config->channels[1].active)
			oneChannelPrint(buffer, ch2[i]);
		else
			return;

		UARTprintf(buffer);
	}
}

void configUART(uint32_t sysClock)
{
	// UART0: PA.0 - RX, PA.1 - TX

	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlDelay(1000);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// always use PinType after PinConfigure
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	//UARTClockSourceSet(UART0_BASE, UART_CLOCK_SYSTEM) ; // Set System Clock for UART

	// enable receive interrupt
	UARTIntEnable(UART0_BASE, UART_INT_RX);

	UARTStdioConfig(0, 115200, sysClock);
	UARTFlowControlSet(UART0_BASE, UART_FLOWCONTROL_NONE) ;
	UARTFIFOEnable(UART0_BASE) ; // UART FIFO enable
	UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX7_8, UART_FIFO_RX2_8);	// receive 4 chars each interrupt
	IntEnable(INT_UART0);
}

void outputDebug(double vin1, double vin2)
{
	char buffer[256];
	if(vin1>=0)
		sprintf(buffer, "Vin1=+%.2f Vin2=%.2f\n", vin1, vin2);
	else
		sprintf(buffer, "Vin1=%.2f Vin2=%.2f\n", vin1, vin2);
	UARTprintf(buffer);
}

char buffer[512];
volatile int buffer_index = 0;

void handleCommand(char* command, int len)
{
	if(strcmp("trig", command)==0)
	{
		UARTprintf("#TRIG#\n");
		ADCComparatorIntEnable(ADC0_BASE, 0);
		configAdc();
	}
}

#define COMMAND_LEN	4

void UartISR(void)
{
	// handle serial input
	UARTIntDisable(UART0_BASE, UART_INT_RX);
	UARTIntClear(UART0_BASE, UART_INT_RX);

	while(UARTCharsAvail(UART0_BASE))
	{
		// receive data
		uint32_t c = UARTCharGetNonBlocking(UART0_BASE);
		buffer[buffer_index] = (char)c;
		buffer_index++;

		if(buffer_index >= COMMAND_LEN)
		{
			buffer_index = 0;
			handleCommand(buffer, COMMAND_LEN);
		}
	}

	buffer_index = 0;

	UARTIntEnable(UART0_BASE, UART_INT_RX);
}
