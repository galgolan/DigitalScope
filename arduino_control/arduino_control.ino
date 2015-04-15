/*
  Microchip PGA control
  
 The circuit:
  * CS - to digital pin 10  (SS pin)
  * SDI - to digital pin 11 (MOSI pin)
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

// set pin 10 as the slave select for the digital pot:
const int slaveSelectPin = 10;

const int gain_0 = 2;  // yellow
const int gain_1 = 3;  // red
const int gain_2 = 4;  // green

byte lastGain;

void setupSpi()
{
  pinMode (slaveSelectPin, OUTPUT);
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

void setup() {
  delay(100);
  setupDip();
  // set the slaveSelectPin as an output:
  setupSpi();
  selectChannel(CHN0);
  lastGain = GAIN_1;
  selectGain(lastGain);
 }

void loop() {
  changeGain();    // set gain
  delay(500);
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

void programDac()
{
  // todo
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
}
