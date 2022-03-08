# Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor
A set of codes to address the Mitsubishi M64282FP artificial retina with an Arduino Uno and a simple GNU Octave image saver. 

There has been many previous attempts to interface this sensor. There is a famous [Arduino port](https://github.com/shimniok/avr-gameboy-cam) which have been coupled with a [java interface](https://www.bot-thoughts.com/2010/04/gameboy-camera-prototyping.html), a [TI Calculator interface](https://www.cemetech.net/projects/item.php?id=54) or an [OpenGL interface](https://www.instructables.com/PC-Interfacing-a-GameBoy-Camera/). However, all these projects (even if not claiming it) uses the same obsolete code originating from the [initial 2005 project](http://sophiateam.undrgnd.free.fr/microcontroller/camera/) as main engine.

The project here is a complete reboot to ensure an Arduino/ESP8266/ESP32 compatibility with up to date commands. The code is much more simple than previous versions too. It embbeds only the strict necessary instructions to be interfaced via the serial protocol. A GNU Octave image reader is provided but any other way of sniffing the serial port to build the images is possible.

Required installations: [Arduino IDE](https://www.arduino.cc/en/software) and [GNU Octave](https://www.gnu.org/software/octave/index).

Simply grab the eye ball of a GB camera, connect to an Arduino Uno following the pinout (harvest or buy a JST ZH1.5MM 9 Pin connector for a conservative approach), upload the code to the board, run the GNU Octave code and enjoy your pixelated images in the ./images/ folder ! You can add a LED to D4 to see when registers are written into the sensor. Sensor registers are commented in the GNU Octave code and can be modified following the [M64282FP user manual](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Mitsubishi%20Integrated%20Circuit%20M64282FP%20Image%20Sensor.pdf). The codes are heavily commented so that launching and modifying them to play with is quite straighforward. I say "quite" because the description and use of registers is a mess in the sensor user manual.

Images are cropped in 112x128 pixels into the Octave code but it reads and process the whole sensor 128x128 pixels images. The 5 lower lines of pixels are adressable but return junk data (the pixels [are physically differents here](https://github.com/Raphael-Boichot/Game-Boy-chips-decapping-project), whatever their function) while the upper part of image displays typical CMOS amplification artifacts at high exposure time. User may modify the code easily to show and save the whole image. 

## Pinout and setup
![setup](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/setup.png)

The Pinout given here is the same than in the Arduino program. I'm not particularly proud of my setting but it does the job for the time of code developpement. Using an Arduino proto shield is strongly advised.

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
