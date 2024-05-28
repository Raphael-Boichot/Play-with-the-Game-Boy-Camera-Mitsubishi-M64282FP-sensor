//Simplified and completely rewritten by Raphaël BOICHOT in 2023/02/15 to be Arduino IDE compatible
//from a code of Laurent Saint-Marcel (lstmarcel@yahoo.fr) written in 2005/07/05
//once cleaned and updated, the code is in fact damn simple to understand
//the ADC of Arduino is very slow (successive approximations) compared to Game Boy Camera (Flash ADC).
//ADC slowness is the bottleneck of the code, just reading 128x128 pixels and transmitting them to serial takes several seconds
//to perform live image rendering, the use of an external flash ADC is probably mandatory or try using https://github.com/avandalen/avdweb_AnalogReadFast

#include "avdweb_AnalogReadFast.h"
#define NOP __asm__ __volatile__("nop\n\t")  //minimal possible delay
//#define MF64283FP                            //forces the M64283FP mode, if inactivated, forces M64282FP mode
//I've only programmed the random access mode because it's fun but the projection mode is just a matter of changing few variables

int VOUT = A3;   //Analog signal from sensor, read shortly after clock is set low, converted to 10 bits by Arduino/ESP ADC then reduced to 8-bits for transmission
int LED = 4;     //just for additionnal LED on D4, not essential to the protocol
int RED = 5;     //to show access time to ADC
int STRB = 6;    //Strobe pin, for M64283FP only (UNUSED here as it follows CLOCK but why not using it)
int TADD = 7;    //TADD pin, for M64283FP only
int READ = 8;    //Read image signal, goes high on rising clock
int CLOCK = 9;   //Clock input, pulled down internally, no specification given for frequency
int RESET = 10;  //system reset, pulled down internally, active low, sent on rising edge of clock
int LOAD = 11;   //parameter set enable, pulled down internally, Raise high as you clear the last bit of each register you send
int SIN = 12;    //parameter input, pulled down internally
int START = 13;  //Image sensing start, pulled down internally, has to be high before rising CLOCK

//enter here the parameters to allow random access to the sensor, see documentation
unsigned char startX_random_access = 6;  //for M64283FP only
unsigned char startY_random_access = 0;  //for M64283FP only
unsigned char endX_random_access = 10;   //for M64283FP only
unsigned char endY_random_access = 16;   //for M64283FP only
//the voltage range of VOUT is approx. (V + O) to (V + O + 2 volts) but it is not an exact science
//targeting the mid range for autoexposure gives good results overall
unsigned char v_min = 50;   //minimal voltage returned by the sensor in 8 bits DEC, about V+O, so 0.5V (or 26 in 8-bit on 0-5V ADC scale)
unsigned char v_max = 150;  //maximal voltage returned by the sensor in 8 bits DEC, about V+O+2, so 2.5V (or 180 in 8-bit on 0-5V ADC scale)
//beware if you port this code to ESP32 or Pi Pico : the voltage scale of the ADC is 0-3.3V only, it's easy to fry the ADC channel !
unsigned char x_tiles = 16;               //for M64282FP
unsigned char y_tiles = 16;               //for M64282FP
unsigned char cycles = 0;                 //usefull for ESP32, Pi Pico, etc. Adds a delay in machine cycle to slower the Clock rate
// the good way to assess a good timing is to check if the exposure time when exposure register is set to 0xFFFF (in the dark) is close to 1048 ms (must be a bit higher, never less)
// this ensure that the clock rate is approximately 500 kHz (can be slower, never faster). On Arduino Uno, no delay is necessary anyway

// typical registers of a Game Boy Camera, goes well also for the 83FP sensor when not using extra registers
//////////////////////////{ 0bZZOOOOOO, 0bNVVGGGGG, 0bCCCCCCCC, 0bCCCCCCCC, 0bSAC_PPPP, 0bPPMOMMMM, 0bXXXXXXXX, 0bEEEEIVVV };
unsigned char camReg[8] = { 0b10000000, 0b11100111, 0b00000100, 0b00000000, 0b00000001, 0b00000000, 0b00000001, 0b00000001 };  //registers
  /////////////////////////{ 0byyyyxxxx, 0byyyyxxxx };
  /////////////////////////{ 0bSSSSSSSS, 0bEEEEEEEE };
unsigned char camTADD[2] = { 0b00000000, 0b00000000 };  //for M64283FP only

/*
  registers for the M64282FP sensor
  reg0=Z1 Z0 O5 O4 O3 O2 O1 O0 enable calibration with reg O (O and V add themselves only if V not 0)
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
  reg4 = SH  AZ  CL  []  [P3  P2  P1  P0] //CL and OB low activates the clamp circuit for autocalibration
  reg5 = PX  PY  MV4 OB  [M3  M2  M1  M0] //OB low enables outputing black level on line 1 instead of VOUT
  reg6 = MV3 MV2 MV1 MV0 [X3  X2  X1  X0]
  reg7 = like M64282FP but E register is shifted in value
  reg8 = ST7  ST6  ST5  ST4  ST3  ST2  ST1  ST0  - random access start address by (x, y), beware image divided into 8x8 tiles !
  reg9 = END7 END6 END5 END4 END3 END2 END1 END0 - random access stop address by (x', y'), beware image divided into 8x8 tiles !
  y occupies the 4 MSB, x the 4 LSBs
*/

// some other variables
unsigned char reg;
unsigned long int accumulator, counter, current_exposure, new_exposure;
unsigned long currentTime, previousTime;  //time counter for exposure
unsigned char startX;                     //for M64283FP only
unsigned char startY;                     //for M64283FP only
unsigned char endX;                       //for M64283FP only
unsigned char endY;                       //for M64283FP only
unsigned char START_reg;                  //for M64283FP only
unsigned char END_reg;                    //for M64283FP only

void setup() {
  Serial.begin(2000000);

//setting to trigger the random access mode
#ifdef MF64283FP                                                                  //random access mode
  camReg[1] = camReg[1] & 0b00011111;                                             //N, VH1 and VH0 must be LOW for random access mode
  camReg[4] = camReg[4] | 0b00100000;                                             //the CL register must be HIGH for random access mode
  camReg[5] = camReg[5] | 0b00010000;                                             //the OB register must be HIGH for random access mode
  camReg[7] = camReg[7] & 0b00001111;                                             //all E registers must be LOW for random access mode
  x_tiles = endX_random_access - startX_random_access;                            // random access range for 83FP, just used for Matlab through serial
  y_tiles = endY_random_access - startY_random_access;                            // random access range for 83FP, just used for Matlab through serial
  START_reg = (startY_random_access << 4) | (startX_random_access & 0b00001111);  //y, 4 MSB, x 4 LSB
  END_reg = (endY_random_access << 4) | (endX_random_access & 0b00001111);        //y, 4 MSB, x 4 LSB
  camTADD[0] = START_reg;
  camTADD[1] = END_reg;
#endif
  pinMode(TADD, OUTPUT);     // TADD
  digitalWrite(TADD, HIGH);  // default state apart when sending extra registers
  pinMode(STRB, INPUT);      // STRB
  pinMode(READ, INPUT);      // READ
  pinMode(CLOCK, OUTPUT);    // CLOCK
  pinMode(RESET, OUTPUT);    // RESET
  pinMode(LOAD, OUTPUT);     // LOAD
  pinMode(SIN, OUTPUT);      // SIN
  pinMode(START, OUTPUT);    // START
  pinMode(LED, OUTPUT);      // LED
  pinMode(RED, OUTPUT);      // LED
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
  Serial.print("Exposure registers:   ");
  Serial.println(new_exposure, HEX);
}  //end of loop

int auto_exposure() {
  double exp_regs, new_regs, error, mean_value;
  unsigned int setpoint = (v_max + v_min) / 2; //we target a mid "voltage"
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
  camReadPicture();  // get pixels, dump them immediately as no ram available
  camReset();
  Serial.println("");
}

void camDelay()  //Allow a lag in processor cycles to maintain signals long enough, critical for exposure time, sensor must be clocked at 1 MHz MAXIMUM (can be less, see nigth mode)
{
  for (int i = 0; i < cycles; i++) NOP;
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
  camDelay();
  digitalWrite(CLOCK, LOW);
  camDelay();
  digitalWrite(RESET, LOW);
  digitalWrite(CLOCK, HIGH);
  camDelay();
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
  for (int reg = 0; reg < 2; ++reg) {
    camSetRegTADD(reg + 1, camTADD[reg]);  //adress 0 with TADD LOW does not exist
  }
}

void camSetReg(unsigned char regaddr, unsigned char regval)  // Sets one of the 8 8-bit registers in the sensor
{
  unsigned char bitmask;
  Serial.print("Address:");
  for (bitmask = 0x4; bitmask >= 0x1; bitmask >>= 1) {  // Write 3-bit address.
    digitalWrite(CLOCK, LOW);
    camDelay();
    digitalWrite(LOAD, LOW);  // ensure load bit is cleared from previous call
    if (regaddr & bitmask) {
      digitalWrite(SIN, HIGH);  // Set the SIN bit
      Serial.print("1");
    } else {
      digitalWrite(SIN, LOW);
      Serial.print("0");
    }
    digitalWrite(CLOCK, HIGH);
    camDelay();
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
    if (bitmask == 1) {
      digitalWrite(LOAD, HIGH);  // Assert load at rising edge of CLOCK
      Serial.print(" TADD:1");
    }
    digitalWrite(CLOCK, HIGH);
    camDelay();
    digitalWrite(SIN, LOW);
    digitalWrite(CLOCK, LOW);
    camDelay();
    digitalWrite(LOAD, LOW);  // come back to normal state
  }
  Serial.println(" ");
}

void camSetRegTADD(unsigned char regaddr, unsigned char regval)  // Sets one of the 8 8-bit registers in the sensor
{
  unsigned char bitmask;
  Serial.print("Adress:");
  for (bitmask = 0x4; bitmask >= 0x1; bitmask >>= 1) {  // Write 3-bit address.
    digitalWrite(CLOCK, LOW);
    camDelay();
    digitalWrite(LOAD, LOW);  // ensure load bit is cleared from previous call
    if (regaddr & bitmask) {
      digitalWrite(SIN, HIGH);  // Set the SIN bit
      Serial.print("1");
    } else {
      digitalWrite(SIN, LOW);
      Serial.print("0");
    }
    digitalWrite(CLOCK, HIGH);
    camDelay();
    digitalWrite(SIN, LOW);  // set the SIN bit low
  }
  Serial.print(" Byte:");
  for (bitmask = 128; bitmask >= 1; bitmask >>= 1) {  // Write the 8-bits register
    digitalWrite(CLOCK, LOW);
    camDelay();
    if (regval & bitmask) {
      digitalWrite(SIN, HIGH);  // set the SIN bit
      Serial.print("1");
    } else {
      digitalWrite(SIN, LOW);
      Serial.print("0");
    }
    if (bitmask == 1) {
      digitalWrite(TADD, LOW);   // Assert TADD at rising edge of CLOCK
      digitalWrite(LOAD, HIGH);  // Assert LOAD at rising edge of CLOCK
      Serial.print(" TADD:0");
    }
    digitalWrite(CLOCK, HIGH);
    camDelay();
    digitalWrite(SIN, LOW);
    digitalWrite(CLOCK, LOW);
    camDelay();
    digitalWrite(LOAD, LOW);   // come back to normal state
    digitalWrite(TADD, HIGH);  // come back to normal state
  }
  Serial.println(" ");
}

void camReadPicture()  // Take a picture, read it and send it through the serial port.
{
  unsigned char pixel;  // Buffer for pixel read in
  int x, y;
  // Camera START sequence
  digitalWrite(CLOCK, LOW);
  camDelay();
  digitalWrite(LOAD, LOW);
  digitalWrite(START, HIGH);  // START rises before CLOCK
  digitalWrite(CLOCK, HIGH);
  camDelay();
  digitalWrite(START, LOW);  // START valid on rising edge of CLOCK, so can drop now
  digitalWrite(CLOCK, LOW);
  camDelay();
  digitalWrite(LED, HIGH);
  previousTime = millis();  //starts a timer
  while (1) {               // Wait for READ to go high
    digitalWrite(CLOCK, HIGH);
    camDelay();
    if (digitalRead(READ) == HIGH)  // READ goes high with rising CLOCK
      break;
    digitalWrite(CLOCK, LOW);
    camDelay();
  }
  currentTime = millis() - previousTime;  //to dislay the real exposure time in ms
  digitalWrite(LED, LOW);
  accumulator = 0;
  counter = 0;
  digitalWrite(RED, HIGH);
  for (y = 0; y < (y_tiles * 8); y++) {
    for (x = 0; x < x_tiles * 8; x++) {
      digitalWrite(CLOCK, LOW);
      camDelay();
      //pixel = analogRead(VOUT) >> 2;// The ADC is 10 bits, this sacrifies the 2 least significant bits to simplify transmission
      pixel = analogReadFast(VOUT) >> 2;
      //Serial.write(pixel);
      accumulator = accumulator + pixel;  //for autoexposure
      if (pixel > 255) pixel = 255;
      if (pixel < 0) pixel = 0;
      //output in ASCII as output as raw data makes the serial crazy due to unsupported characters
      //it's longer so...
      //as I have virtually no memory (2kB and I would need 16 kB to store an image), I cannot store data to process them further
      if (pixel <= 0x0F) Serial.print('0');
      Serial.print(pixel, HEX);
      Serial.print(" ");
      digitalWrite(CLOCK, HIGH);
    }  // end for x
  }    /* for y */
  digitalWrite(RED, LOW);
  while (digitalRead(READ) == HIGH) {  // Go through the remaining rows
    digitalWrite(CLOCK, LOW);
    camDelay();
    digitalWrite(CLOCK, HIGH);
    camDelay();
  }
  digitalWrite(CLOCK, LOW);
  camDelay();
  Serial.println(" ");
  Serial.print("Current exposure time (ms):");
  Serial.println(currentTime, DEC);
}
