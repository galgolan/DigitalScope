#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
#include "driverlib/fpu.h"
//#include "driverlib/interrupt.h"

// utilities
#include "utils/uartstdio.h"

#include "spi.h"
#include "calc.h"
#include "adc.h"
#include "uart.h"

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

void setup()
{
	FPUEnable();

	//
	// Run from the PLL at 120 MHz.
	//
	g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
					SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
					SYSCTL_CFG_VCO_480), 120000000);

	configUART(g_ui32SysClock);
	configSPI();
	configAdc();

	setDacVoltage(3.3/2, 1);
	setDacVoltage(3.3/2, 2);

	setPga1Channel(PGA_CHANNEL_0);
	setPga1Channel(PGA_CHANNEL_0);
	setPga1Gain(PGA_GAIN_1);
	setPga2Gain(PGA_GAIN_1);
}

/*
 * main.c
 */
int main(void)
{
	setup();

	char buffer[256];
	while(1)
	{
		SysCtlDelay(g_ui32SysClock / 1000000);

		uint32_t* samples = sampleAdc();
		double vin1 = calcCh1Input(samples[0]);
		double vin2 = calcCh1Input(samples[1]);

		sprintf(buffer, "Vin1=%f Vin2=%f\n", vin1, vin2);
		UARTprintf(buffer);
	}

	return 0;
}
