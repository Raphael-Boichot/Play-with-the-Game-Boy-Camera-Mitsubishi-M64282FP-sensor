/**
 * @author     Laurent Saint-Marcel (lstmarcel@yahoo.fr)
 * @date       2005/07/05
 * Simplified and cleaned by Raphaël BOICHOT in February 2022
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

// PORT C: Analogic/digital converter
#define CAM_ADC_PIN   A3

/* ------------------------------------------------------------------------ */
/* GLOBALS                                                                  */
/* ------------------------------------------------------------------------ */
//Typical register setting used by the GB camera with 2D edge enhancement
//Z1 Z0 O5 O4 O3 O2 O1 O0 zero point calibration and output reference voltage
int reg0=0b10000000;
//N VH1 VH0 G4 G3 G2 G1 G0 
int reg1=0b01100000;
//C17 C16 C15 C14 C13 C12 C11 C10 / exposure time by 4096 ms steps (max 1.0486 s)
int reg2=0b00000011;
//C07 C06 C05 C04 C03 C02 C01 C00 / exposure time by 16 µs steps (max 4096 ms)
int reg3=0b00000000;
//P7 P6 P5 P4 P3 P2 P1 P0 
int reg4=0b00000010;
//M7 M6 M5 M4 M3 M2 M1 M0
int reg5=0b00000101;
//X7 X6 X5 X4 X3 X2 X1 X0
int reg6=0b00000001;
//E3 E2 E1 E0 I V2 V1 V0
int reg7=0b00000111;

// example of default value for registers giving a smooth image
// no edge detection, exposure=0,30, offset=-27, vref=+1.0
//unsigned char camReg[8]={155,0,3,0,1,0,1,7};

unsigned char camReg[8]={reg0,reg1,reg2,reg3,reg4,reg5,reg6,reg7};
unsigned char camClockSpeed = 0x07; // was 0x0A


/* ------------------------------------------------------------------------ */
/* MACROS                                                                   */
/* ------------------------------------------------------------------------ */

#define Serialwait()   while (Serial.available() == 0) ;

int dataIn;
int dataOut;
boolean dataReady;
unsigned char reg;

/* ------------------------------------------------------------------------ */
/* Initialize all components                                                */
/* ------------------------------------------------------------------------ */
void setup()
{

  dataReady = false;
 	Serial.begin(115200);
  Serial.println("");
  Serial.println("Camera initialisation");
  Serial.println("");
	camInit();
	/* enable interrupts */
	sei();
}

/* ------------------------------------------------------------------------ */
/* Program entry point                                                      */
/* ------------------------------------------------------------------------ */
void loop()
{

      camReset();
      camSetRegisters();
      Serial.println("");
      Serial.println("Get new image");
      digitalWrite(4,HIGH);
      camReadPicture(true, true); // get both pixels and objects
      digitalWrite(4,LOW);
      Serial.println("");
      delay(500);
      camReset();
  
} // loop

///////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// CAM TIMING AND CONTROL
// cbi(port, bitId) = clear bit(port, bitId) = Set the signal Low
// sbi(port, bitId) = set bit(port, bitId) = Set the signal High
// Delay used between each signal sent to the AR (four per xck cycle).
void camStepDelay() {
  	unsigned char u=camClockSpeed;
	while(u--) {__asm__ __volatile__ ("nop");}
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
  pinMode(8, INPUT);   // READ
  pinMode(9, OUTPUT);  // XCK
  pinMode(10, OUTPUT);  // XRST
  pinMode(11, OUTPUT); // LOAD
  pinMode(12, OUTPUT); // SIN
  pinMode(13, OUTPUT); // START
  pinMode(4, OUTPUT);   // LED
  
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
  for(reg=0; reg<8; ++reg) {
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
  for(bitmask = 0x4; bitmask >= 0x1; bitmask >>= 1){
    camClockL();
    camStepDelay();
    // ensure load bit is cleared from previous call
    cbi(CAM_DATA_PORT, CAM_LOAD_BIT);
    // Set the SIN bit
    if(regaddr & bitmask)
      sbi(CAM_DATA_PORT, CAM_SIN_BIT);
    else
      cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();

    camClockH();
    camStepDelay();
    // set the SIN bit low
    cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();
  }

  // Write 7 most significant bits of 8-bit data.
  for(bitmask = 128; bitmask >= 1; bitmask>>=1){
    camClockL();
    camStepDelay();
    // set the SIN bit
    if(regval & bitmask)
      sbi(CAM_DATA_PORT, CAM_SIN_BIT);
    else
      cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();
    // Assert load at rising edge of xck
    if (bitmask == 1)
      sbi(CAM_DATA_PORT, CAM_LOAD_BIT);
    camClockH();
    camStepDelay();
    // reset the SIN bit
    cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();
  }
}

// Take a picture, read it and send it trhough the serial port. 
// getPicture -- send the pixel data to the requester
// START:  XCK Falling Edge 
// FINISH: XCK Just before Rising Edge
void camReadPicture(boolean getPixels, boolean getObjects)
{
  boolean searching=true;    // current state
  boolean bright=false;      // flag for pixel brightness
  unsigned int pixel;       // Buffer for pixel read in
  int x,y;

  // Seems to take a long time to send
 // Serial.write(CAM_COM_SEND_START);
 
  // Camera START sequence
  camClockL();
  camStepDelay();
  // ensure load bit is cleared from previous call
  cbi(CAM_DATA_PORT, CAM_LOAD_BIT);
  // START rises before xck
  sbi(CAM_DATA_PORT, CAM_START_BIT);
  camStepDelay();
  camClockH();
  camStepDelay();
  // start valid on rising edge of xck, so can drop now
  cbi(CAM_DATA_PORT, CAM_START_BIT);
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
  
//  if (getPixels)
//  Serial.write(CAM_COM_START_READ);
  //sbi(CAM_LED_PORT, CAM_LED_BIT);
  camStepDelay();


  for (y = 0; y < 128; y++) {

    searching = true;                                // start in 'searching' state
    for (x = 0; x < 128; x++) {
      camClockL();
      camStepDelay();
      // get the next pixel, buffer it, and send it out over serial
      pixel = analogRead(CAM_ADC_PIN) >> 2;
      if (getPixels) {

        Serial.write(pixel);
       //Serial.print(" ");
//       if (pixel < 16) {Serial.print("0");}
//       Serial.print(pixel,HEX);
      }
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
