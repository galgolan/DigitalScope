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

// include these for working with ROM functions
//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"

#include "driverlib/sysctl.h"
#include "driverlib/fpu.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "driverlib/comp.h"
#include "driverlib/watchdog.h"

// utilities
#include "utils/uartstdio.h"

#include "spi.h"
#include "calc.h"
#include "adc.h"
#include "uart.h"
#include "config.h"
#include "probecomp.h"

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
//uint32_t g_ui32SysClock;

void configureFPU()
{
	FPUEnable();
	FPULazyStackingEnable();
}

void setup()
{
	ScopeConfig* config = getConfig();

	configureFPU();

	SysCtlDelay(1000);

	// Run from the PLL at 120 MHz.
	config->systClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
					SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
					SYSCTL_CFG_VCO_480), 120000000);

	if (config->systClock == 0)
	{
		while(1)
		{

		}
	}
	SysCtlDelay(1000);

	configUART();
	configAdc();
	configSPI();
	configureAnalogFrontend();
	configProbeCompensation();

	IntMasterEnable();
}

void waitUntilReady()
{
	//ScopeConfig* config = getConfig();
	while(adcState == ADC_STATE_CAPTURING)
	{
		SysCtlDelay(1);	// TODO: i think there is no point in a delay (its not like we can starve a thread)
	}
}

void createConfig()
{
	ScopeConfig* config = getConfig();

	// configure trigger
	config->trigger.level = 0;
	config->trigger.type = TRIG_RISING;
	config->trigger.mode = TRIG_MODE_AUTO;
	config->trigger.source = TRIG_SRC_CH1;

	// configure ch1
	config->channels[0].active = true;
	config->channels[0].offset = 0;
	config->channels[0].gain = 1;

	// configure ch2
	config->channels[1].active = true;
	config->channels[1].offset = 0;
	config->channels[1].gain = 1;

	config->sampleRate = 1000;
}


/*
 * main.c
 */
int main(void)
{
	int i;

	IntMasterDisable();
	SysCtlPeripheralDisable(WATCHDOG0_BASE);
	SysCtlPeripheralDisable(WATCHDOG1_BASE);
	createConfig();

	SysCtlDelay(1000000);
	setup();
	SysCtlDelay(1000);

	while(1)
	{
		waitUntilReady();

		outputTrigger();

		// send all data
		for(i=0;i<BUFFER_SIZE;++i)
		{
			float ch1 = calcVinFromSample(0, samples_ch1[i]);
			float ch2 = calcVinFromSample(1, samples_ch2[i]);
			outputData(ch1, ch2);
		}
		adcState = ADC_STATE_CAPTURING;
		//ADCIntEnable(ADC0_BASE, 0);
	}

	return 0;
}
