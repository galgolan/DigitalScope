/*
 * adc.h
 *
 *  Created on: Jun 6, 2015
 *      Author: t-galgo
 */

#ifndef ADC_H_
#define ADC_H_

void configAdc();

void triggerAdc();

void sampleAdc(uint32_t* samples);

void AdcISR(void);

#endif /* ADC_H_ */
