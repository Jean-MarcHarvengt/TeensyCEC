#ifndef EMUAPI_H
#define EMUAPI_H

//#define HAS_SND 1

// Title:     <                                        >
#define TITLE "          Atari2600 Emulator            "
#define ROMSDIR "2600"

extern void vcs_Init(void);
extern void vcs_Start(char * filename);
extern void vcs_Step(void);
#define emu_Init(ROM) {vcs_Init();vcs_Start(ROM);}
#define emu_Step() {vcs_Step();}

#define VID_FRAME_SKIP       0x0
#define PALETTE_SIZE         256
#define TFT_VBUFFER_YCROP    0

#define ACTION_NONE          0
#define ACTION_MAXKBDVAL     4
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

#define KEYBOARD_X           8
#define KEYBOARD_Y           50
#define KEYBOARD_KEY_H       38
#define KEYBOARD_KEY_W       28
#define KEYBOARD_HIT_COLOR   RGBVAL16(0xff,0x00,0x00)

const unsigned short keysw[] = {
  TAREA_XY,KEYBOARD_X,KEYBOARD_Y,
  TAREA_WH,KEYBOARD_KEY_W,KEYBOARD_KEY_H,
  TAREA_NEW_ROW,40,40,140,40,40,  
  TAREA_END};
   
const unsigned char keys[] = {
  1,2,ACTION_NONE,3,4
  }; 
 
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
extern void * emu_Malloc(int size);
extern int emu_LoadFile(char * filename, char * buf, int size);
extern void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index);
extern void emu_DrawScreen(unsigned char * VBuf, int width, int height, int stride);
extern void emu_DrawLine(unsigned char * VBuf, int width, int height, int line);
extern void emu_DrawVsync(void);

extern void emu_InitJoysticks(void);
extern int emu_SwapJoysticks(int statusOnly);
extern unsigned short emu_DebounceLocalKeys(void);
extern int emu_ReadKeys(void);
extern int emu_GetPad(void);

extern void emu_sndPlaySound(int chan, int volume, int freq);
extern void emu_sndInit();

#endif




