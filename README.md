# TeensyCEC
A collection of 80th game console emulators for the Teensy3.6 MCU
<p float="left">
  <img src="/images/full1.jpg" width="400" />
  <img src="/images/full2.jpg" width="400" />
  <img src="/images/minimal.jpg" width="400" /> 
</p>



# Teensy Console Emulators Collection
4 game consoles are currently supported:
Atari2600 (teensyvcs), Philips Videopac/Odyssey (teensyo2em), Colecovision (teensycolem) and NES (teensynes).<br>
And 2 computer core supported:
Zx81 (teensy81) and Zx Spectrum (teensyspeccy)

All emulators support both ILI9341 TFT and VGA output using the uVGA library https://github.com/qix67/uVGA
<br>
Can be compiled at 144MHz,180MHz qnd 240 MHz (180MHz is best for all!), only
NES requires 240Mhz for better performances.


# Minimalrequirements:
- Teensy3.6 board
- ILI9341 display with touch screen (I use the 2.8 inches version)
- Analog joypad (Arduino or PSP like)
- 3 buttons (FIRE, USER1 and RESET)
- SDFAT library (https://github.com/greiman/SdFat)
- Audio library (part of the SDK)
- Optionnaly: i2c_t3 library https://github.com/nox771/i2c_t3

# Optional requirements:
- VGA connector according to https://github.com/qix67/uVGA pinout
- 3 more buttons (for USER2,USER3,USER4)
- Digital 9 pins port for external joystick (Atari/C64 like joystick)
- I2C custom keyboard usin i2c_t3 library 

# Wiring
- see pinout.txt file in the project

# Installation
- Format the SD card as FAT32
- extract the content of SD.zip in the root directory 
- there must be a sub-directorie for each emulator and the default callibration file for the ILI9341 touch screen
  - "2600" => for teensyvcs, put your Atari 2600 roms here (.bin)
  - "o2em" => for teenso2em, put your Videopac/Odysssey roms here (.bin)
  - "coleco" => for teensycolem, put your Colecovision roms here (.rom, including coleco.rom)
  - "nes" => for teensynes, put your .nes files here, onlt 32k games are supported (galaga,xevious,mario1...)
  - "spec" => for teensyspeccy, put your ".z80" or ".sna" files here ( max 48K games so typically smaller z80 images)
  - "z81" => for teensy81, put your ".p" files here.
  - "cal.cfg"  
- insert the microSD in the teensy

# Compilation
- Install SDFAT abd uVGA libraries for the Teensy (see links above)
- Because the standard Audio library is clashing with the SDFAT library, you need to remove SD streaming from the Audio library 
  - locate play_sd_raw*,play_sd_wav* and play_serialflash_raw* and move them inside a backup directory
- load the emulator's ino file in the IDE and compile for 180MHz (or whatever best frequency)

# Status and known issues
- teensycolem:
  - has sound!
- teensyvcs:
  - no sound yet
  - limited cartridge size support (due to ram constraint)
- teensyo2em:
  - no sound
  - only videopac G7000 games supported (due to ram constraint)
- teensynes:
  - no sound
  - 32k roms only supported
  - needs a little bit of speedup but playable...
- teensyspeccy:
  - Z80 and SNA support
  - YM and preliminary buzz sound support
  - 48k games only supported
  - kempston joystick supported but on screen keyboard may not answer in some games
  - I2C custom keyboard support!
- teensy81:
  - .P format support
  - no sound
  - I2C custom keyboard support!
  - no HIRES


- general:
  - menu not perfect!
  - touch screen precision to be checked (not sure if calibration works for all)
  - one joystick supported only
  - few bugs in GFX rendering (disturbance in VGA especially)

# Running
- download the compiled project to the teensy from the SDK (only one emulator at a time is supported!)
- you should see the roms you copied on the SD being listed
- if you touch the screen while rebooting the teensy, you enter the callibration mode:
  - follow the instructions and click the red cross at each corner untill the procedure is finished
  - then touch the center and the menu will be displayed again...
- you can select the rom with up/down or via the touch screen or the touch arrows at the left
- you can start the game in TFT mode by pressing the FIRE key or clicking the TFT icon at the right
- you can start the game in VGA mode by pressing the USER1 key or clicking the VGA icon at the right
- while the game is running, clicking the touch screen will show up the emulator's keyboard/keypad for more interaction.
  - you can just click on the on screen key to perform the corresponding action,it will leave the keyboard also
  - e.g. coleco games typically requires a digit to be pressed
- You can then play the game with the analog joystick and the FIRE/USER1 keys  
- press the RESET key to reboot the emulator and load another ROM
- while the menu is shown at startup you can also swap the joysticks and use the external one if it was wired.

# Credits
I mostly ported the emulators from existing projects, all the credit goes to the authors of
colem, o2em , x2600, moarnes and mc-4u projects!
Thanks a lot also to Frank Boesing for his ILI DMA library from which I started from and his great Teensy64 project https://github.com/FrankBoesing/Teensy64

