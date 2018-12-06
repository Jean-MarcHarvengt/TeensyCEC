#ifndef IOPINS_H
#define IOPINS_H

//#include <Arduino.h>

// ILI9341
#define TFT_SCLK        13
#define TFT_MOSI        12
#define TFT_MISO        11  
#define TFT_TOUCH_CS    38
#define TFT_TOUCH_INT   37
#define TFT_DC          9
#define TFT_CS          10
#define TFT_RST         255  // 255 = unused, connected to 3.3V


// I2C keyboard
//#define I2C_SCL_IO        (gpio_num_t)5 
//#define I2C_SDA_IO        (gpio_num_t)4 


// Analog joystick (primary) for JOY2 and 5 extra buttons
#define PIN_JOY2_A1X    A12
#define PIN_JOY2_A2Y    A13
#define PIN_JOY2_BTN    36
#define PIN_KEY_USER1   35
#define PIN_KEY_USER2   34
#define PIN_KEY_USER3   33
#define PIN_KEY_USER4   39
#define PIN_KEY_ESCAPE  23


// Second joystick
#define PIN_JOY1_BTN     30
#define PIN_JOY1_1       16
#define PIN_JOY1_2       17
#define PIN_JOY1_3       18
#define PIN_JOY1_4       19


#endif




