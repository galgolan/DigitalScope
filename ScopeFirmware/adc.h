/*
 * adc.h
 *
 *  Created on: Jun 6, 2015
 *      Author: t-galgo
 */

#ifndef ADC_H_
#define ADC_H_

#include "config.h"

typedef enum AdcState
{
	ADC_STATE_CAPTURING=1,		// adc is currently capturing data into buffer
	ADC_STATE_SUSPENDED=2			// adc is currently suspended (probably because the main loop is sending the data)
} AdcState;

extern volatile AdcState adcState;
extern uint16_t samples_ch1[BUFFER_SIZE];
extern uint16_t samples_ch2[BUFFER_SIZE];

// calculates the analog voltage from an ADC sample
double calcVoltage(uint32_t d);

uint32_t translateCompRef(float refValue);

void configAdc();

void triggerAdc();

void setTriggerSource();

void setTriggerLevel();

void setTriggerMode();

void setTriggerType();

void setSampleRate();

int sampleAdc(uint32_t* samples);

void AdcISR(void);

#endif /* ADC_H_ */
