/*
 * adc.h
 *
 *  Created on: Jun 6, 2015
 *      Author: t-galgo
 */

#ifndef ADC_H_
#define ADC_H_

#include "config.h"

extern volatile bool ready;
extern double samples_ch1[BUFFER_SIZE];
extern double samples_ch2[BUFFER_SIZE];

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
