This code... works, but not very smoothly. The image suffers artifacts due to poor timing of the sensor bitbanging loop, the SD library performs very badly and the TFT library is too slow. 

It is probably possible to rewrite everything but even well written the code will suffer an unexpected limitation: the ESP32 memory is limited to 115200 bytes whatever claims Espressif. I can't do much with that.
