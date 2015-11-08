/*
 * calc.h
 *
 *  Created on: Jun 5, 2015
 *      Author: t-galgo
 */

#ifndef CALC_H_
#define CALC_H_

double calcVinFromVout2(int channel, double vout2);

double calcVout2FromVin(int channel, double vin);

double calcVDacFromOffset(double offset);

double calcVinFromSample(int channel, uint16_t sample);

double calcOffsetFromVin(int channel, double vout2);

#endif /* CALC_H_ */
