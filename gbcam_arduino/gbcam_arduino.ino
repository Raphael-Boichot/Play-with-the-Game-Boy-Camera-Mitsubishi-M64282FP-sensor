//Laurent Saint-Marcel (lstmarcel@yahoo.fr) 2005/07/05
//Simplified and cleaned by Raphaël BOICHOT 2020/03/04
/*
RESET/XRST has to be low on the rising edge of XCK
Raise LOAD high as you clear the last bit of each register you send
START has to be high before rixing XCK
Send START once
The camera won't pulse the START pin; the datasheet is confusing about this
READ goes high on rising XCK
Read VOUT analog values shortly after you set XCK low
*/
#include <compat/deprecated.h>

#define CAM_DATA_PORT     PORTB
#define CAM_DATA_DDR      DDRB
#define CAM_READ_PIN      8       // Arduino D8
#define CAM_LED_DDR       DDRB
#define CAM_LED_PORT      PORTB

// CAM_DATA_PORT
#define CAM_START_BIT 5
#define CAM_SIN_BIT   4
#define CAM_LOAD_BIT  3
#define CAM_RESET_BIT 2
#define CAM_CLOCK_BIT 1
#define CAM_READ_BIT  0

// PORT C:
#define CAM_ADC_PIN   A3 //Analogic/digital converter

unsigned char camReg[8] = {155,0,0,0,1,0,1,7}; //default value for warm-up
unsigned char camClockSpeed = 0x07; // was 0x0A
boolean dataReady;
unsigned char reg;

//main code//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  dataReady = false;
  Serial.begin(115200);
  Serial.println("Camera initialisation");
  pinMode(8, INPUT);   // READ
  pinMode(9, OUTPUT);  // XCK
  pinMode(10, OUTPUT);  // XRST
  pinMode(11, OUTPUT); // LOAD
  pinMode(12, OUTPUT); // SIN
  pinMode(13, OUTPUT); // START
  pinMode(4, OUTPUT);   // LED
  camInit();
  sei();// enable interrupts
}

void loop()//the main loop reads 8 registers from GNU Octave and send an image to serial
{

  Serial.println("Ready for injecting registers");
  if (Serial.available() > 0) {
    digitalWrite(4, HIGH);
    for (reg = 0; reg < 8; ++reg) {
    camReg[reg] = Serial.read(); //read the 8 current registers sent by GNU Octave
    }
    digitalWrite(4, LOW);

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

} //main code//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// PRIVATE FUNCTIONS

// CAM TIMING AND CONTROL
// cbi(port, bitId) = clear bit(port, bitId) = Set the signal Low
// sbi(port, bitId) = set bit(port, bitId) = Set the signal High
// Delay used between each signal sent to the AR (four per xck cycle).
void camStepDelay() {
  unsigned char u = camClockSpeed;
  while (u--) {
    __asm__ __volatile__ ("nop");
  }
}
// Set the clock signal Low
inline void camClockL()
{
  cbi(CAM_DATA_PORT, CAM_CLOCK_BIT);
}
// Set the clock signal High
inline void camClockH()
{
  sbi(CAM_DATA_PORT, CAM_CLOCK_BIT);
}

// Initialise the IO ports for the camera
void camInit()
{
  cbi(CAM_DATA_PORT, CAM_CLOCK_BIT);
  sbi(CAM_DATA_PORT, CAM_RESET_BIT);  // Reset is active low
  cbi(CAM_DATA_PORT, CAM_LOAD_BIT);
  cbi(CAM_DATA_PORT, CAM_START_BIT);
  cbi(CAM_DATA_PORT, CAM_SIN_BIT);
}

// Sends a 'reset' pulse to the AR chip.
// START:  XCK Rising Edge
// FINISH: XCK Just before Falling Edge
void camReset()
{
  camClockH(); // clock high
  camStepDelay();
  camStepDelay();

  camClockL(); // clock low
  camStepDelay();
  cbi(CAM_DATA_PORT, CAM_RESET_BIT);
  camStepDelay();

  camClockH(); // clock high
  camStepDelay();
  sbi(CAM_DATA_PORT, CAM_RESET_BIT);
  camStepDelay();
}

// locally set the value of a register but do not set it in the AR chip. You
// must run camSendRegisters1 to write the register value in the chip
void camStoreReg(unsigned char reg, unsigned char data)
{
  camReg[reg] = data;
}

// Reset the camera and set the camera's 8 registers
// from the locally stored values (see camStoreReg)
void camSetRegisters(void)
{
  for (reg = 0; reg < 8; ++reg) {
    camSetReg(reg, camReg[reg]);
  }
}

// Sets one of the 8 8-bit registers in the AR chip.
// START:  XCK Falling Edge
// FINISH: XCK Just before Falling Edge
void camSetReg(unsigned char regaddr, unsigned char regval)
{
  unsigned char bitmask;

  // Write 3-bit address.
  for (bitmask = 0x4; bitmask >= 0x1; bitmask >>= 1) {
    camClockL();
    camStepDelay();
    cbi(CAM_DATA_PORT, CAM_LOAD_BIT);// ensure load bit is cleared from previous call
    if (regaddr & bitmask)
      sbi(CAM_DATA_PORT, CAM_SIN_BIT);// Set the SIN bit
    else
      cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();
    camClockH();
    camStepDelay();
    cbi(CAM_DATA_PORT, CAM_SIN_BIT);// set the SIN bit low
    camStepDelay();
  }

  // Write 7 most significant bits of 8-bit data.
  for (bitmask = 128; bitmask >= 1; bitmask >>= 1) {
    camClockL();
    camStepDelay();
    if (regval & bitmask)
      sbi(CAM_DATA_PORT, CAM_SIN_BIT);// set the SIN bit
    else
      cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();
    if (bitmask == 1)
      sbi(CAM_DATA_PORT, CAM_LOAD_BIT);// Assert load at rising edge of xck
    camClockH();
    camStepDelay();
    cbi(CAM_DATA_PORT, CAM_SIN_BIT);// reset the SIN bit
    camStepDelay();
  }
}

// Take a picture, read it and send it trhough the serial port.
// getPicture -- send the pixel data to the requester
// START:  XCK Falling Edge
// FINISH: XCK Just before Rising Edge
void camReadPicture()
{
  boolean searching = true;  // current state
  boolean bright = false;    // flag for pixel brightness
  unsigned int pixel;       // Buffer for pixel read in
  int x, y;

  // Seems to take a long time to send
  // Serial.write(CAM_COM_SEND_START);

  // Camera START sequence
  camClockL();
  camStepDelay();// ensure load bit is cleared from previous call
  cbi(CAM_DATA_PORT, CAM_LOAD_BIT);// START rises before xck
  sbi(CAM_DATA_PORT, CAM_START_BIT);
  camStepDelay();
  camClockH();
  camStepDelay();
  cbi(CAM_DATA_PORT, CAM_START_BIT);// start valid on rising edge of xck, so can drop now
  camStepDelay();
  camClockL();
  camStepDelay();
  camStepDelay();

  // Wait for READ to go high
  while (1) {
    camClockH();
    camStepDelay();
    // READ goes high with rising XCK
    //    if ( inp(CAM_READ_PIN) & (1 << CAM_READ_BIT) )
    if (digitalRead(CAM_READ_PIN) == HIGH) // CAM pin on PB0/D8
      break;
    camStepDelay();
    camClockL();
    camStepDelay();
    camStepDelay();
  }
  camStepDelay();

  for (y = 0; y < 128; y++) {
    searching = true;                                // start in 'searching' state
    for (x = 0; x < 128; x++) {
      camClockL();
      camStepDelay();
      pixel = analogRead(CAM_ADC_PIN) >> 2;// get the next pixel, buffer it, and send it out over serial
      Serial.write(pixel);
      camClockH();
      camStepDelay();
      camStepDelay();

    } // end for x
  } /* for y */

  // Go through the remaining rows
  while ( digitalRead(CAM_READ_PIN) == HIGH ) {
    camClockL();
    camStepDelay();
    camStepDelay();
    camClockH();
    camStepDelay();
    camStepDelay();
  }
  camClockL();
  camStepDelay();
  camStepDelay();
}
