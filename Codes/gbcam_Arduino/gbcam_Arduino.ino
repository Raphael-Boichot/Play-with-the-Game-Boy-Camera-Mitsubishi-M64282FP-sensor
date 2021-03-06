//Simplified and completely rewritten by Raphaël BOICHOT in 2022/03/07 to be Arduino/ESP8266/ESP32 compatible
//from a code of Laurent Saint-Marcel (lstmarcel@yahoo.fr) written in 2005/07/05
//once cleaned and updated, the code is in fact damn simple to understand
//the ADC of ESP/Arduino is very slow (successive approximations) compared to Game Boy Camera (Flash ADC).  
//ADC slowness is the bottleneck of the code, just reading 128x128 pixels take 1.8 seconds
//to perform live image rendering, the use of an external flash ADC is probably mandatory or try using https://github.com/avandalen/avdweb_AnalogReadFast

#include "avdweb_AnalogReadFast.h"

#define NOP __asm__ __volatile__ ("nop\n\t") //// delay 62.5ns on a 16MHz Arduino

int VOUT =  A3; //Analog signal from sensor, read shortly after clock is set low, converted to 10 bits by Arduino/ESP ADC then reduced to 8-bits for transmission
int READ =  8; //Read image signal, goes high on rising clock
int CLOCK = 9; //Clock input, pulled down internally, no specification given for frequency
int RESET = 10; //system reset, pulled down internally, active low, sent on rising edge of clock
int LOAD =  11; //parameter set enable, pulled down internally, Raise high as you clear the last bit of each register you send
int SIN =   12; //parameter input, pulled down internally
int START = 13; //Image sensing start, pulled down internally, has to be high before rising CLOCK
int LED =   4; //just for additionnal LED on D4, not essential to the protocol

unsigned char camReg[8]; //registers
unsigned char reg;
int cycles=10; //time delay in processor cycles, to fit with your ESP/Arduino frequency, here for Arduino Uno, about 0.62 µs

void setup()
{
  Serial.begin(2000000);
  pinMode(READ, INPUT);    // READ
  pinMode(CLOCK, OUTPUT);  // CLOCK
  pinMode(RESET, OUTPUT);  // RESET
  pinMode(LOAD, OUTPUT);   // LOAD
  pinMode(SIN, OUTPUT);    // SIN
  pinMode(START, OUTPUT);  // START
  pinMode(LED, OUTPUT);    // LED
  camInit();
  Serial.println("Version for Arduino with Analog Read Fast");
  Serial.println("Ready for transmission");
  delay(25);// just to be sure that the registers are sent on Octave side
}

// there is no special command to trigger sensor, as soon as 8 registers are received, an image is requested to the sensor
void loop()//the main loop just reads 8 registers from GNU Octave and send an image to serial in 8-bits binary
{
  if (Serial.available() == 8) {// do I have 8 bytes into the read buffer ?
    digitalWrite(LED, HIGH);
    for (reg = 0; reg < 8; ++reg) {
    camReg[reg] = Serial.read(); //read the 8 current registers sent by GNU Octave
    }
    delay(25);//the delay here is just to see the LED flashing without slowing down the protocol, can be removed if necessary
    digitalWrite(LED, LOW);

    Serial.print("Registers injected in sensor: ");
    for (reg = 0; reg < 8; ++reg) {
      Serial.print(camReg[reg], DEC);//sent them back to GNU Octave to be sure that everything is OK, may be removed
      Serial.print(" ");
    }
    Serial.println("");

    camReset();
    camSetRegisters();// Send 8 registers to the sensor
    camReadPicture(); // get pixels, dump them on serial, GNU Octave reads them and does the rest of the job
    Serial.println("");
    Serial.println("Image acquired");
    camReset();
    Serial.println("Sensor reset");
    Serial.println("Ready for transmission");
    delay(25);// just to be sure that the registers are sent on Octave side
  }
} //end of loop


void camDelay()// Allow a lag in processor cycles to maintain signals long enough
{
for (int i = 0; i < cycles; i++) NOP;
}

void camInit()// Initialise the IO ports for the camera
{
digitalWrite(CLOCK, LOW);
digitalWrite(RESET, HIGH);
digitalWrite(LOAD, LOW);
digitalWrite(START, LOW);
digitalWrite(SIN, LOW);
}

void camReset()// Sends a RESET pulse to sensor
{
digitalWrite(CLOCK, HIGH);
camDelay();
digitalWrite(CLOCK, LOW);
camDelay();
digitalWrite(RESET, LOW);
camDelay();
digitalWrite(CLOCK, HIGH);
camDelay();
digitalWrite(RESET, HIGH);
camDelay();
}

void camSetRegisters(void)// Sets the sensor 8 registers
{
  for (reg = 0; reg < 8; ++reg) {
    camSetReg(reg, camReg[reg]);
  }
}

void camSetReg(unsigned char regaddr, unsigned char regval)// Sets one of the 8 8-bit registers in the sensor
{
  unsigned char bitmask;
  for (bitmask = 0x4; bitmask >= 0x1; bitmask >>= 1) {// Write 3-bit address.
    digitalWrite(CLOCK, LOW);
    camDelay();
    digitalWrite(LOAD, LOW);// ensure load bit is cleared from previous call
    if (regaddr & bitmask)
      digitalWrite(SIN, HIGH);// Set the SIN bit
    else
      digitalWrite(SIN, LOW);
    camDelay();
    digitalWrite(CLOCK, HIGH);
    camDelay();
    digitalWrite(SIN, LOW);// set the SIN bit low
    camDelay();
  }
  for (bitmask = 128; bitmask >= 1; bitmask >>= 1) {// Write the 8-bits register
    digitalWrite(CLOCK, LOW);
    camDelay();
    if (regval & bitmask)
      digitalWrite(SIN, HIGH);// set the SIN bit
    else
      digitalWrite(SIN, LOW);
      camDelay();
    if (bitmask == 1)
      digitalWrite(LOAD, HIGH);// Assert load at rising edge of CLOCK
    digitalWrite(CLOCK, HIGH);
    camDelay();
    digitalWrite(SIN, LOW);
    camDelay();
  }
}

void camReadPicture()// Take a picture, read it and send it through the serial port.
{
  unsigned int pixel;       // Buffer for pixel read in
  int x, y;
  // Camera START sequence
  digitalWrite(CLOCK, LOW);
  camDelay();// ensure load bit is cleared from previous call
  digitalWrite(LOAD, LOW);
  digitalWrite(START, HIGH);// START rises before CLOCK
  camDelay();
  digitalWrite(CLOCK, HIGH);
  camDelay();
  digitalWrite(START, LOW);// START valid on rising edge of CLOCK, so can drop now
  camDelay();
  digitalWrite(CLOCK, LOW);
  camDelay();
  
  while (1) {// Wait for READ to go high
    digitalWrite(CLOCK, HIGH);
    camDelay();
    if (digitalRead(READ) == HIGH) // READ goes high with rising CLOCK
      break;
    camDelay();  
    digitalWrite(CLOCK, LOW);
    camDelay();  
  }
  camDelay();  

  for (y = 0; y < 128; y++) {
    for (x = 0; x < 128; x++) {
      digitalWrite(CLOCK, LOW);
      camDelay();
      //pixel = analogRead(VOUT) >> 2;// The ADC is 10 bits, this sacrifies the 2 least significant bits to simplify transmission
      pixel = analogReadFast(VOUT) >> 2;
      Serial.write(pixel);
      camDelay();  
      digitalWrite(CLOCK, HIGH);
      camDelay();  
    } // end for x
  } /* for y */

  while (digitalRead(READ) == HIGH) { // Go through the remaining rows
    digitalWrite(CLOCK, LOW);
    camDelay();  
    digitalWrite(CLOCK, HIGH);
    camDelay();  
  }
  digitalWrite(CLOCK, LOW);
  camDelay();  
}
