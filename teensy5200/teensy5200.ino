extern "C" {
  #include "emuapi.h"
}

#include "keyboard_osd.h"
#include "ili9341_t3dma.h"
#include <elapsedMillis.h>

extern "C" {
#include "atari5200.h"
}

#include <uVGA.h>
uVGA uvga;
#if F_CPU == 144000000
#define UVGA_144M_326X240
#define UVGA_XRES 326
#define UVGA_YRES 240
#define UVGA_XRES_EXTRA 10
#elif  F_CPU == 180000000
#define UVGA_180M_360X300
#define UVGA_XRES 360
#define UVGA_YRES 300
#define UVGA_XRES_EXTRA 8 
#elif  F_CPU == 240000000
#define UVGA_240M_452X240
#define UVGA_XRES 452
#define UVGA_YRES 240
#define UVGA_XRES_EXTRA 12 
#else
#error Please select F_CPU=240MHz or F_CPU=180MHz or F_CPU=144MHz
#endif

#include <uVGA_valid_settings.h>

bool vgaMode = false;
#ifdef DMA_FULL
uint8_t * VGA_frame_buffer;
#else
UVGA_STATIC_FRAME_BUFFER(uvga_fb);
uint8_t * VGA_frame_buffer = uvga_fb;
#endif


#define SCK             13
#define MISO            12
#define MOSI            11
#define TFT_TOUCH_CS    38
#define TFT_TOUCH_INT   37
#define TFT_DC          9
#define TFT_CS          10
#define TFT_RST         255  // 255 = unused, connected to 3.3V
#define TFT_SCLK        SCK
#define TFT_MOSI        MOSI
#define TFT_MISO        MISO

ILI9341_t3DMA tft = ILI9341_t3DMA(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO, TFT_TOUCH_CS, TFT_TOUCH_INT);
static unsigned char  palette8[PALETTE_SIZE];
static unsigned short palette16[PALETTE_SIZE];
static IntervalTimer myTimer;
volatile boolean vbl=true;
static int skip=0;
static elapsedMicros tius;


static void main_step() {
  uint16_t bClick = emu_DebounceLocalKeys();
  
  // Global key handling
  if (bClick & MASK_KEY_USER1) {  
    emu_printf((char*)"user1");
  }
  if (bClick & MASK_KEY_USER2) {  
    emu_printf((char*)"user2");
  }
  if (bClick & MASK_KEY_USER3) {  
    emu_printf((char*)"user3");
  }
  if (bClick & MASK_KEY_USER4) {  
    emu_printf((char*)"user4");
    emu_SwapJoysticks(0);    
  }  
  if (bClick & MASK_KEY_ESCAPE) {  
    emu_printf((char*)"esc");
    *(volatile uint32_t *)0xE000ED0C = 0x5FA0004;
    while (true) {
      ;
    }    
  }
     
  if (menuActive()) {
    int action = handleMenu(bClick);
    char * filename = menuSelection();
    if (action == ACTION_RUNTFT) {
      tft.setFrameBuffer((uint16_t *)malloc((ILI9341_TFTHEIGHT*ILI9341_TFTWIDTH)*sizeof(uint16_t)));
      Serial.print("TFT init: ");  
      Serial.println(tft.getFrameBuffer()?"ok":"ko");       
      toggleMenu(false);
      vgaMode = false;   
      tft.fillScreenNoDma( RGBVAL16(0x00,0x00,0x00) );
      tft.refresh();
      emu_Init(filename);
    }
    else if (action == ACTION_RUNVGA) {
      toggleMenu(false);
      vgaMode = true;
      tft.setFrameBuffer((uint16_t *)malloc((UVGA_YRES*(UVGA_XRES+UVGA_XRES_EXTRA))*sizeof(uint8_t)));
      VGA_frame_buffer = (uint8_t *)tft.getFrameBuffer();
      emu_Init(filename); 
      uvga.set_static_framebuffer(VGA_frame_buffer);
      int retvga = uvga.begin(&modeline);
      Serial.print("VGA init: ");  
      Serial.println(retvga);       
      uvga.clear(0x00);      
      //tft.start();
      tft.fillScreenNoDma( RGBVAL16(0x00,0x00,0x00) );
      // In VGA mode, we show the keyboard on TFT
      toggleVirtualkeyboard(true); // keepOn
      Serial.println("Starting");              
    }         
    delay(20);
  }
  else if (callibrationActive()) {
    handleCallibration(bClick);
  } 
  else {
    handleVirtualkeyboard();
    if ( (!virtualkeyboardIsActive()) || (vgaMode) ) {     
      emu_Step();
    }
  }
  //emu_printi(tius);
  //tius=0;
}

static void vblCount() { 
  if (vbl) {
    vbl = false;
  } else {
    vbl = true;
  }
#ifdef TIMER_REND
  main_step();
#endif
}




// ****************************************************
// the setup() method runs once, when the sketch starts
// ****************************************************
void setup() {
  tft.begin();
  tft.flipscreen(true);  
  tft.start();

  emu_init();  
  
#ifdef TIMER_REND
  myTimer.begin(vblCount, 20000);  //to run every 20ms
#else
  myTimer.begin(vblCount, 5000);  // will try to VSYNC next at 5ms
#endif 
}

// ****************************************************
// the loop() method runs continuously
// ****************************************************

void loop() {
#ifndef TIMER_REND  
  main_step();
#endif
}




void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index)
{
  if (index<PALETTE_SIZE) {
    //Serial.println("%d: %d %d %d\n", index, r,g,b);
    palette8[index]  = RGBVAL8(r,g,b);
    palette16[index] = RGBVAL16(r,g,b);    
  }
}

void emu_DrawVsync(void)
{
  skip += 1;
  skip &= VID_FRAME_SKIP;
/*
  volatile boolean vb=vbl;
  if (!vgaMode) {
    while (vbl==vb) {};
  }
  else {
    while (vbl==vb) {};
  }
*/  
}

void emu_DrawLine(unsigned char * VBuf, int width, int height, int line) 
{
  if (!vgaMode) {
    tft.writeLine(width,1,line, VBuf, palette16);
  }
  else {
    int fb_width=UVGA_XRES,fb_height=UVGA_YRES;
    fb_width += UVGA_XRES_EXTRA;
    int offx = (fb_width-width)/2;   
    int offy = (fb_height-height)/2+line;
    uint8_t * dst=VGA_frame_buffer+(fb_width*offy)+offx;    
    for (int i=0; i<width; i++)
    {
       uint8_t pixel = palette8[*VBuf++];
        *dst++=pixel;
    }
  }
}  

void emu_DrawScreen(unsigned char * VBuf, int width, int height, int stride) 
{
  if (!vgaMode) {
    if (skip==0) {
      tft.writeScreen(width,height-TFT_VBUFFER_YCROP,stride, VBuf+(TFT_VBUFFER_YCROP/2)*stride, palette16);
    }
  }
  else {
    int fb_width=UVGA_XRES, fb_height=UVGA_YRES;
    //uvga.get_frame_buffer_size(&fb_width, &fb_height);
    fb_width += UVGA_XRES_EXTRA;
    int offx = (fb_width-width)/2;   
    int offy = (fb_height-height)/2;
    uint8_t * buf=VGA_frame_buffer+(fb_width*offy)+offx;
    for (int y=0; y<height; y++)
    {
      uint8_t * dest = buf;
      for (int x=0; x<width; x++)
      {
        uint8_t pixel = palette8[*VBuf++];
        *dest++=pixel;
        //*dest++=pixel;
      }
      buf += fb_width;
      VBuf += (stride-width);
    }             
  }
}


void emu_resetus(void) {
  tius=0;  
}

int emu_us(void)  {
  return (tius);
}




#ifdef HAS_SND

#include <Audio.h>
#include "AudioPlaySystem.h"

AudioPlaySystem mymixer;
AudioOutputAnalog dac1;
AudioConnection   patchCord1(mymixer, dac1);

void emu_sndInit() {
  Serial.println("sound init");  

  AudioMemory(16);
  mymixer.start();
}

void emu_sndPlaySound(int chan, int volume, int freq)
{
  emu_printi(freq);
  if (chan < 6) {
    mymixer.sound(chan, freq, volume); 
  }
  
  /*
  Serial.print(chan);
  Serial.print(":" );  
  Serial.print(volume);  
  Serial.print(":" );  
  Serial.println(freq); 
  */ 
}

void emu_sndPlayBuzz(int size, int val) {
  mymixer.buzz(size,val); 
  //Serial.print((val==1)?1:0); 
  //Serial.print(":"); 
  //Serial.println(size); 
}

#endif





