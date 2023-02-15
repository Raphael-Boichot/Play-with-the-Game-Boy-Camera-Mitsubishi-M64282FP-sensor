//Simplified and completely rewritten by Raphaël BOICHOT in 2023/02/15 to be Arduino
//from a code of Laurent Saint-Marcel (lstmarcel@yahoo.fr) written in 2005/07/05
//once cleaned and updated, the code is in fact damn simple to understand
//the ADC of Arduino is very slow (successive approximations) compared to Game Boy Camera (Flash ADC).
//ADC slowness is the bottleneck of the code, just reading 128x128 pixels and transmitting them to serial takes several seconds
//to perform live image rendering, the use of an external flash ADC is probably mandatory or try using https://github.com/avandalen/avdweb_AnalogReadFast

#include "avdweb_AnalogReadFast.h"

int VOUT =  A3; //Analog signal from sensor, read shortly after clock is set low, converted to 10 bits by Arduino/ESP ADC then reduced to 8-bits for transmission
int READ =  8; //Read image signal, goes high on rising clock
int CLOCK = 9; //Clock input, pulled down internally, no specification given for frequency
int RESET = 10; //system reset, pulled down internally, active low, sent on rising edge of clock
int LOAD =  11; //parameter set enable, pulled down internally, Raise high as you clear the last bit of each register you send
int SIN =   12; //parameter input, pulled down internally
int START = 13; //Image sensing start, pulled down internally, has to be high before rising CLOCK
int LED =   4; //just for additionnal LED on D4, not essential to the protocol

/*
  %some real settings used by the Mitsubishi sensor on Game Boy Camera, except exposure
  reg0=0b10011111;%Z1 Z0 O5 O4 O3 O2 O1 O0 zero point calibration and output reference voltage
  reg1=0b11101000;%N VH1 VH0 G4 G3 G2 G1 G0
  reg2=0b00000001;%C17 C16 C15 C14 C13 C12 C11 C10 / exposure time by 4096 ms steps (max 1.0486 s)
  reg3=0b00000000;%C07 C06 C05 C04 C03 C02 C01 C00 / exposure time by 16 µs steps (max 4096 ms)
  reg4=0b00000001;%P7 P6 P5 P4 P3 P2 P1 P0 filtering kernels
  reg5=0b00000000;%M7 M6 M5 M4 M3 M2 M1 M0 filtering kernels
  reg6=0b00000001;%X7 X6 X5 X4 X3 X2 X1 X0 filtering kernels
  reg7=0b00000011;%E3 E2 E1 E0 I V2 V1 V0 set Vref
*/
// typical registers of a Game Boy Camera
unsigned char camReg[8] = {0b10011111, 0b11101000, 0b00000001, 0b00000000, 0b00000001, 0b000000000, 0b00000001, 0b00000011}; //registers
unsigned char v_min = 27; //minimal voltage returned by the sensor in 8 bits DEC (0.58 volts)
unsigned char v_max = 155;//maximal voltage returned by the sensor in 8 bits DEC (3.04 volts)
unsigned char reg;
unsigned long int accumulator, counter, current_exposure, new_exposure;

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
}

// there is no special command to trigger sensor, as soon as 8 registers are received, an image is requested to the sensor
void loop()//that's all folks !
{
  take_a_picture();
  new_exposure = auto_exposure();// self explanatory
  push_exposure(); //update exposure registers C2-C3
  Serial.print("Exposure registers: ");
  Serial.println(new_exposure, HEX);
} //end of loop

int auto_exposure() {
  double exp_regs, new_regs, error, mean_value;
  unsigned int setpoint = (v_max + v_min) / 2;
  unsigned char max_line = 120; //last 5-6 rows of pixels contains dark pixel value and various artifacts, so I remove 8 to have a full tile line
  exp_regs = camReg[2] * 256 + camReg[3];// I know, it's a shame to use a double here but we have plenty of ram
  mean_value = accumulator / (128 * max_line);
  error = setpoint - mean_value; // so in case of deviation, registers 2 and 3 are corrected
  // this part is very similar to what a Game Boy Camera does, except that it does the job with only bitshift operators and in more steps.
  // Here we can use 32 bits variables for ease of programming.
  // the bigger the error is, the bigger the correction on exposure is.
  new_regs = exp_regs;
  if (error > 80)                     new_regs = exp_regs * 2;
  if (error < -80)                    new_regs = exp_regs / 2;
  if ((error <= 80) && (error >= 30))    new_regs = exp_regs * 1.3;
  if ((error >= -80) && (error <= -30))  new_regs = exp_regs / 1.3;
  if ((error <= 30) && (error >= 10))     new_regs = exp_regs * 1.03;
  if ((error >= -30) && (error <= -10))   new_regs = exp_regs / 1.03;
  if ((error <= 10) && (error >= 4))   new_regs = exp_regs + 1;//this level is critical to avoid flickering in full sun, 3-4 is nice
  if ((error >= -10) && (error <= -4))  new_regs = exp_regs - 1;//this level is critical to avoid flickering in full sun,  3-4 is nice
  return int(new_regs);
}

void push_exposure() {
  if (new_exposure < 0x0030) new_exposure = 0x0030;//minimum of the sensor for these registers, below there are verticals artifacts, see sensor documentation for details
  if (new_exposure > 0xFFFF) new_exposure = 0xFFFF;//maximum of the sensor, about 1 second
  camReg[2] = int(new_exposure / 256);//Janky, I know...
  camReg[3] = int(new_exposure - camReg[2] * 256);//Janky, I know...
}

unsigned int get_exposure(unsigned char camReg[8]) {
  double exp_regs;
  exp_regs = camReg[2] * 256 + camReg[3];//
  return exp_regs;
}

void take_a_picture() {
  Serial.println("New image:");
  camReset();// resets the sensor
  camSetRegisters();// Send 8 registers to the sensor
  camReadPicture(); // get pixels, dump them in RawCamData
  camReset();
  Serial.println("");
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
  digitalWrite(CLOCK, LOW);
  digitalWrite(RESET, LOW);
  digitalWrite(CLOCK, HIGH);
  digitalWrite(RESET, HIGH);
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
    digitalWrite(LOAD, LOW);// ensure load bit is cleared from previous call
    if (regaddr & bitmask)
      digitalWrite(SIN, HIGH);// Set the SIN bit
    else
      digitalWrite(SIN, LOW);
    digitalWrite(CLOCK, HIGH);
    digitalWrite(SIN, LOW);// set the SIN bit low
  }
  for (bitmask = 128; bitmask >= 1; bitmask >>= 1) {// Write the 8-bits register
    digitalWrite(CLOCK, LOW);
    if (regval & bitmask)
      digitalWrite(SIN, HIGH);// set the SIN bit
    else
      digitalWrite(SIN, LOW);
    if (bitmask == 1)
      digitalWrite(LOAD, HIGH);// Assert load at rising edge of CLOCK
    digitalWrite(CLOCK, HIGH);
    digitalWrite(SIN, LOW);
  }
}

void camReadPicture()// Take a picture, read it and send it through the serial port.
{
  unsigned int pixel;       // Buffer for pixel read in
  int x, y;
  // Camera START sequence
  digitalWrite(CLOCK, LOW);
  digitalWrite(LOAD, LOW);
  digitalWrite(START, HIGH);// START rises before CLOCK
  digitalWrite(CLOCK, HIGH);
  digitalWrite(START, LOW);// START valid on rising edge of CLOCK, so can drop now
  digitalWrite(CLOCK, LOW);
  digitalWrite(LED, HIGH);
  while (1) {// Wait for READ to go high
    digitalWrite(CLOCK, HIGH);
    if (digitalRead(READ) == HIGH) // READ goes high with rising CLOCK
      break;
    digitalWrite(CLOCK, LOW);
  }
  digitalWrite(LED, LOW);
  accumulator = 0;
  counter = 0;
  for (y = 0; y < 128; y++) {
    for (x = 0; x < 128; x++) {
      digitalWrite(CLOCK, LOW);
      //pixel = analogRead(VOUT) >> 2;// The ADC is 10 bits, this sacrifies the 2 least significant bits to simplify transmission
      pixel = analogReadFast(VOUT) >> 2;
      //Serial.write(pixel);
      accumulator = accumulator + pixel;
      if (pixel <= 0x0F) Serial.print('0');
      Serial.print(pixel, HEX);
      Serial.print(" ");
      digitalWrite(CLOCK, HIGH);
    } // end for x
  } /* for y */

  while (digitalRead(READ) == HIGH) { // Go through the remaining rows
    digitalWrite(CLOCK, LOW);
    digitalWrite(CLOCK, HIGH);
  }
  digitalWrite(CLOCK, LOW);
}
