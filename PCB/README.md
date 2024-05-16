## BOM

- An [Arduino Uno](https://fr.aliexpress.com/item/1005006088733150.html), the cheaper the better;
- PCB ordered at [JLCPCB](https://jlcpcb.com/). Just drop the Gerber .zip files on their site and order with default options.
- Some [male pin headers](https://fr.aliexpress.com/item/1005006104110168.html). The clearance with the Arduino Uno shield may be tight, so trim any pins showing below the PCB (if any); 
- Some [9 pins JST/ZH1.5MM connector](https://fr.aliexpress.com/item/1005006028155508.html). Choose **vertical or horizontale SMD**, ZH1.5MM, 9P. The PCB was made for both (just follow the markings).
- 1 [regular 5 mm LEDs](https://fr.aliexpress.com/item/32848810276.html) and 1 [through hole resistors](https://fr.aliexpress.com/item/32866216363.html) of 220 Ohms (low value = high brighness).

Use this PCB at your own risk. STRB and TADD pins are intended to be connected from a MF64283FP sensor (specific strategy not tested yet). Sub board to use a MF64283FP in a Game boy Camera (and with this project), can be found [here](https://github.com/HerrZatacke/M64283FP-Camera-PCB). Wires to connect TADD and STRB must be external lines.

Regular MF64282FP can be just connected with the regular camera ribbon.
