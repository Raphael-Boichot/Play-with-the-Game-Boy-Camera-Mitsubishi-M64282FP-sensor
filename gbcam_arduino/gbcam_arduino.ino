//Simplified and completely rewritten by Raphaël BOICHOT in 2022/03/07 to be Arduino/ESP8266/ESP32 compatible
//from a code of Laurent Saint-Marcel (lstmarcel@yahoo.fr) written in 2005/07/05
//once cleaned and updated, the code is in fact damn simple to understand contrary to the original code

int VOUT = A3; //Analog signal from sensor, read shortly after clock is set low, converted to 10 bits by Arduino/ESP then reduced to 8-bits for transmission
int READ = 8; //Read image signal, goes high on rising clock
int CLOCK = 9; //Clock input, pulled down internally, no specification given for frequency
int RESET = 10; //system reset, pulled down internally, active low, sent on rising edge of clock
int LOAD = 11; //parameter set enable, pulled down internally, Raise high as you clear the last bit of each register you send
int SIN = 12; //parameter input, pulled down internally
int START = 13; //Image sensing start, pulled down internally, has to be high before rising CLOCK
int LED = 4; //just for additionnal LED on D4, not essential to the protocol

unsigned char camReg[8] = {155,0,0,0,1,0,1,7}; //default value, can be used for debug, is erased anyway by Octave
unsigned char reg;

void setup()
{
  Serial.begin(115200);
  Serial.println("Camera initialisation");
  pinMode(READ, INPUT);   // READ
  pinMode(CLOCK, OUTPUT);  // CLOCK
  pinMode(RESET, OUTPUT);  // RESET
  pinMode(LOAD, OUTPUT); // LOAD
  pinMode(SIN, OUTPUT); // SIN
  pinMode(START, OUTPUT); // START
  pinMode(LED, OUTPUT);   // LED
  camInit();
  for (int i = 0; i < 100; i++) {
  Serial.println("Camera initialised"); //Octave is not happy without being spammed initially, may be removed in portable device
  }
}

// there is no special command to trigger sensor, as soon as 8 registers are received, an image is requested to the sensor
void loop()//the main loop just reads 8 registers from GNU Octave and send an image to serial in 8-bits binary
{
  Serial.println("Ready for injecting registers"); //Go for sending registers on GNU Octave side
  if (Serial.available() > 0) {// do I receive a byte ?
    digitalWrite(LED, HIGH);
    for (reg = 0; reg < 8; ++reg) {
    camReg[reg] = Serial.read(); //read the 8 current registers sent by GNU Octave
    }
    delay(100);//the delay here is just to see the LED flashing without slowing down the protocol, can be removed if necessary
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
    Serial.println("sensor reset");
}

} //end of main code//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// PRIVATE FUNCTIONS

// Initialise the IO ports for the camera
void camInit()
{
digitalWrite(CLOCK, LOW);
digitalWrite(RESET, HIGH);
digitalWrite(LOAD, LOW);
digitalWrite(START, LOW);
digitalWrite(SIN, LOW);
}

// Sends a 'reset' pulse to the AR chip.
// START:  CLOCK Rising Edge
// FINISH: CLOCK Just before Falling Edge
void camReset()
{
digitalWrite(CLOCK, HIGH);
delayMicroseconds(1);//I think all delays may be removed, to check with you own board
digitalWrite(CLOCK, LOW);
delayMicroseconds(1);
digitalWrite(RESET, LOW);
delayMicroseconds(1);
digitalWrite(CLOCK, HIGH);
delayMicroseconds(1);
digitalWrite(RESET, HIGH);
delayMicroseconds(1);
}

// Reset the camera and set the camera's 8 registers
// from the locally stored values 
void camSetRegisters(void)
{
  for (reg = 0; reg < 8; ++reg) {
    camSetReg(reg, camReg[reg]);
  }
}

// Sets one of the 8 8-bit registers in the AR chip.
// START:  CLOCK Falling Edge
// FINISH: CLOCK Just before Falling Edge
void camSetReg(unsigned char regaddr, unsigned char regval)
{
  unsigned char bitmask;
  for (bitmask = 0x4; bitmask >= 0x1; bitmask >>= 1) {// Write 3-bit address.
    digitalWrite(CLOCK, LOW);
    delayMicroseconds(1);
    digitalWrite(LOAD, LOW);// ensure load bit is cleared from previous call
    if (regaddr & bitmask)
      digitalWrite(SIN, HIGH);// Set the SIN bit
    else
      digitalWrite(SIN, LOW);
    delayMicroseconds(1);
    digitalWrite(CLOCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(SIN, LOW);// set the SIN bit low
    delayMicroseconds(1);
  }

  for (bitmask = 128; bitmask >= 1; bitmask >>= 1) {// Write 7 most significant bits of 8-bit data 
    digitalWrite(CLOCK, LOW);
    delayMicroseconds(1);
    if (regval & bitmask)
      digitalWrite(SIN, HIGH);// set the SIN bit
    else
      digitalWrite(SIN, LOW);
      delayMicroseconds(1);
    if (bitmask == 1)//Write at the end the least significant bit, I do not see the interest of not sending everything at once
      digitalWrite(LOAD, HIGH);// Assert load at rising edge of xck
    digitalWrite(CLOCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(SIN, LOW);
    delayMicroseconds(1);
  }
    digitalWrite(LED, HIGH);
    delay(10);
    digitalWrite(LED, LOW);
}

// Take a picture, read it and send it through the serial port.
// START:  CLOCK Falling Edge
// FINISH: CLOCK Just before Rising Edge
void camReadPicture()
{
  unsigned int pixel;       // Buffer for pixel read in
  int x, y;

  // Camera START sequence
  digitalWrite(CLOCK, LOW);
  delayMicroseconds(1);// ensure load bit is cleared from previous call
  digitalWrite(LOAD, LOW);// START rises before CLOCK
  digitalWrite(START, HIGH);
  delayMicroseconds(1);
  digitalWrite(CLOCK, HIGH);
  delayMicroseconds(1);
  digitalWrite(START, LOW);// START valid on rising edge of CLOCK, so can drop now
  delayMicroseconds(1);
  digitalWrite(CLOCK, LOW);
  delayMicroseconds(1);
  
  while (1) {// Wait for READ to go high
    digitalWrite(CLOCK, HIGH);
    delayMicroseconds(1);
    if (digitalRead(READ) == HIGH) // READ goes high with rising CLOCK
      break;
    delayMicroseconds(1);  
    digitalWrite(CLOCK, LOW);
    delayMicroseconds(1);  
  }
  delayMicroseconds(1);  

  for (y = 0; y < 128; y++) {
    for (x = 0; x < 128; x++) {
      digitalWrite(CLOCK, LOW);
      delayMicroseconds(1);
      pixel = analogRead(VOUT) >> 2;// The ADC is 10 bits, this sacrifies the 2 least significant bits to simplify transmission
      Serial.write(pixel);
      delayMicroseconds(1);  
      digitalWrite(CLOCK, HIGH);
      delayMicroseconds(1);  
    } // end for x
  } /* for y */

  while (digitalRead(READ) == HIGH) { // Go through the remaining rows
    digitalWrite(CLOCK, LOW);
    delayMicroseconds(1);  
    digitalWrite(CLOCK, HIGH);
    delayMicroseconds(1);  
  }
  digitalWrite(CLOCK, LOW);
  delayMicroseconds(1);  
}
