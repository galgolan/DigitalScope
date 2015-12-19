#include <stdint.h>
#include <stdbool.h>

// board definitions
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

#include "driverlib/ssi.h"
#include "spi.h"
#include "scope_common.h"
#include "config.h"
#include "../common/common.h"
#include "calc.h"

#define PGA1_SS_PORT	GPIO_PORTP_BASE
#define PGA2_SS_PORT	GPIO_PORTN_BASE
#define DAC1_SS_PORT	GPIO_PORTN_BASE
#define DAC2_SS_PORT	GPIO_PORTL_BASE

#define	PGA1_SS_PIN		GPIO_PIN_2
#define PGA1_SS_PIN_NUM	2
#define	PGA2_SS_PIN		GPIO_PIN_2
#define PGA2_SS_PIN_NUM	2
#define	DAC1_SS_PIN		GPIO_PIN_3
#define DAC1_SS_PIN_NUM	3
#define DAC2_SS_PIN		GPIO_PIN_3
#define DAC2_SS_PIN_NUM	3

#define SSI_BASE	SSI2_BASE

const uint32_t dacRes = 4096;
const double dacReference = 3.3;

static volatile double dac1_voltages[2] = {0.0, 0.0};
static volatile double dac2_voltages[2] = {0.0, 0.0};

static volatile uint32_t dac1Gain = 1;
static volatile uint32_t dac2Gain = 1;

static volatile uint8_t pga1GainRegister = PGA_GAIN_1;
static volatile uint8_t pga2GainRegister = PGA_GAIN_1;

int8_t translateGain(byte gainValue, int originalGain);

void setDac1Voltage(double voltage, int channel);

void setDac2Voltage(double voltage, int channel);

void setPga1Gain(uint8_t gain);

void setPga2Gain(uint8_t gain);

void setPga1Channel(uint8_t channel);

void setPga2Channel(uint8_t channel);

void busyWaitForSSi()
{
	while(SSIBusy(SSI_BASE))
	{
	}
}

void configSlaveSelectPins()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);

	GPIOPinTypeGPIOOutput(PGA1_SS_PORT, PGA1_SS_PIN);
	GPIOPinTypeGPIOOutput(PGA2_SS_PORT, PGA2_SS_PIN);
	GPIOPinTypeGPIOOutput(DAC1_SS_PORT, DAC1_SS_PIN);
	GPIOPinTypeGPIOOutput(DAC2_SS_PORT, DAC2_SS_PIN);

	GPIOPinWrite(PGA1_SS_PORT, PGA1_SS_PIN, HIGH << PGA1_SS_PIN_NUM);
	GPIOPinWrite(PGA2_SS_PORT, PGA2_SS_PIN, HIGH << PGA2_SS_PIN_NUM);
	GPIOPinWrite(DAC1_SS_PORT, DAC1_SS_PIN, HIGH << DAC1_SS_PIN_NUM);
	GPIOPinWrite(DAC2_SS_PORT, DAC2_SS_PIN, HIGH << DAC2_SS_PIN_NUM);
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

	GPIOPinConfigure(GPIO_PD3_SSI2CLK);
	GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
	GPIOPinConfigure(GPIO_PD2_SSI2FSS);
	GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_3 | GPIO_PIN_1);

	// Configure the SSI.
	SSIClockSourceSet(SSI_BASE, SSI_CLOCK_SYSTEM);
	ScopeConfig* config = getConfig();
	SSIConfigSetExpClk(SSI_BASE, config->systClock, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 100000, 8);
	SSIEnable(SSI_BASE);
	SysCtlDelay(1000);
}

void setGain()
{
	ScopeConfig* config = getConfig();
	setPga1Gain(config->channels[0].gain);
	setPga2Gain(config->channels[1].gain);
}

void setOffset()
{
	ScopeConfig* config = getConfig();

	double vdacCh1 = calcVDacFromOffset(config->channels[0].offset);
	double vdacCh2 = calcVDacFromOffset(config->channels[1].offset);
	setDac2Voltage(vdacCh1, 1);
	setDac2Voltage(vdacCh2, 2);
}

void setCompRef(double v)
{
	setDac1Voltage(v, 2);
}

void configureAnalogFrontend()
{
	// set reference voltage for both PGAs
	setDac1Voltage(VCC/2, 1);
	setCompRef(0.0);

	setOffset();

	setPga1Channel(PGA_CHANNEL_0);
	setPga2Channel(PGA_CHANNEL_0);
	setGain();
}

uint16_t calcDacLevel(double voltage, int dacGain)
{
	return (dacRes-1) * voltage / ((double)dacGain * dacReference);
}

void programPga(uint8_t inst, uint8_t data, uint32_t ssPort, uint8_t ssPin, uint8_t ssPinNum)
{
	GPIOPinWrite(ssPort, ssPin, LOW << ssPinNum);
	SSIDataPut(SSI_BASE, inst);
	busyWaitForSSi();
	SSIDataPut(SSI_BASE, data);
	busyWaitForSSi();
	GPIOPinWrite(ssPort, ssPin, HIGH << ssPinNum);
}

void programPga1(uint8_t inst, uint8_t data)
{
	programPga(inst, data, PGA1_SS_PORT, PGA1_SS_PIN, PGA1_SS_PIN_NUM);
}

void programPga2(uint8_t inst, uint8_t data)
{
	programPga(inst, data, PGA2_SS_PORT, PGA2_SS_PIN, PGA2_SS_PIN_NUM);
}

void setPga1Gain(uint8_t gain)
{
	uint8_t gainRegister = translateGain(gain, pga1GainRegister);
	programPga1(PGA_SET_GAIN, gainRegister);
	pga1GainRegister = gainRegister;
}

void setPga2Gain(uint8_t gain)
{
	uint8_t gainRegister = translateGain(gain, pga2GainRegister);
	programPga2(PGA_SET_GAIN, gainRegister);
	pga2GainRegister = gainRegister;
}

void setPga1Channel(uint8_t channel)
{
	programPga1(PGA_SET_CHANNEL, channel);
}

bool isValidGain(int gain)
{
	return translateGain(gain, -1) != -1;
}

int8_t translateGain(byte gainValue, int originalGain)
{
	//1;2;4;5;8;10;16;32
	switch(gainValue)
	{
	case 1:		return PGA_GAIN_1;
	case 2:		return PGA_GAIN_2;
	case 4:		return PGA_GAIN_4;
	case 5:		return PGA_GAIN_5;
	case 8:		return PGA_GAIN_8;
	case 10:		return PGA_GAIN_10;
	case 16:		return PGA_GAIN_16;
	case 32:		return PGA_GAIN_32;
	default:		return originalGain;		// invalid gain which should be ignored
	}
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
void setPga2Channel(uint8_t channel)
{
	programPga2(PGA_SET_CHANNEL, channel);
}

void programDac(int channel, double voltage, int gain, uint32_t ssPort, uint8_t ssPin, uint8_t ssPinNum)
{
	uint8_t config = DAC_UNBUFFERED | DAC_GAIN_1 | DAC_ACTIVE;
	if(channel == 1)
		config |= DAC_A;
	else
		config |= DAC_B;

	uint16_t dn = calcDacLevel(voltage, gain);
	uint16_t inst = (((uint16_t)config) << 12) | (dn & 0x0FFF);
	uint8_t msb = (inst & 0xFF00) >> 8;
	uint8_t lsb = (inst & 0x00FF);

	GPIOPinWrite(ssPort, ssPin, LOW << ssPinNum);
	SSIDataPut(SSI_BASE, msb);
	busyWaitForSSi();
	SSIDataPut(SSI_BASE, lsb);
	busyWaitForSSi();
	GPIOPinWrite(ssPort, ssPin, HIGH << ssPinNum);
}

void setDac1Voltage(double voltage, int channel)
{
	programDac(channel, voltage, dac1Gain, DAC1_SS_PORT, DAC1_SS_PIN, DAC1_SS_PIN_NUM);
	dac1_voltages[channel-1] = voltage;
}

void setDac2Voltage(double voltage, int channel)
{
	programDac(channel, voltage, dac2Gain, DAC2_SS_PORT, DAC2_SS_PIN, DAC2_SS_PIN_NUM);
	dac2_voltages[channel-1] = voltage;
}

double getDac1Voltage(int channel)
{
	return dac1_voltages[channel-1];
}

double getDac2Voltage(int channel)
{
	return dac2_voltages[channel-1];
}
