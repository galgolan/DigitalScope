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
#include "driverlib/comp.h"
#include "driverlib/uart.h"

// include these for working with ROM functions
//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"

// utilities
#include "utils/uartstdio.h"

#include "uart.h"
#include "adc.h"
#include "config.h"
#include "spi.h"
#include "calc.h"

#include "../common/common.h"

typedef enum ReceiveState
{
	RECEIVE_STATE_WAITING_PREAMBLE,	// waiting to sync on preamble
	RECEIVE_STATE_WAITING_CONFIG	// waiting to accumulate enough bytes for config msg
} ReceiveState;

static volatile uint8_t buffer[64];
static volatile uint8_t buffer_index = 0;
static volatile uint64_t preambleBuffer = 0x00;
static volatile ReceiveState state = RECEIVE_STATE_WAITING_PREAMBLE;

void serialFloatPrint(char* buff, float f)
{
	int8_t i,j;
	uint8_t* b = (uint8_t*) &f;

	j=0;
	for(i=3; i>=0; --i)
	{
		uint8_t b1 = (b[i] >> 4) & 0x0f;
		uint8_t b2 = (b[i] & 0x0f);

		char c1 = (b1 < 10) ? ('0' + b1) : 'A' + b1 - 10;
		char c2 = (b2 < 10) ? ('0' + b2) : 'A' + b2 - 10;

		buff[2*j] = c1;
		buff[2*j+1] = c2;

		++j;
	}
}

void outputTrigger()
{
	UARTprintf(":::::TRIG:TRIG:TRIG:TRIG:TRIG:TRIG:");
}

void outputData(float ch1, float ch2)
{
	char buff[18];
	serialFloatPrint(buff, ch1);
	serialFloatPrint(buff+8, ch2);
	buff[16] = ':';
	buff[17] = '\0';

	UARTprintf(buff);
}

void outputDebug(double vin1, double vin2)
{
	char buff[256];
	if(vin1>=0)
		sprintf(buff, "Vin1=+%.2f Vin2=%.2f\n", vin1, vin2);
	else
		sprintf(buff, "Vin1=%.2f Vin2=%.2f\n", vin1, vin2);
	UARTprintf(buff);
}

void configUART()
{
	// UART0: PA.0 - RX, PA.1 - TX

	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlDelay(1000);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// always use PinType after PinConfigure
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	UARTClockSourceSet(UART0_BASE, UART_CLOCK_SYSTEM) ; // Set System Clock for UART

	// enable receive interrupt
	UARTIntEnable(UART0_BASE, UART_INT_RX);
	ScopeConfig* config = getConfig();
	UARTStdioConfig(0, 230400, config->systClock);
	UARTFlowControlSet(UART0_BASE, UART_FLOWCONTROL_NONE) ;
	UARTFIFOEnable(UART0_BASE) ; // UART FIFO enable
	UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX7_8, UART_FIFO_RX2_8);	// receive 4 chars each interrupt
	IntEnable(INT_UART0);
}

bool isChecksumCorrect(ConfigMsg* msg)
{
	return (msg->checksum == 0);
}

void handleCommand()
{
	ConfigMsg* msg = (ConfigMsg*)buffer;
	if((msg->preamble == CONFIG_PREAMBLE) && isChecksumCorrect(msg))
	{
		// received a valid config msg
		updateConfig(msg);
		setGain();
		setOffset();
		setTriggerSource();
		setTriggerMode();
		setTriggerType();
		setTriggerLevel();
		setSampleRate();
	}
}

// handles a new character received
void handleNewByte(uint8_t byte)
{
	switch(state)
	{
	case RECEIVE_STATE_WAITING_PREAMBLE:
		preambleBuffer = preambleBuffer >> 8;
		preambleBuffer |= ((uint64_t)byte) << 56;
		if(preambleBuffer == CONFIG_PREAMBLE)
		{
			uint64_t* pPreamble = (uint64_t*)buffer;
			*pPreamble = preambleBuffer;
			buffer_index = 8;
			state = RECEIVE_STATE_WAITING_CONFIG;
		}
		break;

	case RECEIVE_STATE_WAITING_CONFIG:
		buffer[buffer_index] = byte;
		buffer_index++;
		if(buffer_index >= sizeof(ConfigMsg))
		{
			buffer_index = 0;
			handleCommand();
			state = RECEIVE_STATE_WAITING_PREAMBLE;
			preambleBuffer = 0x00;
		}
		break;

	default:
		return;
	}
}

void UartISR(void)
{
	// handle serial input
	UARTIntClear(UART0_BASE, UART_INT_RX);

	while(UARTCharsAvail(UART0_BASE))
	{
		// receive data
		int32_t c = UARTCharGetNonBlocking(UART0_BASE);
		if(c != -1)
		{
			handleNewByte(c);
		}
	}
}
