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
#include "inc/hw_adc.h"

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#include "driverlib/adc.h"
#include "driverlib/comp.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"

#include "adc.h"
#include "uart.h"
#include "calc.h"
#include "config.h"
#include "scope_common.h"
#include "spi.h"

//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"

#define MUX_CHANNEL_1	0b00000000		//Y3
#define MUX_CHANNEL_2	0b00000001		//Y4

#define ADC_SSTSH               (ADC_O_SSTSH0 - ADC_O_SSMUX0)

typedef enum AdcInterruptSource
{
	ADC_INTERRUPT_SRC_NONE = 0,
	ADC_INTERRUPT_SRC_TIMER,
	ADC_INTERRUPT_SRC_COMP
} AdcInterruptSource;

uint16_t samples_ch1[BUFFER_SIZE];
uint16_t samples_ch2[BUFFER_SIZE];
static volatile uint32_t index = 0;

volatile AdcState adcState = ADC_STATE_CAPTURING;
AdcInterruptSource interruptSource = ADC_INTERRUPT_SRC_NONE;

const double adcRes = 4096;
const double analogRef = VCC;

double calcVoltage(uint32_t d)
{
	double tmp = (analogRef * d);
	return tmp / (double)(adcRes-1);
}

uint32_t translateCompRef(float refValue)	// todo: take into account the offset
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

void setTriggerSource()
{
	uint8_t muxSelect;
	ScopeConfig* config = getConfig();

	switch(config->trigger.source)
	{
	case TRIG_SRC_CH1:
		muxSelect = MUX_CHANNEL_1;
		break;
	case TRIG_SRC_CH2:
		muxSelect = MUX_CHANNEL_2;
		break;
	default:
		// bad trigger src, do nothing
		return;
	}

	// change the channel on the mux
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, muxSelect << 1);
}

void setTriggerLevel()
{
	ScopeConfig* config = getConfig();
	int channel;
	if(config->trigger.source == TRIG_SRC_CH1) channel = 0;
	else if (config->trigger.source == TRIG_SRC_CH2) channel = 1;
	double vout2 = calcVout2FromVin(channel, config->trigger.level);
	setCompRef(vout2);
}

void configureSequencer(uint32_t trigger)
{
	// configure sequencer with 2 steps: sample ch3, sample ch4
	ADCSequenceDisable(ADC0_BASE, 0);
	ADCSequenceConfigure(ADC0_BASE, 0, trigger, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH3);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH4);

	uint32_t sampleAndHold = HWREG(ADC_SSTSH + ADC0_BASE);
	HWREG(ADC_SSTSH + ADC0_BASE) = 0x00000000;
	//sampleAndHold = HWREG(ADC_SSTSH + ADC0_BASE);

	ADCSequenceEnable(ADC0_BASE, 0);
	ADCIntClear(ADC0_BASE, 0);
}

void setSampleRate()
{
	ScopeConfig* config = getConfig();
	uint32_t timer_value = config->systClock / config->sampleRate;
	TimerLoadSet(TIMER1_BASE, TIMER_A, timer_value);
}

void setAdcInterruptSourceTimer()
{
	interruptSource = ADC_INTERRUPT_SRC_TIMER;
	configureSequencer(ADC_TRIGGER_TIMER);
}

void setAdcInterruptSourceComparator()
{
	interruptSource = ADC_INTERRUPT_SRC_COMP;
	configureSequencer(ADC_TRIGGER_COMP0);
}

void setTriggerMode()
{
	ScopeConfig* config = getConfig();
	if(config->trigger.mode == TRIG_MODE_FREE_RUNNING)
		setAdcInterruptSourceTimer();
	else
		setAdcInterruptSourceComparator();
}

void setTriggerType()
{
	ScopeConfig* config = getConfig();
	uint32_t type = getTriggerType(config->trigger.type);

	// use external Comp+ as voltage reference C6
	// we invert the output because the test signal is connected to the negative input
	ComparatorConfigure(COMP_BASE, 0, type | COMP_ASRCP_PIN | COMP_OUTPUT_INVERT);
}

void configMux()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

	setTriggerSource();
}

void configComparator()
{
	ScopeConfig* config = getConfig();
	uint32_t type = getTriggerType(config->trigger.type);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);
	GPIOPinConfigure(GPIO_PL2_C0O);

	GPIOPinTypeComparator(GPIO_PORTC_BASE, GPIO_PIN_7);		// neg input (PC7)
	GPIOPinTypeComparator(GPIO_PORTC_BASE, GPIO_PIN_6);		// pos input / reference voltage
	GPIOPinTypeComparator(GPIO_PORTL_BASE, GPIO_PIN_2);		// out

	setTriggerLevel();
	setTriggerType();

	SysCtlDelay(1000);
}

void setupSamplingTimer()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
	setSampleRate();
	TimerControlTrigger(TIMER1_BASE, TIMER_A, true);
	TimerEnable(TIMER1_BASE, TIMER_A);
}

void configAdc()
{
	ScopeConfig* config = getConfig();

	adcState = ADC_STATE_CAPTURING;
	interruptSource = ADC_INTERRUPT_SRC_NONE;
	index = 0;

	configComparator();
	configMux();

	// ch1 - A3 (PE0)
	// ch2 - A4 (PD7)

	// enable ADC0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	// enable required ports
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

	SysCtlDelay(1000);

	ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, 15);

	// configure adc pins
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_7);
	SysCtlDelay(1000);
	// configure adc
	ADCReferenceSet(ADC0_BASE, ADC_REF_INT);
	SysCtlDelay(1000);

	setupSamplingTimer();

	setTriggerMode();

	// enable ADC0 sequencer 0 interrupt
	ADCIntClear(ADC0_BASE, 0);
	ADCIntEnable(ADC0_BASE, 0);
	IntEnable(INT_ADC0SS0);
}

void AdcISR(void)
{
	//IntMasterDisable();
	ADCIntClear(ADC0_BASE, 0);

	//IntPendClear(INT_ADC0SS0);	// TODO: where to put this ?

	if(adcState == ADC_STATE_CAPTURING)
	{
		uint32_t samples[8];
		// Read the value from the ADC.
		int32_t n = ADCSequenceDataGet(ADC0_BASE, 0, samples);
		samples_ch1[index] = samples[n-2];
		samples_ch2[index] = samples[n-1];
		++index;

		if(index >= BUFFER_SIZE)
		{
			// aquisition completed
			adcState = ADC_STATE_SUSPENDED;
			index = 0;
			ScopeConfig* config = getConfig();
			// if triggered - disable timer, enable comparator
			if((config->trigger.mode != TRIG_MODE_FREE_RUNNING) && (interruptSource == ADC_INTERRUPT_SRC_TIMER))
			{
				setAdcInterruptSourceComparator();
			}
		}
		else
		{
			// if interrupt was comparator - start timer
			if (interruptSource == ADC_INTERRUPT_SRC_COMP)
				setAdcInterruptSourceTimer();
		}
	}
	//IntMasterEnable();
}
