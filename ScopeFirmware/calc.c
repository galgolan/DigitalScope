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
#include "adc.h"
#include "config.h"

const double vref = VCC/2;

const double m[NUM_CHANNELS] = { 0.069051, 0.068276};
const double b[NUM_CHANNELS] = { 1.6602, 1.6559 };

//const double b1 = 0.0102;
//const double m1 = 0.069051;

//const double b2 = 0.0059;
//const double m2 = 0.068276;

// Vout1 = Vin * m + b + offset
// Vdac = Vref + offset
// Vout2 = (Vout1 - Vref) * G + Vref
//       = (Vin * m + b + offset - Vref) * G + Vref
//		 = (Vin * m + b + Vdac - 2*Vref) * G + Vref

// Vin = Vout2 - G*(b+offset) + Vref(G-1)
//		---------------------------------
//					mG

double calcVinFromVout2(int channel, double vout2)
{
	ScopeConfig* config = getConfig();
	uint8_t g = config->channels[channel].gain;
	float offset = config->channels[channel].offset;

	double num = vout2 - g * (b[channel] + offset) + vref * (g-1);
	double den = m[channel] * g;
	return num / den;
}

double calcVout2FromVin(int channel, double vin)
{
	ScopeConfig* config = getConfig();
	float offset = config->channels[channel].offset;
	uint8_t g = config->channels[channel].gain;

	double vout1 = vin * m[channel] + b[channel] + offset;
	return (vout1 - vref) * g + vref;
}

double calcVDacFromOffset(double offset)
{
	return vref + offset;
}

double calcVinFromSample(int channel, uint16_t sample)
{
	double vout2 = calcVoltage(sample);
	return calcVinFromVout2(channel, vout2);
}

double calcOffsetFromVin(int channel, double vin)
{
	return calcVout2FromVin(channel, vin) - vref;
}
