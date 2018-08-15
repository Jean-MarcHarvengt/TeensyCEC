# TeensyCEC
A collection of 80th game console emulators for the Teensy3.6

# Teensy Console Emulators Collection
Currently 3 consoles supported:
Atari 2600, Philips Videopac Odyssey and Colecovision.

All emulators support both TFT and VGA display using uVGA library https://github.com/qix67/uVGA
Can be compiled at 144MHz,180MHz qnd 240 MHz (180MHz is best for all!)


# Minimim Requirements:
- Teensy 3.6 board
- ILIili9341 display
- Analog joypad (arduino or PSP like)
- 3 buttons (forFIRE, USER1 and RESET)
- SDFAT library
- Audio library 

# Optional Requirements:
- VGA connector according to https://github.com/qix67/uVGA
- 3 more buttons (for USER2,USER3,USER4)
- Digital Joystick port (Atari/C64 type)
