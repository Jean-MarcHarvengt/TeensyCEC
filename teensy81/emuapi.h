#ifndef EMUAPI_H
#define EMUAPI_H

#define HAS_SND     1
#define HAS_I2CKBD  1

// Title:     <                                        >
#define TITLE "          ZX81/ZX80 Emulator            "
#define ROMSDIR "z81"

#define emu_Init(ROM) {z81_Start(ROM); z81_Init(); }
#define emu_Step(x) {z81_Step();}

#define PALETTE_SIZE         2
#define VID_FRAME_SKIP       0x3
#define TFT_VBUFFER_YCROP    0
#define SINGLELINE_RENDERING 1

#define ACTION_NONE          0
#define ACTION_MAXKBDVAL     225
#define ACTION_EXITKBD       128
#define ACTION_RUNTFT        129
#define ACTION_RUNVGA        130

#ifdef KEYMAP_PRESENT

#define TAREA_W_DEF          32
#define TAREA_H_DEF          32
#define TAREA_END            255
#define TAREA_NEW_ROW        254
#define TAREA_NEW_COL        253
#define TAREA_XY             252
#define TAREA_WH             251

#define KEYBOARD_X           10
#define KEYBOARD_Y           8
#define KEYBOARD_KEY_H       40
#define KEYBOARD_KEY_W       30
#define KEYBOARD_HIT_COLOR   RGBVAL16(0xff,0x00,0x00)

const unsigned short keysw[] = {
  TAREA_XY,KEYBOARD_X,KEYBOARD_Y,
  TAREA_WH,KEYBOARD_KEY_W,KEYBOARD_KEY_H,
  TAREA_NEW_ROW,30,30,30,30,30,30,30,30,30,30,
  TAREA_NEW_ROW,30,30,30,30,30,30,30,30,30,30,
  TAREA_NEW_ROW,30,30,30,30,30,30,30,30,30,30,
  TAREA_NEW_ROW,30,30,30,30,30,30,30,30,30,30,
  TAREA_END};
   
const unsigned char keys[] = {
  30,31,32,33,34,35,36,37,38,39,
  20,26, 8,21,23,28,25,12,18,19,
   4, 9, 7,22, 4,11,13,14,15,40,
  25, 6,27,29,224,5,17,16,225,44 
  };  

#ifdef HAS_I2CKBD
const unsigned short i2ckeys[] = {
     0X0080,0X0008,0X0180,0X0108,0X0280,0X0208,0X0380,0X0308,0X0480,0X0408,
     0X0040,0X0004,0X0140,0X0104,0X0240,0X0204,0X0340,0X0304,0X0440,0X0404,
     0X0020,0X0002,0X0120,0X0102,0X0220,0X0202,0X0320,0X0302,0X0420,0X0402,
     0X0010,0X0001,0X0110,0X0101,0X0210,0X0201,0X0310,0X0301,0X0410,0X0401,
  };
#endif
   
#endif


#define PIN_JOY1_BTN     30
#define PIN_JOY1_1       16
#define PIN_JOY1_2       17
#define PIN_JOY1_3       18
#define PIN_JOY1_4       19

// Analog joystick for JOY2 and 5 extra buttons
#define PIN_JOY2_A1X    A12
#define PIN_JOY2_A2Y    A13
#define PIN_JOY2_BTN    36
#define PIN_KEY_USER1   35
#define PIN_KEY_USER2   34
#define PIN_KEY_USER3   33
#define PIN_KEY_USER4   39
#define PIN_KEY_ESCAPE  23

#define MASK_JOY2_RIGHT 0x001
#define MASK_JOY2_LEFT  0x002
#define MASK_JOY2_UP    0x004
#define MASK_JOY2_DOWN  0x008
#define MASK_JOY2_BTN   0x010
#define MASK_KEY_USER1  0x020
#define MASK_KEY_USER2  0x040
#define MASK_KEY_USER3  0x080
#define MASK_KEY_USER4  0x100
#define MASK_KEY_ESCAPE 0x200



extern void emu_init(void);
extern void emu_printf(char * text);
extern void emu_printi(int val);
extern void * emu_Malloc(int size);
extern void emu_Free(void * pt);

extern int emu_FileOpen(char * filename);
extern int emu_FileRead(char * buf, int size);
extern void emu_FileClose(void);
extern int emu_FileSize(void);
extern int emu_LoadFile(char * filename, char * buf, int size);
extern int emu_LoadFileSeek(char * filename, char * buf, int size, int seek);
extern void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index);
extern void emu_DrawScreen(unsigned char * VBuf, int width, int height, int stride);
extern void emu_DrawLine(unsigned char * VBuf, int width, int height, int line);
extern void emu_DrawVsync(void);

extern void emu_InitJoysticks(void);
extern int emu_SwapJoysticks(int statusOnly);
extern unsigned short emu_DebounceLocalKeys(void);
extern int emu_ReadKeys(void);
extern int emu_GetPad(void);
extern int emu_ReadI2CKeyboard(void);
extern void emu_sndPlaySound(int chan, int volume, int freq);
extern void emu_sndPlayBuzz(int size, int val);
extern void emu_sndInit();
extern void emu_resetus(void);
extern int emu_us(void);


#endif




