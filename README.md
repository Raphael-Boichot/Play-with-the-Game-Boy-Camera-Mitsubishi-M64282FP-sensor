# Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor
By Raphaël BOICHOT, February 2022. Last major update: 2022-03-09.

**A set of codes to address the Mitsubishi M64282FP artificial retina with an Arduino Uno and a simple GNU Octave image converter.**

There has been many previous attempts to interface this sensor out of a Game Boy Camera. The most famous is an [AVR/Arduino port](https://github.com/shimniok/avr-gameboy-cam) which have been coupled with a [java interface](https://www.bot-thoughts.com/2010/04/gameboy-camera-prototyping.html) or a [TI Calculator](https://www.cemetech.net/projects/item.php?id=54). However, these projects use the same obsolete code originating from the [initial 2005 project](http://sophiateam.undrgnd.free.fr/microcontroller/camera/) as main engine. And everyone knows that dinosaurs were still alive in 2005 to tell how old this code is.

The project here is a complete reboot from the initial AVR code to ensure an Arduino/ESP8266/ESP32 compatibility with up to date commands. The code is much more simple than previous versions too. It embbeds only the strict necessary instructions to be interfaced via the serial protocol. A GNU Octave image reader is provided but any other way of sniffing the serial port to build the images is possible.

Required installations: [Arduino IDE](https://www.arduino.cc/en/software) and [GNU Octave](https://www.gnu.org/software/octave/index).

Simply grab the eye ball of a GB camera, connect to an Arduino Uno following the pinout (harvest or buy a JST ZH1.5MM 9 Pin connector for a conservative approach), upload the code to the board, run the GNU Octave code and enjoy your pixelated images in the ./images/ folder ! You can add a LED to D4 to see when registers are written into the sensor. Sensor registers are commented in the GNU Octave code and can be modified following the [M64282FP user manual](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Mitsubishi%20Integrated%20Circuit%20M64282FP%20Image%20Sensor.pdf). The codes are heavily commented so that launching and modifying them to play with is quite straighforward. I say "quite" because the description and use of registers is a mess in the sensor user manual.

How does it work ? Basically the Arduino waits for 8x8 bits of data (the registers) to be sent on the serial port. When the 8 bytes are received (sent by the Octave code), the Arduino injects them into the sensor and reads the 128x128 analog values of each pixel. The natively 10-bits values are truncated to 8-bits by removing the two least significant bits and sent back to the serial as a unique byte per pixel. Once finished, the Arduino enters a waiting mode for new registers. The Octave code reads the serial, process the 128x128 bytes into an image and modifies or not the registers to inject on the serial, and so on. The whole process take about 2 seconds, the bottleneck beeing the slow analog-to-digital conversion on Arduino.

Images are cropped in 112x128 pixels into the Octave code but it reads and process the whole sensor 128x128 pixels images. The 5 lower lines of pixels are adressable but return junk data (the pixels [are physically differents here](https://github.com/Raphael-Boichot/Game-Boy-chips-decapping-project), whatever their function) while the upper part of image displays typical CMOS amplification artifacts at high exposure time. User may modify the code easily to show and save the whole image. 

## Pinout and setup
![setup](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/setup.png)

The pinout given here is the same than in the Arduino program. I'm not particularly proud of my setting but it does the job for the time of code developpement. Using an Arduino proto shield is strongly advised.

## Example of image output
![results](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/results.png)

You can notice that 2D edge enhancement (on the right) creates image artifacts (vertical streaks and horizontal splitting of image exposure) well known from the Game Boy Camera freaks. In consequence, these artifacts are intrinsic to the M64282FP sensor and not a bug into the Game Boy Camera code implementation. I weared the same black jacket on the two images and you can see the infrared fluorescence on the left one (with outdoor ligth), the sensor being particularly sensitive to it. I was anyway impressed by the sensor sensitivity in very low light conditions. On the contrary, I find that the sensor performs not that well in daylight.

## Example of image artifacts
![results](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Artifacts.png)

Vertical and horizontal artifacts seen on the Game Boy Camera are just a matter of exposure level and intrinsic to the sensor. They are increased by the edge enhancement feature. Anyway, the image quality is remarquable seing the sensor resolution and the crap plastic lens used with it.

## Typical register setting used by the Game Boy Camera
![setting](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/setting.png)

These are what I think to be the registers used by the Game Boy Camera... Images taken with these settings are very similar at least.

## Known limitations and perspectives
As explained, the whole process to display an image is sluggish as hell due to the ADC (Analog-to-Digital Converter) of Arduino which uses a successive approximation method. I've tried to use the [Analog read fast library](https://github.com/avandalen/avdweb_AnalogReadFast), which for sure increases the refresh rate (by a factor of 2 approximately), but renders the code incompatible with ESP8266/ESP32. So the code is given as it, with the native Arduino analogRead() command. It is certainly not optimal, feel free to improve it, but it works "out of the box". I think that ESPs will be much more faster thanks to their high working frequency (240 MHz for ESP32 versus 16 MHz for Arduino Uno).

The autoexposure mode proposed in Octave is coded for the registers entered by default. Changing the filtering kernels also modifies the setpoint for autoexposure which leads to disappointing results. The autoexposure in its current form is rather primitive: it just modifies exposure time so that the average of raw data from an image targets a certain value. I assume that modifying both exposure time and gain, or taking into account the range of raw values and not the average would perhaps lead to something more efficient and universal.

Finally, using a Mitsubishi M64282FP artificial retina is less and less interesting from a technical point of view with the availability of simplier way to display images from Arduino/ESP/Raspberry with dedicated shields. Game Boy Camera is also becoming more and more expensive and enters progressively the club of retro stuff and hipster toys in 2022. So the idea now is more to play with the sensor and its registers **without destroying a Game Boy Camera** to see how advanced was the sensor for 1998, rather than using it for something "usefull", even if playing IS usefull.
