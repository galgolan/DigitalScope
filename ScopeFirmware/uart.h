/*
 * uart.h
 *
 *  Created on: Jun 6, 2015
 *      Author: t-galgo
 */

#ifndef UART_H_
#define UART_H_

#include "config.h"

void configUART();

void outputDebug(double vin1, double vin2);

void outputTrigger();

void outputData(float ch1, float ch2);

void UartISR(void);

#endif /* UART_H_ */
