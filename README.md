# Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor
A set of codes to address the Mitsubishi M64282FP artificial retina with an Arduino Uno. Code is mainly an update/simplification of [this old project](https://github.com/shimniok/avr-gameboy-cam) to go as fast as possible from the reading of the page to an image on your screen.

Simply grab the eye ball of a GB camera, connect to the Arduino (harvest or buy a JST ZH1.5MM 9 Pin connector for cleaner job), upload the code to the board, run the Octave code and enjoy your images ! You can add a LED to D4 to see when data are captured into the serial.

Images are cropped in 112x128 pixels by default but the sensor outputs 128x128 pixels images. User may modify the code to show and save the whole image.

# Pinout with Arduino Uno
![pinout](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/pinout.png)

# Typical register setting used by the Game Boy Camera
![setting](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/setting.png)

# Example of image output
![results](https://github.com/Raphael-Boichot/Play-with-the-Game-Boy-Camera-Mitsubishi-M64282FP-sensor/blob/main/results.png)
