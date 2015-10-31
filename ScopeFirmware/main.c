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
uint32_t g_ui32SysClock;

void configureAnalogFrontend()
{
	ScopeConfig* config = getConfig();
	int i;
	for(i=1; i <= NUM_CHANNELS; ++i)
	{
		setDacVoltage(VCC/2, i);
		setDacVoltage(VCC/2, i);
	}

	setPga1Channel(PGA_CHANNEL_0);
	setPga2Channel(PGA_CHANNEL_0);
	setPga1Gain(config->channels[0].gain);
	setPga2Gain(config->channels[1].gain);
}

void configureFPU()
{
	FPUEnable();
}

void setup()
{
	ScopeConfig* config = getConfig();

	configureFPU();

	SysCtlDelay(1000);

	// Run from the PLL at 120 MHz.
	g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
					SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
					SYSCTL_CFG_VCO_480), 120000000);

	// TODO: if g_ui32SysClock == 0 error
	SysCtlDelay(1000);

	configUART(g_ui32SysClock);
	configAdc(config->trigger);
	configSPI();
	configureAnalogFrontend();
	configProbeCompensation();
}

void waitUntilReady()
{
	while(!ready)
	{
		SysCtlDelay(g_ui32SysClock / 10);
	}
}

void reArmTrigger()
{
	ScopeConfig* config = getConfig();
	ready = false;
	if(config->trigger.mode == TRIG_MODE_FREE_RUNNING)
	{
		triggerAdc();
	}
	else if (config->trigger.mode == TRIG_MODE_AUTO)
	{
		configAdc(config->trigger);
		ADCComparatorIntEnable(ADC0_BASE, 0);
	}
	else if (config->trigger.mode == TRIG_MODE_SINGLE)
	{
		// nothing to do
	}
}

void createConfig()
{
	ScopeConfig* config = getConfig();

	// configure trigger
	config->trigger.level = COMP_REF_1_65V;
	config->trigger.type = TRIG_BOTH;
	config->trigger.mode = TRIG_MODE_AUTO;
	config->trigger.source = TRIG_SRC_CH2;

	//config->trigger.mode = TRIG_MODE_FREE_RUNNING;

	// configure ch1
	config->channels[0].active = true;
	config->channels[0].gain = PGA_GAIN_1;

	// configure ch2
	config->channels[1].active = true;
	config->channels[1].gain = PGA_GAIN_1;
}

void freerunning()
{
	uint32_t samples[2];
	int numSamples = sampleAdc(samples);
	double ch1 = calcCh1Input(samples[0]);
	double ch2 = calcCh2Input(samples[1]);
	outputData(ch1, ch2);
}

void triggered()
{
	int i;
	waitUntilReady();
	//outputDebug(samples_ch1[0], samples_ch2[0]);
	//outputDebugMany(samples_ch1, samples_ch2, BUFFER_SIZE);
	outputTrigger();
	for(i=0;i<BUFFER_SIZE;++i)
	{
		outputData(samples_ch1[i], samples_ch2[i]);
	}
	reArmTrigger();
}

/*
 * main.c
 */
int main(void)
{
	createConfig();

	SysCtlDelay(1000000);
	setup();
	SysCtlDelay(1000);
	IntMasterEnable();

	while(1)
	{
		triggered();
		//freerunning();
	}

	return 0;
}
