/*
  Microchip PGA control
  
 The circuit:
  * CS - to digital pin 10  (SS pin)
  * SDI - to digital pin 11 (MOSI pin)
  * SDO - to digital pin 12 (MISO pin)
  * CLK - to digital pin 13 (SCK pin)
*/

// inslude the SPI library:
#include <SPI.h>

#define INST_SET_GAIN    B01000000
#define INST_SET_CHA     B01000001

#define GAIN_1   B00000000
#define GAIN_2   B00000001
#define GAIN_4   B00000010
#define GAIN_5   B00000011
#define GAIN_8   B00000100
#define GAIN_10  B00000101
#define GAIN_16  B00000110
#define GAIN_32  B00000111

#define CHN0    B00000000
#define CHN1    B00000001

#define DAC_A  		B0000
#define DAC_B  		B1000
#define DAC_BUFFERED  	B0100
#define DAC_UNBUFFERED  B0000
#define DAC_GAIN_1  	B0010
#define DAC_GAIN_2  	B0000
#define DAC_ACTIVE  	B0001
#define DAC_SHUTDOWN  	B0000

// set pin 10 as the slave select for the digital pot:
const int slaveSelectPin = 10;
const int dacSelectPin = 9;

const int gain_0 = 2;  // yellow
const int gain_1 = 3;  // red
const int gain_2 = 4;  // green

// v = (gain*d*ref)/4096
// gain*d*ref=v*4096
// d = 4096*v/(gain*ref)

double vref = 4.92;
const int res = 4096;

int calcD(double voltage, int gain)
{
  return res * voltage / ((double)gain * vref);
}

double calcVoltage(int d)
{
  double tmp = vref * d;
  return tmp / (double)1024;
}

byte lastGain;
double lastVoltage = 2.00;
int analogPin = 5;

void setupSpi()
{
  pinMode (slaveSelectPin, OUTPUT);
  pinMode (dacSelectPin, OUTPUT);
  // initialize SPI:
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV32);  // 16MHz/32=0.5MHz
  SPI.setBitOrder(MSBFIRST);
  SPI.begin();
}

void setupDip()
{
  pinMode(gain_0, INPUT);
  pinMode(gain_1, INPUT);
  pinMode(gain_2, INPUT);
}

// vout = (vref * d) / 2^12

void setup() {
  analogReference(INTERNAL);
  Serial.begin(9600);
  delay(100);
  setupDip();
  // set the slaveSelectPin as an output:
  setupSpi();
  selectChannel(CHN0);
  lastGain = GAIN_1;
  selectGain(lastGain);
  programDac(DAC_UNBUFFERED | DAC_GAIN_1 | DAC_ACTIVE | DAC_A, lastVoltage);
 }
 
 double calcInput(double voltage)
 {
   return (voltage-lastGain * lastVoltage * 1.253373346)/(lastGain * 0.083874753);
 }

void loop()
{
  changeGain();    // set gain
  programDac(DAC_BUFFERED | DAC_GAIN_1 | DAC_ACTIVE | DAC_A, lastVoltage);
  
  delay(500);

  int analogValue = analogRead(A0);
  double v = calcVoltage(analogValue);
  Serial.print("Gain=");
  Serial.print(lastGain);
  Serial.print(", Voffset=");
  Serial.print(lastVoltage);
  Serial.print(", Vout=");
  Serial.print(v);
  Serial.print(", Vin=");
  Serial.println(calcInput(v));
}

void selectChannel(byte channel)
{
  programPga(INST_SET_CHA, channel);
}

void selectGain(byte gain)
{
  programPga(INST_SET_GAIN, gain);
}

void programPga(byte instruction, byte data)
{
  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(instruction);
  SPI.transfer(data);
  digitalWrite(slaveSelectPin, HIGH);
}

void programDac(byte configuration, double voltage)
{
  int dn = calcD(voltage, 1);
  digitalWrite(dacSelectPin, LOW);
  unsigned int instruction = (((unsigned int)configuration) << 12) | (dn & 0x0FFF);
  
  byte msb = (instruction & 0xFF00) >> 8;
  byte lsb = (instruction & 0x00FF);
  
  SPI.transfer(msb);
  SPI.transfer(lsb);
  digitalWrite(dacSelectPin, HIGH);
}

void changeGain()
{
  // read DIP switches
  byte b0 = digitalRead(gain_0);
  byte b1 = digitalRead(gain_1);
  byte b2 = digitalRead(gain_2);
  
  int gain = (b2 << 2) | (b1 << 1) | b0;
  
  if(lastGain != gain)
    selectGain(gain);
    
  lastGain = gain;
}
