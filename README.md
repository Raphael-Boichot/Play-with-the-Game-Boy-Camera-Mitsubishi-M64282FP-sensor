# Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor
A set of codes to address the Mitsubishi M64282FP artificial retina with an Arduino Uno and a simple GNU Octave image saver. Code is a complete reboot of [this old project](https://github.com/shimniok/avr-gameboy-cam) to allow Arduino/ESP8266/ESP32 compiling. The original project, detailed [here](https://www.bot-thoughts.com/2010/04/gameboy-camera-prototyping.html), is a bit more ambitious but uses obsolete libraries and commands.

Required installations: [Arduino IDE](https://www.arduino.cc/en/software) and [GNU Octave](https://www.gnu.org/software/octave/index).

Simply grab the eye ball of a GB camera, connect to an Arduino Uno following the pinout (harvest or buy a JST ZH1.5MM 9 Pin connector for a conservative approach), upload the code to the board, run the GNU Octave code and enjoy your 128x112 pixel images in the ./images/ folder ! You can add a LED to D4 to see when registers are written into the sensor. Sensor registers are commented in the GNU Octave code and can be modified following to the [M64282FP user manual](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Mitsubishi%20Integrated%20Circuit%20M64282FP%20Image%20Sensor.pdf). The codes are heavily commented so that launching and modifying them to play with is quite straighforward. I say "quite" because the description and use of registers is a mess in the sensor user manual.

Images are cropped in 112x128 pixels into the Octave code but it reads and process the whole sensor 128x128 pixels images. The 5 lower lines of pixels are adressable but return junk data (the pixels [are physically differents here](https://github.com/Raphael-Boichot/Game-Boy-chips-decapping-project), whatever their function) while the upper part of image displays typical CMOS amplification artifacts at high exposure time. User may modify the code easily to show and save the whole image. 

## Pinout and setup
![setup](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/setup.png)

Pinout was cross-referenced from this [page](https://www.cemetech.net/projects/item.php?id=54) and this [page](https://www.google.com/amp/s/www.instructables.com/PC-Interfacing-a-GameBoy-Camera/%3famp_page=true). The Pinout given here is the same than in the Arduino program.

## Example of image output
![results](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/results.png)

You can notice that 2D edge enhancement (on the right) creates image artifacts (vertical streaks and horizontal splitting of image exposure) well known from the Game Boy Camera freaks. In consequence, these artifacts are intrinsic to the M64282FP sensor and not a bug into the Game Boy Camera code implementation. I weared the same black jacket on the two images and you can see the infrared fluorescence on the left one (with outdoor ligth), the sensor being particularly sensitive to it.

## Example of image artifacts
![results](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Artifacts.png)

Vertical and horizontal artifacts seen on the Game Boy Camera are just a matter of exposure level and intrinsic to the sensor. They are increased by the edge enhancement feature. Anyway, the image quality is remarquable seing the sensor resolution and the crap plastic lens used with it.

## Typical register setting used by the Game Boy Camera
![setting](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/setting.png)

These are what I think to be the registers used by the Game Boy Camera... Images taken with these settings are very similar at least.

## Known bugs
The registers are sometimes not transmitted correctly by the serial protocol for unknown reason. I must admit that bi-directionnal serial protocol between Arduino and GNU Octave was a real pain to code from scratch and there are still some mysteries to me. I suspect the GNU Octave implementation of serial protocol to have many bugs.
