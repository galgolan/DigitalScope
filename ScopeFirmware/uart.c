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

// include these for working with ROM functions
//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"

#include "driverlib/sysctl.h"

// peripherals
#include "driverlib/uart.h"

// utilities
#include "utils/uartstdio.h"

#include "uart.h"
#include "adc.h"
#include "config.h"
#include "spi.h"


#include "../common/common.h"

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

void twoChannelPrint(char* buff, double ch1, double ch2)
{
	sprintf(buff, "%.2f,%.2f\n", ch1, ch2);
}

void oneChannelPrint(char* buff, double value)
{
	sprintf(buff, "%.2f\n", value);
}

void outputDebugMany(double* ch1, double* ch2, uint32_t n)
{
	ScopeConfig* config = getConfig();
	char buff[24];
	int i;
	for(i=0; i<n; ++i)
	{
		if(config->channels[0].active && config->channels[1].active)
			twoChannelPrint(buff, ch1[i], ch2[i]);
		else if(config->channels[0].active && !config->channels[1].active)
			oneChannelPrint(buff, ch1[i]);
		else if(!config->channels[0].active && config->channels[1].active)
			oneChannelPrint(buff, ch2[i]);
		else
			return;

		UARTprintf(buff);
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

	UARTClockSourceSet(UART0_BASE, UART_CLOCK_SYSTEM) ; // Set System Clock for UART

	// enable receive interrupt
	UARTIntEnable(UART0_BASE, UART_INT_RX);

	UARTStdioConfig(0, 230400, sysClock);
	UARTFlowControlSet(UART0_BASE, UART_FLOWCONTROL_NONE) ;
	UARTFIFOEnable(UART0_BASE) ; // UART FIFO enable
	UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX7_8, UART_FIFO_RX2_8);	// receive 4 chars each interrupt
	IntEnable(INT_UART0);
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

static volatile uint8_t buffer[64];
static volatile uint8_t buffer_index = 0;

int8_t translateGain(byte gainValue)
{
	//1;2;4;5;8;10;16;32
	switch(gainValue)
	{
	case 1:
		return PGA_GAIN_1;
	case 2:
		return PGA_GAIN_2;
	case 4:
		return PGA_GAIN_4;
	case 5:
		return PGA_GAIN_5;
	case 8:
		return PGA_GAIN_8;
	case 10:
		return PGA_GAIN_10;
	case 16:
		return PGA_GAIN_16;
	case 32:
		return PGA_GAIN_32;
	default:
		return -1;		// invalid gain which should be ignored
	}
}

uint32_t translateCompRef(float refValue)
{
	if (refValue<=0.06875) return COMP_REF_0V;
	else if (refValue<=0.20625) return COMP_REF_0_1375V;
	else if (refValue<=0.34375) return COMP_REF_0_275V;
	else if (refValue<=0.48125) return COMP_REF_0_4125V;
	else if (refValue<=0.61875) return COMP_REF_0_55V;
	else if (refValue<=0.75625) return COMP_REF_0_6875V;
	else if (refValue<=0.8765625) return COMP_REF_0_825V;
	else if (refValue<=0.9453125) return COMP_REF_0_928125V;
	else if (refValue<=0.996875) return COMP_REF_0_9625V;
	else if (refValue<=1.0828125) return COMP_REF_1_03125V;
	else if (refValue<=1.1171875) return COMP_REF_1_1V;
	else if (refValue<=1.16875) return COMP_REF_1_134375V;
	else if (refValue<=1.2890625) return COMP_REF_1_2375V;
	else if (refValue<=1.3578125) return COMP_REF_1_340625V;
	else if (refValue<=1.409375) return COMP_REF_1_375V;
	else if (refValue<=1.478125) return COMP_REF_1_44375V;
	else if (refValue<=1.5296875) return COMP_REF_1_5125V;
	else if (refValue<=1.5984375) return COMP_REF_1_546875V;
	else if (refValue<=1.7015625) return COMP_REF_1_65V;
	else if (refValue<=1.7703125) return COMP_REF_1_753125V;
	else if (refValue<=1.821875) return COMP_REF_1_7875V;
	else if (refValue<=1.890625) return COMP_REF_1_85625V;
	else if (refValue<=1.9421875) return COMP_REF_1_925V;
	else if (refValue<=2.0109375) return COMP_REF_1_959375V;
	else if (refValue<=2.1140625) return COMP_REF_2_0625V;
	else if (refValue<=2.2171875) return COMP_REF_2_165625V;
	else if (refValue<=2.3203125) return COMP_REF_2_26875V;
	else return COMP_REF_2_371875V;
}

void updateConfig(ConfigMsg* msg)
{
	ScopeConfig* config = getConfig();

	int8_t gain1 = translateGain(msg->ch1_gain);
	int8_t gain2 = translateGain(msg->ch2_gain);
	if(gain1 != -1)
		config->channels[0].gain = gain1;
	if(gain2 != -1)
		config->channels[1].gain = gain2;
	// TODO: add offsets for each channel
	// TODO: add sample rate
	if(msg->trigger & TRIGGER_CFG_MODE_NONE)
	{
		config->trigger.mode = TRIG_MODE_FREE_RUNNING;
	}
	else
	{
		// a trigger is set
		if (msg->trigger & TRIGGER_CFG_MODE_SINGLE)
			config->trigger.mode = TRIG_MODE_SINGLE;
		else if (msg->trigger & TRIGGER_CFG_MODE_AUTO)
			config->trigger.mode = TRIG_MODE_AUTO;

		config->trigger.level = translateCompRef(msg->triggerLevel);

		if(msg->trigger & TRIGGER_CFG_SRC_CH1)
			config->trigger.source = TRIG_SRC_CH1;
		else if(msg->trigger & TRIGGER_CFG_SRC_CH2)
			config->trigger.source = TRIG_SRC_CH2;
	}
}

void handleCommand()
{
	ConfigMsg* msg = (ConfigMsg*)buffer;
	if(msg->preamble == CONFIG_PREAMBLE)
	{
		// received a valid config msg
		updateConfig(msg);
		setGain();
	}
}

void UartISR(void)
{
	// handle serial input
	//UARTIntDisable(UART0_BASE, UART_INT_RX);
	UARTIntClear(UART0_BASE, UART_INT_RX);

	while(UARTCharsAvail(UART0_BASE))
	{
		// receive data
		int32_t c = UARTCharGetNonBlocking(UART0_BASE);
		if(c != -1)
		{
			buffer[buffer_index] = (uint8_t)c;
			buffer_index++;

			if(buffer_index >= sizeof(ConfigMsg))
			{
				buffer_index = 0;
				handleCommand();
			}
		}
	}

	//buffer_index = 0;

	//UARTIntEnable(UART0_BASE, UART_INT_RX);
}
