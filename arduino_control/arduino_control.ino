#include <MCP49x2.h>
#include <mcp6s2x.h>

/*
  Digital Scope MCU Stub
  
 The circuit:
  * CS - to digital pin 10  (SS pin)
  * SDI - to digital pin 11 (MOSI pin)
  * CLK - to digital pin 13 (SCK pin)
*/

// inslude the SPI library:
#include <SPI.h>

// dip switches
const int gain_0 = 2;
const int gain_1 = 3;
const int gain_2 = 4;

byte lastGain;
MCP6s2x pga(10);
MCP49x2 dac(14);

void setupDip()
{
  pinMode(gain_0, INPUT);
  pinMode(gain_1, INPUT);
  pinMode(gain_2, INPUT);
}

void setup()
{
  delay(100);
  
  setupDip();
  pga.start();
  
  pga.setChannel(0);
  lastGain = GAIN_1;
  pga.setGain(GAIN_1);
  
  dac.program(DAC_A | DAC_BUFFERED | DAC_GAIN_1 | DAC_ACTIVE, 0x000);
}

void loop()
{
  changeGain();    // set gain
  delay(500);
}

void changeGain()
{
  // read DIP switches
  byte b0 = digitalRead(gain_0);
  byte b1 = digitalRead(gain_1);
  byte b2 = digitalRead(gain_2);
  
  int gain = (b2 << 2) | (b1 << 1) | b0;
  
  if(lastGain != gain)
    pga.setGain(gain);
}
