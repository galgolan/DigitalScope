/*
 * calc.c
 *
 *  Created on: Jun 5, 2015
 *      Author: t-galgo
 */
#include <stdint.h>
#include <stdbool.h>

// board definitions
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#include "calc.h"
#include "spi.h"
#include "spi.h"

const double analogRef = 3.3;
const double adcRes = 4096;

const double b1 = 0.0102;
const double m1 = 0.069051;

const double b2 = 0.0059;
const double m2 = 0.068276;

const double vref = 1.65;

double calcInputVoltage(double voltage, uint8_t gain, double m, double b)
{
	double num = voltage - (gain * b) + vref * (gain-1);
	double den = m * gain;
	return num/den;
}

double calcOutputVoltage(uint32_t d)
{
	double tmp = (analogRef * d);
	return tmp / (double)(adcRes-1);
}

double calcCh1Input(uint32_t d)
{
	double vout = calcOutputVoltage(d);
	uint8_t gain = getPga1Gain();
	float dacVoltage = getDac2Voltage(1);
	return calcInputVoltage(vout, gain, m1, b1 + dacVoltage);
}

double calcCh2Input(uint32_t d)
{
	double vout = calcOutputVoltage(d);
	uint8_t gain = getPga2Gain();
	float dacVoltage = getDac2Voltage(2);
	return calcInputVoltage(vout, gain, m2, b2 + dacVoltage);
}

double calcCh1Offset(float volts)
{
	double offset = volts * m1 + analogRef/2;
	if(offset < 0)
		return 0;
	if(offset > analogRef)
		return analogRef;

	return offset;
}

double calcCh2Offset(float volts)
{
	double offset = volts * m2 + analogRef/2;
	if(offset < 0)
		return 0;
	if(offset > analogRef)
		return analogRef;

	return offset;
}
