# TeensyCEC
A collection of 80th game console emulators for the Teensy3.6 MCU

# Teensy Console Emulators Collection
Currently 3 game consoles are supported:
Atari2600 (teensyvcs), Philips Videopac/Odyssey (teensyo2em) and Colecovision (teensycolem).

All emulators support both ILI9341 TFT and VGA output using the uVGA library https://github.com/qix67/uVGA
<br>
Can be compiled at 144MHz,180MHz qnd 240 MHz (180MHz is best for all!)


# Minimim requirements:
- Teensy3.6 board
- ILI9341 display
- Analog joypad (Arduino or PSP like)
- 3 buttons (FIRE, USER1 and RESET)
- SDFAT library (https://github.com/greiman/SdFat)
- Audio library (part of the SDK)

# Optional requirements:
- VGA connector according to https://github.com/qix67/uVGA pinout
- 3 more buttons (for USER2,USER3,USER4)
- Digital Joystick port (Atari/C64 like)

# Installation
- Format an SD card as FAT32
- extract the content of SD.zip in the root directory 
- there must be sub-directories for each emulator and the default callibration file for the ILI9341 touch screen
  - 2600 => for teensyvcs, put your Atari 2600 roms here
  - o2em => for teenso2em, put your Videopac/Odysssey roms here 
  - coleco => for teensycolem, put your Colecovision roms here (.rom, including coleco.rom)
  - cal.cfg

# Compilation
- Install SDFAT abd uVGA libraries for the Teensy
- Because the standard Audio library is clashing with the SDFAT library, you need to remove SD streaming from the Audio library 
  - locate play_sd_raw*,play_sd_wav* and play_serialflash_raw* and move them inside a backup directory
- load the ino file in the Arduino IDE and compile for 180MHz

# Status and known issues
- teensycolem:
  - limited keyboard functionality (not all keys are working)
  - but has sound
- teensyvcs:
  - no sound yet
  - limited cartridge size support
- teensyo2em:
  - no sound
  - only videopac G7000 games supported 
- general:
  - menu not perfect!
  - touch screen precision TBV (not sure if calibration works for all)
  - one joystick supported only
  - few bugs in GFX rendering (disturbance in VGA especially)
  


# Running
100. First list item
     - First nested list item
       - Second nested list item
