//Simplified and completely rewritten by Raphaël BOICHOT in 2022/09/23 to be ESP32 compatible
//from a code of Laurent Saint-Marcel (lstmarcel@yahoo.fr) written in 2005/07/05
//stole some code for Rafael Zenaro NeoGB printer: https://github.com/zenaro147/NeoGB-Printer
//version for Arduino here (requires a computer): https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor
//the ADC of ESP/Arduino is very slow (successive approximations) compared to a Flash ADC.
//Use a level shifter on digital pins. Not using one renders the board unstable when booting
//ADC slowness is the bottleneck of the code, just reading 128x128 pixels take about 1 incompressible second
//In consequence, many parts are not optimal but completely hidden by this slowness.

//SD libraries
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define NOP __asm__ __volatile__ ("nop\n\t") //// delay 62.5ns on a 16MHz Arduino, 4.16 ns on ESP32
//on ESP32, 480 clock cycles corresponds to 500 kHz, expected clock frequency for the sensor

//BMP header for a 128x112 8 bit grayscale image (RGB format, non indexed). Height (128) is inverted to have the image in correct orientation
const unsigned char BMP_header [] PROGMEM = {0x42, 0x4D, 0xB6, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x04, 0x00, 0x00, 0x28, 0x00,
                                             0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x80, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x02, 0x02,
                                             0x02, 0x00, 0x03, 0x03, 0x03, 0x00, 0x04, 0x04, 0x04, 0x00, 0x05, 0x05, 0x05, 0x00, 0x06, 0x06,
                                             0x06, 0x00, 0x07, 0x07, 0x07, 0x00, 0x08, 0x08, 0x08, 0x00, 0x09, 0x09, 0x09, 0x00, 0x0A, 0x0A,
                                             0x0A, 0x00, 0x0B, 0x0B, 0x0B, 0x00, 0x0C, 0x0C, 0x0C, 0x00, 0x0D, 0x0D, 0x0D, 0x00, 0x0E, 0x0E,
                                             0x0E, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x10, 0x10, 0x10, 0x00, 0x11, 0x11, 0x11, 0x00, 0x12, 0x12,
                                             0x12, 0x00, 0x13, 0x13, 0x13, 0x00, 0x14, 0x14, 0x14, 0x00, 0x15, 0x15, 0x15, 0x00, 0x16, 0x16,
                                             0x16, 0x00, 0x17, 0x17, 0x17, 0x00, 0x18, 0x18, 0x18, 0x00, 0x19, 0x19, 0x19, 0x00, 0x1A, 0x1A,
                                             0x1A, 0x00, 0x1B, 0x1B, 0x1B, 0x00, 0x1C, 0x1C, 0x1C, 0x00, 0x1D, 0x1D, 0x1D, 0x00, 0x1E, 0x1E,
                                             0x1E, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x20, 0x20, 0x20, 0x00, 0x21, 0x21, 0x21, 0x00, 0x22, 0x22,
                                             0x22, 0x00, 0x23, 0x23, 0x23, 0x00, 0x24, 0x24, 0x24, 0x00, 0x25, 0x25, 0x25, 0x00, 0x26, 0x26,
                                             0x26, 0x00, 0x27, 0x27, 0x27, 0x00, 0x28, 0x28, 0x28, 0x00, 0x29, 0x29, 0x29, 0x00, 0x2A, 0x2A,
                                             0x2A, 0x00, 0x2B, 0x2B, 0x2B, 0x00, 0x2C, 0x2C, 0x2C, 0x00, 0x2D, 0x2D, 0x2D, 0x00, 0x2E, 0x2E,
                                             0x2E, 0x00, 0x2F, 0x2F, 0x2F, 0x00, 0x30, 0x30, 0x30, 0x00, 0x31, 0x31, 0x31, 0x00, 0x32, 0x32,
                                             0x32, 0x00, 0x33, 0x33, 0x33, 0x00, 0x34, 0x34, 0x34, 0x00, 0x35, 0x35, 0x35, 0x00, 0x36, 0x36,
                                             0x36, 0x00, 0x37, 0x37, 0x37, 0x00, 0x38, 0x38, 0x38, 0x00, 0x39, 0x39, 0x39, 0x00, 0x3A, 0x3A,
                                             0x3A, 0x00, 0x3B, 0x3B, 0x3B, 0x00, 0x3C, 0x3C, 0x3C, 0x00, 0x3D, 0x3D, 0x3D, 0x00, 0x3E, 0x3E,
                                             0x3E, 0x00, 0x3F, 0x3F, 0x3F, 0x00, 0x40, 0x40, 0x40, 0x00, 0x41, 0x41, 0x41, 0x00, 0x42, 0x42,
                                             0x42, 0x00, 0x43, 0x43, 0x43, 0x00, 0x44, 0x44, 0x44, 0x00, 0x45, 0x45, 0x45, 0x00, 0x46, 0x46,
                                             0x46, 0x00, 0x47, 0x47, 0x47, 0x00, 0x48, 0x48, 0x48, 0x00, 0x49, 0x49, 0x49, 0x00, 0x4A, 0x4A,
                                             0x4A, 0x00, 0x4B, 0x4B, 0x4B, 0x00, 0x4C, 0x4C, 0x4C, 0x00, 0x4D, 0x4D, 0x4D, 0x00, 0x4E, 0x4E,
                                             0x4E, 0x00, 0x4F, 0x4F, 0x4F, 0x00, 0x50, 0x50, 0x50, 0x00, 0x51, 0x51, 0x51, 0x00, 0x52, 0x52,
                                             0x52, 0x00, 0x53, 0x53, 0x53, 0x00, 0x54, 0x54, 0x54, 0x00, 0x55, 0x55, 0x55, 0x00, 0x56, 0x56,
                                             0x56, 0x00, 0x57, 0x57, 0x57, 0x00, 0x58, 0x58, 0x58, 0x00, 0x59, 0x59, 0x59, 0x00, 0x5A, 0x5A,
                                             0x5A, 0x00, 0x5B, 0x5B, 0x5B, 0x00, 0x5C, 0x5C, 0x5C, 0x00, 0x5D, 0x5D, 0x5D, 0x00, 0x5E, 0x5E,
                                             0x5E, 0x00, 0x5F, 0x5F, 0x5F, 0x00, 0x60, 0x60, 0x60, 0x00, 0x61, 0x61, 0x61, 0x00, 0x62, 0x62,
                                             0x62, 0x00, 0x63, 0x63, 0x63, 0x00, 0x64, 0x64, 0x64, 0x00, 0x65, 0x65, 0x65, 0x00, 0x66, 0x66,
                                             0x66, 0x00, 0x67, 0x67, 0x67, 0x00, 0x68, 0x68, 0x68, 0x00, 0x69, 0x69, 0x69, 0x00, 0x6A, 0x6A,
                                             0x6A, 0x00, 0x6B, 0x6B, 0x6B, 0x00, 0x6C, 0x6C, 0x6C, 0x00, 0x6D, 0x6D, 0x6D, 0x00, 0x6E, 0x6E,
                                             0x6E, 0x00, 0x6F, 0x6F, 0x6F, 0x00, 0x70, 0x70, 0x70, 0x00, 0x71, 0x71, 0x71, 0x00, 0x72, 0x72,
                                             0x72, 0x00, 0x73, 0x73, 0x73, 0x00, 0x74, 0x74, 0x74, 0x00, 0x75, 0x75, 0x75, 0x00, 0x76, 0x76,
                                             0x76, 0x00, 0x77, 0x77, 0x77, 0x00, 0x78, 0x78, 0x78, 0x00, 0x79, 0x79, 0x79, 0x00, 0x7A, 0x7A,
                                             0x7A, 0x00, 0x7B, 0x7B, 0x7B, 0x00, 0x7C, 0x7C, 0x7C, 0x00, 0x7D, 0x7D, 0x7D, 0x00, 0x7E, 0x7E,
                                             0x7E, 0x00, 0x7F, 0x7F, 0x7F, 0x00, 0x80, 0x80, 0x80, 0x00, 0x81, 0x81, 0x81, 0x00, 0x82, 0x82,
                                             0x82, 0x00, 0x83, 0x83, 0x83, 0x00, 0x84, 0x84, 0x84, 0x00, 0x85, 0x85, 0x85, 0x00, 0x86, 0x86,
                                             0x86, 0x00, 0x87, 0x87, 0x87, 0x00, 0x88, 0x88, 0x88, 0x00, 0x89, 0x89, 0x89, 0x00, 0x8A, 0x8A,
                                             0x8A, 0x00, 0x8B, 0x8B, 0x8B, 0x00, 0x8C, 0x8C, 0x8C, 0x00, 0x8D, 0x8D, 0x8D, 0x00, 0x8E, 0x8E,
                                             0x8E, 0x00, 0x8F, 0x8F, 0x8F, 0x00, 0x90, 0x90, 0x90, 0x00, 0x91, 0x91, 0x91, 0x00, 0x92, 0x92,
                                             0x92, 0x00, 0x93, 0x93, 0x93, 0x00, 0x94, 0x94, 0x94, 0x00, 0x95, 0x95, 0x95, 0x00, 0x96, 0x96,
                                             0x96, 0x00, 0x97, 0x97, 0x97, 0x00, 0x98, 0x98, 0x98, 0x00, 0x99, 0x99, 0x99, 0x00, 0x9A, 0x9A,
                                             0x9A, 0x00, 0x9B, 0x9B, 0x9B, 0x00, 0x9C, 0x9C, 0x9C, 0x00, 0x9D, 0x9D, 0x9D, 0x00, 0x9E, 0x9E,
                                             0x9E, 0x00, 0x9F, 0x9F, 0x9F, 0x00, 0xA0, 0xA0, 0xA0, 0x00, 0xA1, 0xA1, 0xA1, 0x00, 0xA2, 0xA2,
                                             0xA2, 0x00, 0xA3, 0xA3, 0xA3, 0x00, 0xA4, 0xA4, 0xA4, 0x00, 0xA5, 0xA5, 0xA5, 0x00, 0xA6, 0xA6,
                                             0xA6, 0x00, 0xA7, 0xA7, 0xA7, 0x00, 0xA8, 0xA8, 0xA8, 0x00, 0xA9, 0xA9, 0xA9, 0x00, 0xAA, 0xAA,
                                             0xAA, 0x00, 0xAB, 0xAB, 0xAB, 0x00, 0xAC, 0xAC, 0xAC, 0x00, 0xAD, 0xAD, 0xAD, 0x00, 0xAE, 0xAE,
                                             0xAE, 0x00, 0xAF, 0xAF, 0xAF, 0x00, 0xB0, 0xB0, 0xB0, 0x00, 0xB1, 0xB1, 0xB1, 0x00, 0xB2, 0xB2,
                                             0xB2, 0x00, 0xB3, 0xB3, 0xB3, 0x00, 0xB4, 0xB4, 0xB4, 0x00, 0xB5, 0xB5, 0xB5, 0x00, 0xB6, 0xB6,
                                             0xB6, 0x00, 0xB7, 0xB7, 0xB7, 0x00, 0xB8, 0xB8, 0xB8, 0x00, 0xB9, 0xB9, 0xB9, 0x00, 0xBA, 0xBA,
                                             0xBA, 0x00, 0xBB, 0xBB, 0xBB, 0x00, 0xBC, 0xBC, 0xBC, 0x00, 0xBD, 0xBD, 0xBD, 0x00, 0xBE, 0xBE,
                                             0xBE, 0x00, 0xBF, 0xBF, 0xBF, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0xC1, 0xC1, 0xC1, 0x00, 0xC2, 0xC2,
                                             0xC2, 0x00, 0xC3, 0xC3, 0xC3, 0x00, 0xC4, 0xC4, 0xC4, 0x00, 0xC5, 0xC5, 0xC5, 0x00, 0xC6, 0xC6,
                                             0xC6, 0x00, 0xC7, 0xC7, 0xC7, 0x00, 0xC8, 0xC8, 0xC8, 0x00, 0xC9, 0xC9, 0xC9, 0x00, 0xCA, 0xCA,
                                             0xCA, 0x00, 0xCB, 0xCB, 0xCB, 0x00, 0xCC, 0xCC, 0xCC, 0x00, 0xCD, 0xCD, 0xCD, 0x00, 0xCE, 0xCE,
                                             0xCE, 0x00, 0xCF, 0xCF, 0xCF, 0x00, 0xD0, 0xD0, 0xD0, 0x00, 0xD1, 0xD1, 0xD1, 0x00, 0xD2, 0xD2,
                                             0xD2, 0x00, 0xD3, 0xD3, 0xD3, 0x00, 0xD4, 0xD4, 0xD4, 0x00, 0xD5, 0xD5, 0xD5, 0x00, 0xD6, 0xD6,
                                             0xD6, 0x00, 0xD7, 0xD7, 0xD7, 0x00, 0xD8, 0xD8, 0xD8, 0x00, 0xD9, 0xD9, 0xD9, 0x00, 0xDA, 0xDA,
                                             0xDA, 0x00, 0xDB, 0xDB, 0xDB, 0x00, 0xDC, 0xDC, 0xDC, 0x00, 0xDD, 0xDD, 0xDD, 0x00, 0xDE, 0xDE,
                                             0xDE, 0x00, 0xDF, 0xDF, 0xDF, 0x00, 0xE0, 0xE0, 0xE0, 0x00, 0xE1, 0xE1, 0xE1, 0x00, 0xE2, 0xE2,
                                             0xE2, 0x00, 0xE3, 0xE3, 0xE3, 0x00, 0xE4, 0xE4, 0xE4, 0x00, 0xE5, 0xE5, 0xE5, 0x00, 0xE6, 0xE6,
                                             0xE6, 0x00, 0xE7, 0xE7, 0xE7, 0x00, 0xE8, 0xE8, 0xE8, 0x00, 0xE9, 0xE9, 0xE9, 0x00, 0xEA, 0xEA,
                                             0xEA, 0x00, 0xEB, 0xEB, 0xEB, 0x00, 0xEC, 0xEC, 0xEC, 0x00, 0xED, 0xED, 0xED, 0x00, 0xEE, 0xEE,
                                             0xEE, 0x00, 0xEF, 0xEF, 0xEF, 0x00, 0xF0, 0xF0, 0xF0, 0x00, 0xF1, 0xF1, 0xF1, 0x00, 0xF2, 0xF2,
                                             0xF2, 0x00, 0xF3, 0xF3, 0xF3, 0x00, 0xF4, 0xF4, 0xF4, 0x00, 0xF5, 0xF5, 0xF5, 0x00, 0xF6, 0xF6,
                                             0xF6, 0x00, 0xF7, 0xF7, 0xF7, 0x00, 0xF8, 0xF8, 0xF8, 0x00, 0xF9, 0xF9, 0xF9, 0x00, 0xFA, 0xFA,
                                             0xFA, 0x00, 0xFB, 0xFB, 0xFB, 0x00, 0xFC, 0xFC, 0xFC, 0x00, 0xFD, 0xFD, 0xFD, 0x00, 0xFE, 0xFE,
                                             0xFE, 0x00, 0xFF, 0xFF, 0xFF, 0x00
                                            };

// the order of pins has no importance except that VOUT must be on some ADC and the pin must accept to be in output mode
int VOUT =  15; //Analog signal from sensor, read shortly after clock is set low, converted to 10 bits by Arduino/ESP ADC then reduced to 8-bits for transmission
int READ =  14; //Read image signal, goes high on rising clock
int CLOCK = 27; //Clock input, pulled down internally, no specification given for frequency
int RESET = 26; //system reset, pulled down internally, active low, sent on rising edge of clock
int LOAD =  25; //parameter set enable, pulled down internally, Raise high as you clear the last bit of each register you send
int SIN =   33; //parameter input, pulled down internally
int START = 32; //Image sensing start, pulled down internally, has to be high before rising CLOCK
int LED =   13; //just for additionnal LED on D4, not essential to the protocol

//define pins of TFT screen
int TFT_CS = 22;//CS
int TFT_RST = 21;//RESET 
int TFT_DC = 17;//A0
int TFT_SCLK = 4;//SCK
int TFT_MOSI = 16;//SDA
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

//default config for SD shield, do not modify pinout on your project
int SD_CS = 5;
// SD_SCK <-> 18
// SD_MOSI <-> 23
// SD_MISO <-> 19

/*
  %some real settings used by the Mitsubishi sensor on Game Boy Camera, except exposure
  reg0=0b10100111;%Z1 Z0 O5 O4 O3 O2 O1 O0 zero point calibration and output reference voltage
  reg1=0b11100100;%N VH1 VH0 G4 G3 G2 G1 G0
  reg2=0b00000001;%C17 C16 C15 C14 C13 C12 C11 C10 / exposure time by 4096 ms steps (max 1.0486 s)
  reg3=0b00000000;%C07 C06 C05 C04 C03 C02 C01 C00 / exposure time by 16 µs steps (max 4096 ms)
  reg4=0b00000001;%P7 P6 P5 P4 P3 P2 P1 P0 filtering kernels
  reg5=0b00000000;%M7 M6 M5 M4 M3 M2 M1 M0 filtering kernels
  reg6=0b00000001;%X7 X6 X5 X4 X3 X2 X1 X0 filtering kernels
  reg7=0b00000011;%E3 E2 E1 E0 I V2 V1 V0 set Vref
*/

// the registers here are used to maximize the voltage range of VOUT (from 1.3 to 2.5 volts, so 1.2 volts min to max), only reg2 and reg3 are modified.
// If the effect of G and V registers are clear on voltage, I did not see any effect of O, so I suspect an issue with my sensor.
// In consequence, I use 0 volts for O register to avoid compatibility issues. Registers are set to have a slight border enhancement and a decent gain.
// reader may refer to the Mitsubishi M64282FP Image Sensor datasheet for further (but unclear) explanations
unsigned char camReg[8] = {0b10000000, 0b11100100, 0b00110100, 0b10101000, 0b00000001, 0b000000000, 0b00000001, 0b00000000}; //registers
unsigned char LUT_serial[16] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46}; //HEX characters in ASCII table
unsigned char reg, char1, char2;
char fileBMP[10];
int cycles = 0; //time delay in processor cycles, to fit with your ESP/Arduino frequency, do not change anything as the ADC is limiting the data flow
int cycles_2 =0;
// sensor returns a 128*128 vector of voltage but only 112 lines contains image information without artifacts.
// Basically, the first and last 8 lines of pixels are ignored. This is the Game Boy Camera definition (for the same reason).
unsigned int RawCamData[128 * 128];// sensor raw data in 8 bits per pixel
unsigned char CamData[128 * 128];// sensor data in 8 bits per pixel
double error;
void setup()
{

  Serial.begin(500000);
  pinMode(READ, INPUT);    // READ
  pinMode(CLOCK, OUTPUT);  // CLOCK
  pinMode(RESET, OUTPUT);  // RESET
  pinMode(LOAD, OUTPUT);   // LOAD
  pinMode(SIN, OUTPUT);    // SIN
  pinMode(START, OUTPUT);  // START
  pinMode(LED, OUTPUT);    // LED

  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  ID_file_creator("/ID_storage.bin");//create a file on SD card that stores a unique file ID from 1 to 2^32 - 1 (in fact 1 to 99999)
  // you have to manually erase that file if you want to initiate again the counter from 1
  tft.initR(INITR_BLACKTAB);//initialise tft screen
  tft.fillScreen(ST7735_BLACK); //large block of text
  tft.setRotation(2);
  camReset();// resets the sensor
}

void loop()
{
  unsigned long file_number = update_get_next_ID("/ID_storage.bin", 1);//get and update the file number on SD card
  Serial.println("");
  Serial.println("Start image acquisition");
  digitalWrite(LED, HIGH);// led is high when the ESP deals with the sensor
  camReset();// resets the sensor
  camSetRegisters();// Send 8 registers to the sensor
  camReadPicture(RawCamData); // get pixels, dump them in RawCamData
  camReset();
  
  Serial.println("End of image acquisition");
  auto_contrast(RawCamData, CamData);// 12 bits data to 8 bits data + auto-contrast, must be called BEFORE auto_exposure
  auto_exposure(camReg, CamData);// Deals with autoexposure (registers 2 and 3) to target a mid-gray value in average
  digitalWrite(LED, LOW);
  sprintf(fileBMP, "/%05d.bmp", file_number);
  Serial.print("Writing image file: ");
  Serial.println(fileBMP);
  writeFile(fileBMP, BMP_header, CamData);
  display_RGB_bitmap(CamData);
  testdrawtext(fileBMP, ST7735_GREEN,0,129);
  
  //dump_data_to_serial(CamData);//dump data to serial for debugging - ultra slow !
} //end of loop

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void dump_data_to_serial(unsigned char CamData[128 * 128]) {
  int counter = 1;
  for (int i = 0; i < 128 * 128; i++) {
    unsigned int pixel = CamData[i];
    char1 = pixel;
    char1 = char1 >> 4;
    char2 = pixel;
    char2 = char2 << 4;
    char2 = char2 >> 4;
    Serial.write(LUT_serial[char1]);
    Serial.write(LUT_serial[char2]);
    Serial.print(" ");
  }
  Serial.println("");
}

void auto_contrast(unsigned int RawCamData[128 * 128], unsigned char CamData[128 * 128]) {
  for (int i = 0; i < 128 * 128; i++) {
    double rawpixel;// Buffer for pixel read in
    unsigned char pixel;

    rawpixel = RawCamData[i];
    // this is the auto-contrast part
    rawpixel = rawpixel - 1240; //1240 corresponds approximately to 1.3 volts on the 12 bits ADC scale, minimal value of sensor output.
    if (rawpixel < 0) {
      rawpixel = 0;
    }
    rawpixel = rawpixel * 0xFFF / (2730 - 1240);//2730 correponds approximately to 2.5 volts on the 12 bits ADC scale, maximal value of the sensor output.
    if (rawpixel > 0xFFF) {
      rawpixel = 0xFFF;
    }
    pixel = int(rawpixel) >> 4;//now sensor values are rescaled between 0x000 and 0xFFF, cut them to 8 bits instead of 12.
    CamData[i] = pixel;
  }
}

void auto_exposure(unsigned char camReg[8], unsigned char CamData[128 * 128]) {
  double exp_regs, new_regs;
  exp_regs = camReg[2] * 256 + camReg[3];// I know, it's a shame to use a double here but we have plenty of ram
  Serial.print("Current exposure: ");
  Serial.println(exp_regs, DEC);
  double mean_value = 0;
  for (int i = 0; i < 128 * 128; i++) {
    unsigned int pixel = CamData[i];
    mean_value = mean_value + pixel;// calculate the mean gray level, but only from line 9 to 120 as top and bottom of image have artifacts
  }
  mean_value = mean_value / (128 * 128);
  Serial.print("Average pixel value: ");
  Serial.println(mean_value, DEC);// we expect ideally a value of 127 here

  error = 127 - mean_value;// so in case of deviation, registers 2 and 3 are corrected
  Serial.print("Error: ");
  Serial.println(error, DEC);

  // this part is very similar to what a Game Boy Camera does, except that it does the job with only bitshift operators and in more steps.
  // Here we can use 32 bits variables for ease of programming.
  // the bigger the error is, the bigger the correction on exposure is.
  new_regs = exp_regs;
  if (error > 50) {
    new_regs = exp_regs * 1.33;
  }
  if (error < -50) {
    new_regs = exp_regs / 1.33;
  }
  if ((error < 50) && (error > 15)) {
    new_regs = exp_regs * 1.1;
  }
  if ((error > -50) && (error < -15)) {
    new_regs = exp_regs / 1.1;
  }
  if ((error < 15) && (error > 5)) {
    new_regs = exp_regs * 1.033;
  }
  if ((error > -15) && (error < -5)) {
    new_regs = exp_regs / 1.033;
  }

  // The sensor is limited to 0xFFFF (about 1 second) in exposure but also has strong artifacts below 0x10 (256 µs).
  // Each step is 16 µs
  if (new_regs < 0x10) {
    new_regs = 0x10;
  }
  if (new_regs > 0xFFFF) {
    new_regs = 0xFFFF;
  }

  Serial.print("New exposure: ");
  Serial.println(new_regs, DEC);
  camReg[2] = int(new_regs / 256);
  camReg[3] = int(new_regs - camReg[2] * 256);
}

void camDelay()// Allow a lag in processor cycles to maintain signals long enough
{
  for (int i = 0; i < cycles; i++) NOP;// may be adjusted but honestly, does not make any difference, the sensor can handle 1 MHz clock frequency
}

void camDelay_special()// Allow a lag in processor cycles to maintain signals long enough
{
  for (int i = 0; i < cycles_2; i++) NOP;// may be adjusted but honestly, does not make any difference, the sensor can handle 1 MHz clock frequency
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

void camReadPicture(unsigned int RawCamData[128 * 128]) // Take a picture.
{
  unsigned int counter = 0;
  unsigned int subcounter = 0;

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
  camDelay_special();
    if (digitalRead(READ) == HIGH) // READ goes high with rising CLOCK
      break;
    digitalWrite(CLOCK, LOW);
    camDelay_special();
  }
  
  for (y = 0; y < 128; y++) {
    for (x = 0; x < 128; x++) {
      digitalWrite(CLOCK, LOW);
      //pixel = analogRead(VOUT) >> 4;// The ADC is 12 bits, this sacrifies the 4 least significant bits to simplify transmission
      //OK, a bit of explanation is needed here: the ADC of ESP32 is 12 bits on the 0-3.3 volts range
      //I had two solutions here: cut to 8 bits directly and deal with autocontrast then. The issue is that the sensor only use the 1.3 to 2.5 volts range.
      //This means basically using than less that 7 bits accuracy with a slow as shit 12 bits ADC, bummer.
      //So the idea is to keep the 12 bits accuracy as long as possible, make the autocontrast with it and then cut to 8 bits for diplaying/saving

      //analogRead being ultra slow on ESP, it is called just when necessary
      //if ((counter >= 8 * 128) && (counter < 128 * 120)) {//crop a middle 128*112 image
//      if (y<16){
//      analogRead(VOUT);
//      analogRead(VOUT);
//      analogRead(VOUT);
//      }
      RawCamData[subcounter] = analogRead(VOUT);
      subcounter = subcounter + 1;
      //}

      counter = counter + 1;
      digitalWrite(CLOCK, HIGH);
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

void writeFile(const char* path, const unsigned char* BMP_header, unsigned char* CamData) {
  //Serial.printf("Writing file: %s\n", path);
  File file = SD.open(path,FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  digitalWrite(LED, LOW);
  file.write(BMP_header, 1078);
  file.write(CamData, 128 * 128);
  file.close();
}

void ID_file_creator(const char * path) {
  uint8_t buf[4];
  if (SD.exists(path)) {
    //skip the step
    Serial.println("Configuration file already on SD card");
    return;
  } else {
    File file = SD.open(path, FILE_WRITE);
    //start from a fresh install on SD
    unsigned long ID = 0;
    buf[3] = ID >>  0;
    buf[2] = ID >>  8;
    buf[1] = ID >> 16;
    buf[0] = ID >> 24;
    file.write(buf, 4);
    file.close();
    Serial.println("Fresh configuration file created on SD card");
  }
}

unsigned long update_get_next_ID(const char * path, int increment) {
  uint8_t buf[4];
  File file = SD.open(path);
  file.read(buf, 4);
  unsigned long Next_ID = ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]));
  Next_ID = Next_ID + increment;
  if (Next_ID == 99999) {
    Next_ID = 1;
  }
  file.close();
  buf[3] = Next_ID >>  0;
  buf[2] = Next_ID >>  8;
  buf[1] = Next_ID >> 16;
  buf[0] = Next_ID >> 24;
  file = SD.open(path, FILE_WRITE);
  file.write(buf, 4);
  file.close();
  return Next_ID;
}

void display_RGB_bitmap(unsigned char CamData[128 * 128]) {
  tft.startWrite();
  tft.setAddrWindow(0, 0, 128, 128);
  for (int i = 0; i < 128 * 128; i++) {
    unsigned int pixel = CamData[i];
    tft.pushColor(tft.color565(pixel, pixel, pixel));
  }
  tft.endWrite();
}

void testdrawtext(char *text, uint16_t color,int x,int y) {
  tft.fillRect(x, y, 128, 32, ST7735_BLACK);
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}
