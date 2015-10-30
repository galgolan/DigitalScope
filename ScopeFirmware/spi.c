#include <stdint.h>
#include <stdbool.h>

// board definitions
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#include "driverlib/ssi.h"
#include "spi.h"
#include "scope_common.h"

#define PGA1_SS_PORT	GPIO_PORTP_BASE
#define PGA2_SS_PORT	GPIO_PORTN_BASE
#define DAC_SS_PORT		GPIO_PORTN_BASE

#define	PGA1_SS_PIN		GPIO_PIN_2
#define	PGA2_SS_PIN		GPIO_PIN_2
#define	DAC_SS_PIN		GPIO_PIN_3

#define SSI_BASE	SSI2_BASE

static volatile double voltages[2] = {0, 0};

const double dacReference = 3.3;
static volatile uint32_t dacGain = 1;
const uint32_t dacRes = 4096;

static volatile uint8_t pga1GainRegister = 1;
static volatile uint8_t pga2GainRegister = 1;

void configSlaveSelectPins()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

	// pga1 cs - pin 11 PP2
	// pga2 cs - pin 13 PN2
	// dac cs - pin 12 PN3
	GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_2);
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_2);

	GPIOPinWrite(DAC_SS_PORT, DAC_SS_PIN, HIGH);
	GPIOPinWrite(PGA1_SS_PORT, PGA1_SS_PIN, HIGH);
	GPIOPinWrite(PGA2_SS_PORT, PGA2_SS_PIN, HIGH);
}

// config SSI2
void configSPI()
{
	configSlaveSelectPins();

	// CLK2	-	PD3
	// MOSI2	PD1
	// CS2		PD2

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	SysCtlDelay(1000);

	GPIOPinConfigure(GPIO_PD3_SSI2CLK);
	GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
	GPIOPinConfigure(GPIO_PD2_SSI2FSS);
	GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_3 | GPIO_PIN_1);

	// Configure the SSI.
	SSIConfigSetExpClk(SSI_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 500000, 8);
	SSIEnable(SSI_BASE);
}

uint16_t calcDacLevel(double voltage)
{
	return (dacRes-1) * voltage / ((double)dacGain * dacReference);
}

void programPga(uint8_t inst, uint8_t data, uint32_t ssPort, uint32_t ssPin)
{
	GPIOPinWrite(ssPort, ssPin, LOW);
	SSIDataPut(SSI_BASE, inst);
	SSIDataPut(SSI_BASE, data);
	GPIOPinWrite(ssPort, ssPin, HIGH);
}

void programPga1(uint8_t inst, uint8_t data)
{
	programPga(inst, data, PGA1_SS_PORT, PGA1_SS_PIN);
}

void programPga2(uint8_t inst, uint8_t data)
{
	programPga(inst, data, PGA2_SS_PORT, PGA2_SS_PIN);
}

void setPga1Gain(uint8_t gain)
{
	programPga1(PGA_SET_GAIN, gain);
	pga1GainRegister = gain;
}

void setPga2Gain(uint8_t gain)
{
	programPga2(PGA_SET_GAIN, gain);
	pga2GainRegister = gain;
}

void setPga1Channel(uint8_t channel)
{
	programPga1(PGA_SET_CHANNEL, channel);
}

uint8_t getPgaGain(uint8_t gainRegister)
{
	switch(gainRegister)
	{
	case PGA_GAIN_1: return 1;
	case PGA_GAIN_2: return 2;
	case PGA_GAIN_4: return 4;
	case PGA_GAIN_5: return 5;
	case PGA_GAIN_8: return 8;
	case PGA_GAIN_10: return 10;
	case PGA_GAIN_16: return 16;
	case PGA_GAIN_32: return 32;
	}

	return 0;
}

uint8_t getPga1Gain()
{
	return getPgaGain(pga1GainRegister);
}

void setPga2Channel(uint8_t channel)
{
	programPga2(PGA_SET_CHANNEL, channel);
}

uint8_t getPga2Gain()
{
	return getPgaGain(pga2GainRegister);
}

void programDac(uint8_t config, double voltage)
{
	uint16_t dn = calcDacLevel(voltage);
	uint16_t inst = (((uint16_t)config) << 12) | (dn & 0x0FFF);
	uint8_t msb = (inst & 0xFF00) >> 8;
	uint8_t lsb = (inst & 0x00FF);

	GPIOPinWrite(DAC_SS_PORT, DAC_SS_PIN, LOW);
	SSIDataPut(SSI_BASE, msb);
	SSIDataPut(SSI_BASE, lsb);
	GPIOPinWrite(DAC_SS_PORT, DAC_SS_PIN, HIGH);
}

void setDacVoltage(double voltage, int channel)
{
	uint8_t config = DAC_UNBUFFERED | DAC_GAIN_1 | DAC_ACTIVE;
	if(channel == 1)
		config |= DAC_A;
	else
		config |= DAC_B;

	programDac(config, voltage);
	voltages[channel-1] = voltage;
}

double getDacVoltage(int channel)
{
	return voltages[channel-1];
}
