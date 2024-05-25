//Simplified and completely rewritten by Raphaël BOICHOT in 2023/02/15 to be Arduino IDE compatible
//from a code of Laurent Saint-Marcel (lstmarcel@yahoo.fr) written in 2005/07/05
//once cleaned and updated, the code is in fact damn simple to understand
//the ADC of Arduino is very slow (successive approximations) compared to Game Boy Camera (Flash ADC).
//ADC slowness is the bottleneck of the code, just reading 128x128 pixels and transmitting them to serial takes several seconds
//to perform live image rendering, the use of an external flash ADC is probably mandatory or try using https://github.com/avandalen/avdweb_AnalogReadFast

#include "avdweb_AnalogReadFast.h"
//#define MF64283FP

int VOUT = A3;   //Analog signal from sensor, read shortly after clock is set low, converted to 10 bits by Arduino/ESP ADC then reduced to 8-bits for transmission
int LED = 4;     //just for additionnal LED on D4, not essential to the protocol
int TADD = 7;    //TADD register, for M64283FP only
int STRB = 6;    //Strobe pin, for M64283FP only (UNUSED)
int READ = 8;    //Read image signal, goes high on rising clock
int CLOCK = 9;   //Clock input, pulled down internally, no specification given for frequency
int RESET = 10;  //system reset, pulled down internally, active low, sent on rising edge of clock
int LOAD = 11;   //parameter set enable, pulled down internally, Raise high as you clear the last bit of each register you send
int SIN = 12;    //parameter input, pulled down internally
int START = 13;  //Image sensing start, pulled down internally, has to be high before rising CLOCK

unsigned char v_min = 50;   //minimal voltage returned by the sensor in 8 bits DEC
unsigned char v_max = 180;  //maximal voltage returned by the sensor in 8 bits DEC
unsigned char reg;
unsigned long int accumulator, counter, current_exposure, new_exposure;
unsigned char startX = 0;     //for M64283FP only
unsigned char startY = 0;     //for M64283FP only
unsigned char endX = 16;      //for M64283FP only
unsigned char endY = 16;      //for M64283FP only
unsigned char x_tiles = 16;   //for M64282fp AND M64283FP
unsigned char y_tiles = 16;   //for M64282fp AND M64283FP
unsigned char START_reg = 0;  //for M64283FP only
unsigned char END_reg = 0;    //for M64283FP only

/*
  registers for the M64282FP sensor
  reg0=Z1 Z0 O5 O4 O3 O2 O1 O0 enable calibration with reg O/ O and V add themselves 
  reg1=N VH1 VH0 G4 G3 G2 G1 G0 set edge / type of edge / gain
  reg2=C17 C16 C15 C14 C13 C12 C11 C10 / exposure time by 4096 ms steps (max 1.0486 s)
  reg3=C07 C06 C05 C04 C03 C02 C01 C00 / exposure time by 16 µs steps (max 4096 ms)
  reg4=P7 P6 P5 P4 P3 P2 P1 P0 filtering kernels, always 0x01 on a GB camera, but can be different here
  reg5=M7 M6 M5 M4 M3 M2 M1 M0 filtering kernels, always 0x01 on a GB camera, but can be different here
  reg6=X7 X6 X5 X4 X3 X2 X1 X0 filtering kernels, always 0x01 on a GB camera, but can be different here
  reg7=E3 E2 E1 E0 I V2 V1 V0 Edge enhancement ratio / invert / Output ref voltage (Vref)) V = 0 is not allowed
*/

/*
detail of the registers for the M64283FP
reg0 = like M64282FP
reg1 = like M64282FP
reg2 = like M64282FP
reg3 = like M64282FP
reg4 = SH  AZ  CL  []  [P3  P2  P1  P0]
reg5 = PX  PY  MV4 OB  [M3  M2  M1  M0] //OB low enables outputing black level on line 1 instead of VOUT
reg6 = MV3 MV2 MV1 MV0 [X3  X2  X1  X0]
reg7 = like M64282FP but E register is shifted in value
reg8 = ST7  ST6  ST5  ST4  ST3  ST2  ST1  ST0  - random access start address by (x, y), beware image divided into 8x8 tiles !
reg9 = END7 END6 END5 END4 END3 END2 END1 END0 - random access stop address by (x', y'), beware image divided into 8x8 tiles !
y occupies the 4 MSB, x the 4 LSBs
*/

// typical registers of a Game Boy Camera, goes well also for the 83FP sensor
//////////////////////////{ 0bZZOOOOOO, 0bNVVGGGGG, 0bCCCCCCCC, 0bCCCCCCCC, 0bSAC_PPPP, 0bPPMOMMMM, 0bXXXXXXXX, 0bEEEEIVVV };
unsigned char camReg[8] = { 0b10000000, 0b11100111, 0b00000100, 0b00000000, 0b00000001, 0b00000000, 0b00000001, 0b00000001 };  //registers
  /////////////////////////{ 0byyyyxxxx, 0byyyyxxxx };
  /////////////////////////{ 0bSSSSSSSS, 0bEEEEEEEE };
unsigned char camTADD[2] = { 0b00000000, 0b00000000 };  //for M64283FP only

void setup() {
  Serial.begin(2000000);

#ifdef MF64283FP
  camReg[7] = camReg[7] | 0b01000000;  //use the correct 2D enhancement, black level calibration active by default
  camReg[4] = camReg[4] | 0b00100000;  //the CL register must be set to "H" for random access mode
  //enter here the paremeters to allow random access to the sensor, see documentation
  startX = 5;
  startY = 5;
  endX = 10;
  endY = 10;
  x_tiles = endX - startX;  //tiles centered by default in random access mode for 83FP, must be even
  y_tiles = endY - startY;  //tiles centered by default in random access mode for 83FP, must be even
  START_reg = (startY << 4) | (startX & 0b00001111);
  END_reg = (endY << 4) | (endX & 0b00001111);
  camTADD[0] = START_reg;
  camTADD[1] = END_reg;
#endif

  pinMode(TADD, OUTPUT);  // TADD
  digitalWrite(TADD, HIGH);
  pinMode(STRB, INPUT);    // STRB
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
void loop()  //that's all folks !
{
  Serial.print("x_tiles: ");
  Serial.println(x_tiles, DEC);
  Serial.print("y_tiles: ");
  Serial.println(y_tiles, DEC);

#ifdef MF64283FP
  Serial.print("Random access registers for 83FP sensor:");
  Serial.print(START_reg, HEX);
  Serial.print("-");
  Serial.println(END_reg, HEX);
#endif

  take_a_picture();
  new_exposure = auto_exposure();  // self explanatory
  push_exposure();                 //update exposure registers C2-C3
  Serial.print("Exposure registers: ");
  Serial.println(new_exposure, HEX);
}  //end of loop

int auto_exposure() {
  double exp_regs, new_regs, error, mean_value;
  unsigned int setpoint = (v_max + v_min) / 2;
  exp_regs = camReg[2] * 256 + camReg[3];  // I know, it's a shame to use a double here but we have plenty of ram
  mean_value = accumulator / (x_tiles * 8 * y_tiles * 8);
  error = setpoint - mean_value;  // so in case of deviation, registers 2 and 3 are corrected
  // this part is very similar to what a Game Boy Camera does, except that it does the job with only bitshift operators and in more steps.
  // Here we can use 32 bits variables for ease of programming.
  // the bigger the error is, the bigger the correction on exposure is.
  new_regs = exp_regs;
  if (error > 80) new_regs = exp_regs * 2;
  if (error < -80) new_regs = exp_regs / 2;
  if ((error <= 80) && (error >= 30)) new_regs = exp_regs * 1.3;
  if ((error >= -80) && (error <= -30)) new_regs = exp_regs / 1.3;
  if ((error <= 30) && (error >= 10)) new_regs = exp_regs * 1.03;
  if ((error >= -30) && (error <= -10)) new_regs = exp_regs / 1.03;
  if ((error <= 10) && (error >= 4)) new_regs = exp_regs + 1;    //this level is critical to avoid flickering in full sun, 3-4 is nice
  if ((error >= -10) && (error <= -4)) new_regs = exp_regs - 1;  //this level is critical to avoid flickering in full sun,  3-4 is nice
  return int(new_regs);
}

void push_exposure() {
  if (new_exposure < 0x0010) new_exposure = 0x0010;  //minimum of the sensor for these registers, below there are verticals artifacts, see sensor documentation for details
  if (new_exposure > 0xFFFF) new_exposure = 0xFFFF;  //maximum of the sensor, about 1 second
  camReg[2] = int(new_exposure / 256);               //Janky, I know...
  camReg[3] = int(new_exposure - camReg[2] * 256);   //Janky, I know...
}

unsigned int get_exposure(unsigned char camReg[8]) {
  double exp_regs;
  exp_regs = camReg[2] * 256 + camReg[3];  //
  return exp_regs;
}

void take_a_picture() {
  Serial.println("New image incoming...");
  camReset();         // resets the sensor
  camSetRegisters();  // Send 8 registers to the sensor
#ifdef MF64283FP
  camSetRegistersTADD();
#endif
  camReadPicture();  // get pixels, dump them in RawCamData
  camReset();
  Serial.println("");
}

void camInit()  // Initialise the IO ports for the camera
{
  digitalWrite(CLOCK, LOW);
  digitalWrite(RESET, HIGH);
  digitalWrite(LOAD, LOW);
  digitalWrite(START, LOW);
  digitalWrite(SIN, LOW);
}

void camReset()  // Sends a RESET pulse to sensor
{
  digitalWrite(CLOCK, HIGH);
  digitalWrite(CLOCK, LOW);
  digitalWrite(RESET, LOW);
  digitalWrite(CLOCK, HIGH);
  digitalWrite(RESET, HIGH);
}

void camSetRegisters()  // Sets the sensor 8 registers
{
  for (reg = 0; reg < 8; ++reg) {
    camSetReg(reg, camReg[reg]);
  }
}

void camSetRegistersTADD()  //Sets the sensor 2 ADDitional registers to TADD pin of the M64283FP (must be low)
{
  digitalWrite(TADD, LOW);  //must be low just for these two registers, but must look at the datasheet again
  for (int reg = 0; reg < 2; ++reg) {
    camSetReg(reg + 1, camTADD[reg]);  //adress 0 with TADD LOW does not exist
  }
  digitalWrite(TADD, HIGH);  //back to default state
}

void camSetReg(unsigned char regaddr, unsigned char regval)  // Sets one of the 8 8-bit registers in the sensor
{
  unsigned char bitmask;
  Serial.print("Adress:");
  for (bitmask = 0x4; bitmask >= 0x1; bitmask >>= 1) {  // Write 3-bit address.
    digitalWrite(CLOCK, LOW);
    digitalWrite(LOAD, LOW);  // ensure load bit is cleared from previous call
    if (regaddr & bitmask) {
      digitalWrite(SIN, HIGH);  // Set the SIN bit
      Serial.print("1");
    } else {
      digitalWrite(SIN, LOW);
      Serial.print("0");
    }
    digitalWrite(CLOCK, HIGH);
    digitalWrite(SIN, LOW);  // set the SIN bit low
  }
  Serial.print(" Byte:");
  for (bitmask = 128; bitmask >= 1; bitmask >>= 1) {  // Write the 8-bits register
    digitalWrite(CLOCK, LOW);
    if (regval & bitmask) {
      digitalWrite(SIN, HIGH);  // set the SIN bit
      Serial.print("1");
    } else {
      digitalWrite(SIN, LOW);
      Serial.print("0");
    }
    if (bitmask == 1)
      digitalWrite(LOAD, HIGH);  // Assert load at rising edge of CLOCK
    digitalWrite(CLOCK, HIGH);
    digitalWrite(SIN, LOW);
  }
  Serial.println(" ");
}

void camReadPicture()  // Take a picture, read it and send it through the serial port.
{
  unsigned char pixel;  // Buffer for pixel read in
  int x, y;
  // Camera START sequence
  digitalWrite(CLOCK, LOW);
  digitalWrite(LOAD, LOW);
  digitalWrite(START, HIGH);  // START rises before CLOCK
  digitalWrite(CLOCK, HIGH);
  digitalWrite(START, LOW);  // START valid on rising edge of CLOCK, so can drop now
  digitalWrite(CLOCK, LOW);
  digitalWrite(LED, HIGH);
  while (1) {  // Wait for READ to go high
    digitalWrite(CLOCK, HIGH);
    if (digitalRead(READ) == HIGH)  // READ goes high with rising CLOCK
      break;
    digitalWrite(CLOCK, LOW);
  }
  digitalWrite(LED, LOW);
  accumulator = 0;
  counter = 0;
  for (y = 0; y < (y_tiles * 8); y++) {
    for (x = 0; x < x_tiles * 8; x++) {
      digitalWrite(CLOCK, LOW);
      //pixel = analogRead(VOUT) >> 2;// The ADC is 10 bits, this sacrifies the 2 least significant bits to simplify transmission
      pixel = analogReadFast(VOUT) >> 2;
      //Serial.write(pixel);
      accumulator = accumulator + pixel;  //reject the 8 last lines of pixels that contain junk
      if (pixel > 255) pixel = 255;
      if (pixel < 0) pixel = 0;
      if (pixel <= 0x0F) Serial.print('0');
      Serial.print(pixel, HEX);
      Serial.print(" ");
      digitalWrite(CLOCK, HIGH);
    }  // end for x
  }    /* for y */

  while (digitalRead(READ) == HIGH) {  // Go through the remaining rows
    digitalWrite(CLOCK, LOW);
    digitalWrite(CLOCK, HIGH);
  }
  digitalWrite(CLOCK, LOW);
}
