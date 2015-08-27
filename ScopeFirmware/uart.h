/*
 * uart.h
 *
 *  Created on: Jun 6, 2015
 *      Author: t-galgo
 */

#ifndef UART_H_
#define UART_H_

#include "config.h"

void configUART(uint32_t sysClock);

void outputDebug(double vin1, double vin2);

void outputDebugMany(double* ch1, double* ch2, uint32_t n);

void UartISR(void);

#endif /* UART_H_ */
