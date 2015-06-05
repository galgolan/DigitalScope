#ifndef _SPI_H_
#define _SPI_H_

#define DAC_ACTIVE		0x1
#define DAC_SHUTDOWN	0x0
#define DAC_GAIN_1		0x2
#define DAC_GAIN_2		0x0
#define DAC_UNBUFFERED	0x0
#define DAC_BUFFERED	0x4
#define DAC_A 			0x0
#define DAC_B 			0x8

void configSPI();

void setDacVoltage(double voltage, int channel);

double getDacVoltage(int channel);

#endif
