# Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor
By Raphaël BOICHOT, February 2022. Last major update: 2022-03-11.

**A set of codes to bit bang the Mitsubishi M64282FP artificial retina with an Arduino Uno and a simple GNU Octave/Matlab image converter.**

There has been many previous attempts to interface this sensor out of a Game Boy Camera. The most famous is an [AVR/Arduino port](https://github.com/shimniok/avr-gameboy-cam) which have been coupled with a [java interface](https://www.bot-thoughts.com/2010/04/gameboy-camera-prototyping.html) or a [TI Calculator](https://www.cemetech.net/projects/item.php?id=54). However, these projects use the same obsolete code originating from the [initial 2005 project](http://sophiateam.undrgnd.free.fr/microcontroller/camera/) as main engine.

The project here is a complete reboot from the initial AVR code to ensure an Arduino/ESP8266/ESP32 compatibility with up to date commands. The code is much more simple than previous versions too. It embbeds only the strict necessary instructions to be interfaced via the serial protocol. A GNU Octave/Matlab image reader is provided but any other way of sniffing the serial port to build the images is possible.

Required installations: [Arduino IDE](https://www.arduino.cc/en/software) and [GNU Octave](https://www.gnu.org/software/octave/index).

Simply grab the eye ball of a Game Boy camera, connect to an Arduino Uno following the pinout (harvest or buy a JST ZH1.5MM 9 Pin connector for a conservative approach), upload the code to the board, run the GNU Octave code and enjoy your pixelated images in the ./images/ folder ! You can add a LED to D4 to see when registers are written into the sensor. Sensor registers are commented in the GNU Octave code and can be modified following the [M64282FP user manual](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Mitsubishi%20Integrated%20Circuit%20M64282FP%20Image%20Sensor.pdf). The codes are heavily commented so that launching and modifying them to play with is quite straighforward. I say "quite" because the description and use of registers is a mess in the sensor user manual.

How does it work ? Basically the Arduino waits for 8x8 bits of data (the registers) to be sent on the serial port. When the 8 bytes are received (sent by the Octave code), the Arduino injects them into the sensor and reads the 128x128 analog values of each pixel. The natively 10-bits converted values are truncated to 8-bits by removing the two least significant bits and sent back to the serial as a unique byte per pixel. Once finished, the Arduino enters a waiting mode for new registers. The Octave code reads the serial, process the stream of bytes into an image (vector to matrix ordering, cropping, thresholding and autocontrast) and modifies or not the registers (depending on pixel intensity) before injecting them again on the serial, and so on. The whole process take about 2 seconds, the bottleneck beeing the slow analog-to-digital conversion on Arduino.

Images are cropped in 112x128 pixels into the Octave code but it reads and process the whole sensor 128x128 pixels images. The 5 lower lines of pixels are adressable but return junk data (the pixels [are physically differents here](https://github.com/Raphael-Boichot/Game-Boy-chips-decapping-project), see next chapter) while the upper part of image displays typical CMOS amplification artifacts at high exposure time. User may modify the code easily to show and save the whole image. 

Reader may read with interest the [user manual of the M64283FP](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Mitsubishi%20Integrated%20Circuit%20M64283FP%20Image%20Sensor.pdf), the next sensor of the series, which is much more detailed about what may be the function of the M64282FP junk pixels area which is called "dark pixels column". This is basically an area of obfuscated pixels on purpose. The manual indicates how to modify the registers practically from the signal extracted from these pixels (see the dark level calibration section). Some forbidden states, probably common to the two sensors, are also described. Apart from some new registers and features, the M64283FP seems to share many functions and settings of the M64282FP.

## Exemple of normal pixel and dark pixel on the sensor
![dark pixel](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Mitsubishi%20M64282FP_detail%20of%20light%20sensors.png)

High resolution images of the Game Boy Camera sensor and details of the pixels can be found [here](https://github.com/Raphael-Boichot/Game-Boy-chips-decapping-project).

## Pinout and setup
![setup](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/setup.png)

The pinout given here is the same than in the Arduino program. I'm not particularly proud of my setting but it does the job for the time of code developpement. Using an Arduino proto shield is strongly advised.

## Example of image output
![results](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/results.png)

You can notice that 2D edge enhancement (on the right) reveals image artifacts (vertical streaks and horizontal splitting of image exposure) well known from the Game Boy Camera freaks. In consequence, these artifacts are intrinsic to the M64282FP sensor and not a bug into the Game Boy Camera code implementation. I weared the same black jacket on the two images and you can see the infrared fluorescence on the left one (with outdoor ligth), the sensor being particularly sensitive to it. I was anyway impressed by the sensor sensitivity in very low light conditions. On the contrary, I find that the sensor performs not that well in daylight.

## Example of image output with lens modification
![results](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Examples.png)

The sensor resolution is high enough to perform convincing astrophotography. Used at the back of a film camera, the crop factor is approximately x10, wich means that the equivalent focal lenght is multiplied by 10. On the right image of the moon, the lens is 300 mm for a 24x36 mm film, which corresponds to 3000 mm for the tiny Mitsubishi sensor.

## Example of image artifacts
![results](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Artifacts.png)

Vertical and horizontal artifacts seen on the Game Boy Camera are just a matter of exposure level and are intrinsic to the sensor. They are increased by the edge enhancement feature and are always more or less present (for certain exposure settings both vertical and horizontal artifacts are even present). Anyway, the image quality is remarquable seing the sensor resolution and the crap plastic lens used with it. Edge enhancement globally improves the quality of image.

## Typical register setting used by the Game Boy Camera
![setting](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/setting.png)

These are what I think to be the registers used by the Game Boy Camera... Images taken with these settings are very similar at least.

## Known limitations and perspectives
As explained, the whole process to display an image is sluggish as hell due to the ADC (Analog-to-Digital Converter) of Arduino which uses a successive approximation method. I've tried to use the [Analog read fast library](https://github.com/avandalen/avdweb_AnalogReadFast), which for sure increases the refresh rate (by a factor of 2 approximately), but renders the code incompatible with ESP8266/ESP32. So the code is given in two versions, with the native analogRead() command for Arduino/ESP and with the analogReadFast() command for Arduino only. I think that ESPs will be much faster thanks to their high working frequency (240 MHz for ESP32 versus 16 MHz for Arduino Uno).

The autoexposure mode proposed in Octave is coded for the registers entered by default. Changing the filtering kernels also modifies the setpoint for autoexposure which leads to disappointing results. The autoexposure in its current form is rather primitive: it just modifies exposure time so that the average of raw data from an image targets a certain value. I assume that modifying both exposure time and gain, or taking into account the range of raw values and not the average would perhaps lead to something more efficient and universal. I also proposed an image output with 8x8 Bayer matrix dithering but the aesthetical result does not satisfy me at all.

Finally, using a Mitsubishi M64282FP artificial retina is less and less interesting from a technical point of view in 2022, considering the availability of simplier way to display images from Arduino/ESP/Raspberry with dedicated shields. Game Boy Camera is also becoming more and more expensive as it becomes part of the videogames history. So the idea now is more to play with the sensor and its registers **without destroying a Game Boy Camera** to see how advanced the sensor was for 1998. And because playing with this sensor is in fact just fun.

## To do
- Implement a dithering algorithm similar to the one coded into the Game Boy Camera reverse engineered in [this project](https://github.com/AntonioND/gbcam-rev-engineer).

## Ideas
- Embedding the auto-exposure feature, currently made by Octave/Matlab, into the Arduino code, add an SD shield (storing raw binary data on SD) and make a standalone capture device. Would necessitate an external code to rebuild the images from raw data stored on SD card;
- Shifting code to an ESP32 with LCD display, SD shield and embedding a [PNG converter](https://github.com/Raphael-Boichot/PNGenc) too. This will make a kind of primitive digital camera using the sensor;
- Sending the images over wifi to make a primitive low-res webcam out of the sensor;
