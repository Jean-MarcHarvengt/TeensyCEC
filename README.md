# TeensyCEC
A collection of 80th game console emulators for the Teensy3.6

# Teensy Console Emulators Collection
Currently 3 game consoles are supported:
Atari 2600 (teensyvcs), Philips Videopac Odyssey (teensyo2em) and Colecovision(teensycolem).

All emulators support both TFT and VGA display using uVGA library https://github.com/qix67/uVGA
<br>
Can be compiled at 144MHz,180MHz qnd 240 MHz (180MHz is best for all!)


# Minimim Requirements:
- Teensy3.6 board
- ILIili9341 display
- Analog joypad (Arduino or PSP like)
- 3 buttons (for FIRE, USER1 and RESET)
- SDFAT library
- Audio library 

# Optional Requirements:
- VGA connector according to https://github.com/qix67/uVGA
- 3 more buttons (for USER2,USER3,USER4)
- Digital Joystick port (Atari/C64 type)
