This code... works, but not very smoothly. The image suffers artifacts due to poor timing of the sensor bitbanging loop (my fault), the ADC is slow as shit (slower than an Arduino Uno working at 16 MHz !) and drifts constantly, the SD library performs very badly (2 seconds delay to just create a fucking file !) and the TFT library I used is a piece of crap (it's SLOW). I was very badly impressed by the ESP32 programming in general. The Espressif documentation is sketchy, the device has very tricky hardware bugs and sometimes things just do not work as intended for no reasons. Plus the random pin ordering on the device complicates everything. All feel cheap in fact.

It is probably possible to rewrite everything but even well written the code will suffer another fatal unexpected limitation: the ESP32 memory is limited to about 110k bytes without using some programming sorcery involving memory management, whatever claims Espressif. I can't do much with that. [Jump here](https://github.com/Raphael-Boichot/Mitsubishi-M64282FP-dashcam) to see what a reliable hardware is able to.

Final word from Espressif: can it be even more confusing ?

![](/ESP32_version_beta/ESP32_specifications.png)
