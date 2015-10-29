/*
 * probecomp.h
 *
 *  Created on: Oct 26, 2015
 *      Author: galgo
 */

#ifndef PROBECOMP_H_
#define PROBECOMP_H_

//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"

// configures the probe compensation output to 1KHz 3.3Vpp square wave
void configProbeCompensation();

void probeCompISR(void);

#endif /* PROBECOMP_H_ */
