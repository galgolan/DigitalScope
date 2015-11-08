#ifndef _SPI_H_
#define _SPI_H_

#include "../common/common.h"

// PGA constants
#define PGA_SET_GAIN	0x40
#define PGA_SET_CHANNEL	0x41
#define PGA_GAIN_1		0x0
#define PGA_GAIN_2		0x1
#define PGA_GAIN_4		0x2
#define PGA_GAIN_5		0x3
#define PGA_GAIN_8		0x4
#define PGA_GAIN_10		0x5
#define PGA_GAIN_16		0x6
#define PGA_GAIN_32		0x7
#define PGA_CHANNEL_0	0x0
#define PGA_CHANNEL_1	0x1

// DAC constants
#define DAC_ACTIVE		0x1
#define DAC_SHUTDOWN	0x0
#define DAC_GAIN_1		0x2
#define DAC_GAIN_2		0x0
#define DAC_UNBUFFERED	0x0
#define DAC_BUFFERED	0x4
#define DAC_A 			0x0
#define DAC_B 			0x8

void configSPI();

void configureAnalogFrontend();

void setGain();

void setOffset();

int8_t translateGain(byte gainValue, int originalGain);

void setDac1Voltage(double voltage, int channel);

double getDac1Voltage(int channel);

void setDac2Voltage(double voltage, int channel);

double getDac2Voltage(int channel);

void setPga1Gain(uint8_t gain);

void setPga2Gain(uint8_t gain);

void setPga1Channel(uint8_t channel);

uint8_t getPga1Gain();

void setPga2Channel(uint8_t channel);

uint8_t getPga2Gain();

#endif
