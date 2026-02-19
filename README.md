# Arduino-Leonardo-Dongle
bare-metal rt-stepper dongle firmware for Arduino Leonardo

### rt-stepper dongle
The rt-stepper dongle is a retired product from ecklersoft.com. The dongle was used to convert a parallel port CNC controller into a USB CNC controller. This version of the dongle was based on the Arduino Leonardo. Note, this FW will only run on an ATmega32U4 processor like the Arduino Leonardo, Pro Micro or Arduino Micro. For example the FW will NOT run on the Arduino Uno or Arduino Mega 2560. Reference the images in the image directory for leonardo pin and LED identification. By convention each step/direction Pn number name corresponds to a DB25 pin number.

### What's in this repository?
This repo contains source code that runs on the ATmega32U4 processor. The rtusb_leonardo-1.30-freebie.zip file contains a pre-compiled .hex file that can be used for flashing.

### Arduino Leonardo caveats
   * Board should be 5v, 16mhz
   * Use a ICSP programmer to flash the FW
   * Use Avrdude with the ICSP programmer to flash the FW with the following command
```
      avrdude -p atmega32u4 -c usbasp -u -U flash:w:rtusb_leonardo.hex
```
   * The bootloader will be replaced by the FW
   * The Arduino IDE can NOT be used to flash the FW
   * Once the FW is flashed the board will function as a rt-stepper dongle

