# Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor
A set of codes to address the Mitsubishi M64282FP artificial retina with an Arduino Uno. Code is mainly an update/simplification of [this old project](https://github.com/shimniok/avr-gameboy-cam) to go as fast as possible from the reading of this present page to something working.

Simply grab the eye ball of a GB camera, connect to an Arduino Uno (harvest or buy a JST ZH1.5MM 9 Pin connector for a conservative approach), upload the code to the board, run the GNU Octave code and enjoy your images ! You can add a LED to D4 to see when data are captured into the serial. Sensor registers are commented in the Arduino code and can be modified with some painfull calculations according to the [M64282FP user manual](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/Mitsubishi%20Integrated%20Circuit%20M64282FP%20Image%20Sensor.pdf). 

Images are cropped in 112x128 pixels into the Octave code but it reads and process the whole sensor 128x128 pixels images. The 5 lower lines of pixels are adressable but return junk data (the pixels [are physically differents here](https://github.com/Raphael-Boichot/Game-Boy-chips-decapping-project), whatever their function) while the upper part of image displays amplification artifacts at high exposure time. User may modify the code easily to show and save the whole image. 

# Pinout with Arduino Uno
![pinout](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/pinout.png)

# Global setup
![setup](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/setup.png)

# Example of image output
![results](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/results.png)

You can notice that aggressive 2D edge enhancement (on the right) creates image artifacts (vertical streaks and horizontal splitting of image exposure) well known from the Game Boy Camera freaks. In consequence, these artifacts are intrinsic to the M64282FP sensor and not a bug into the Game Boy Camera code implementation. I weared the same black jacket on the two images and you can see the infrared fluorescence on the left one (with outdoor ligth), the sensor being particularly sensitive to it.

# Typical register setting used by the Game Boy Camera
![setting](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/Additionnal%20informations/setting.png)

Setting the registers to get a correct image is a trial-and-error work as the user manual of the sensor is all but obvious to understand. Some good settings returns only chunks of good images with lots of artifacts for no reason. Some obvious settings return unexpected results. You have to try how to find good results for your lighting conditions by yourself.
