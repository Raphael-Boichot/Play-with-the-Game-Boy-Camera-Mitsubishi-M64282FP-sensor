//Simplified and completely rewritten by Raphaël BOICHOT in 2022/03/07 to be Arduino/ESP8266/ESP32 compilable 
//from a code of Laurent Saint-Marcel (lstmarcel@yahoo.fr) 2005/07/05
/*
RESET has to be low on the rising edge of CLOCK
Raise LOAD high as you clear the last bit of each register you send
START has to be high before rixing CLOCK
Send START once
The camera won't pulse the START pin; the datasheet is confusing about this
READ goes high on rising CLOCK
Read VOUT analog values shortly after you set CLOCK low
*/

int VOUT = A3; //Analogic-digital converter
int READ = 8;
int CLOCK = 9;
int RESET = 10;
int LOAD = 11;
int SIN = 12;
int START = 13;
int LED = 4;

unsigned char camReg[8] = {155,0,0,0,1,0,1,7}; //default value just for warm-up
unsigned char reg;

//main code//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
  Serial.println("Camera initialised"); //Octave is not happy without spamming the serial initially...
  }
  
}

void loop()//the main loop reads 8 registers from GNU Octave and send an image to serial
{

  Serial.println("Ready for injecting registers");
  if (Serial.available() > 0) {
    digitalWrite(LED, HIGH);
    for (reg = 0; reg < 8; ++reg) {
    camReg[reg] = Serial.read(); //read the 8 current registers sent by GNU Octave
    }
    delay(100);
    digitalWrite(LED, LOW);

    Serial.print("Registers injected in sensor by GNU Octave for verification: ");
    for (reg = 0; reg < 8; ++reg) {
      Serial.print(camReg[reg], DEC);//sent them back to GNU Octave for acknowledgment
      Serial.print(" ");
    }
    Serial.println("");

    camReset();
    camSetRegisters();// Send 8 registers to the sensor
    camReadPicture(); // get pixels, GNU Octave reads them on serial and does the rest of the job
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
delayMicroseconds(1);
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
  // Write 3-bit address.
  for (bitmask = 0x4; bitmask >= 0x1; bitmask >>= 1) {
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

  // Write 7 most significant bits of 8-bit data 
  //(I do not understand why, the command is 8 bits... /BOICHOT)
  for (bitmask = 128; bitmask >= 1; bitmask >>= 1) {
    digitalWrite(CLOCK, LOW);
    delayMicroseconds(1);
    if (regval & bitmask)
      digitalWrite(SIN, HIGH);// set the SIN bit
    else
      digitalWrite(SIN, LOW);
      delayMicroseconds(1);
    if (bitmask == 1)
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

  // Wait for READ to go high
  while (1) {
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
