extern "C" {
  #include "emuapi.h"
}

#include "keyboard_osd.h"
#include "ili9341_t3dma.h"

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
uint8_t * VGA_frame_buffer = (uint8_t *)ILI9341_t3DMA::getFrameBuffer();
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

void vblCount() { 
  if (vbl) {
    vbl = false;
  } else {
    vbl = true;
  }
}

// ****************************************************
// the setup() method runs once, when the sketch starts
// ****************************************************
void setup() {
  tft.begin();
  tft.start();

  emu_init(); 
  
  myTimer.begin(vblCount, 15000);  //to run every 20ms
}


// ****************************************************
// the loop() method runs continuously
// ****************************************************
void loop() {
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
    if ( (bClick & MASK_JOY2_BTN) || (action == ACTION_RUNTFT) ) {
      toggleMenu(false);
      vgaMode = false;   
      tft.fillScreenNoDma( RGBVAL16(0x00,0x00,0x00) );
      tft.refresh();
      emu_Init(filename);
    }
    else if ( (bClick & MASK_KEY_USER1)|| (action == ACTION_RUNVGA) )  {
      toggleMenu(false);
      vgaMode = true;
      emu_Init(filename); 
      uvga.set_static_framebuffer(VGA_frame_buffer);
      int retvga = uvga.begin(&modeline);
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
}





void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index)
{
//Serial.println("%d: %d %d %d\n", index, r,g,b);
  palette8[index]  = RGBVAL8(r,g,b);
  palette16[index] = RGBVAL16(r,g,b);
}

void emu_DrawVsync(void)
{
  volatile boolean vb=vbl;
  if (!vgaMode) {
    while (vbl==vb) {};
  }
  else {
    //Serial.println("r");
    while (vbl==vb) {};
  }
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

int skip=0;
void emu_DrawScreen(unsigned char * VBuf, int width, int height, int stride) 
{
  skip += 1;
  skip &= VID_FRAME_SKIP;
  if (skip==0) {
    boolean vb=vbl;
    if (!vgaMode) {
      tft.writeScreen(width,height-TFT_VBUFFER_YCROP,stride, VBuf+(TFT_VBUFFER_YCROP/2)*stride, palette16);     
    }
    else {
      int fb_width=UVGA_XRES, fb_height=UVGA_YRES;
      //uvga.get_frame_buffer_size(&fb_width, &fb_height);
      fb_width += UVGA_XRES_EXTRA;
      int offx = (fb_width-width*2)/2;   
      int offy = (fb_height-height)/2;
      uint8_t * buf=VGA_frame_buffer+fb_width*offy+offx;
      for (int y=0; y<height; y++)
      {
        uint8_t * dest = buf;
        for (int x=0; x<width; x++)
        {
          uint8_t pixel = palette8[*VBuf++];
          *dest++=pixel;
          *dest++=pixel;
        }
        buf += fb_width;
        VBuf += (stride-width);
      }           
    }
  }
}



#ifdef HAS_SND

#include <Audio.h>

AudioSynthWaveform waveform1;
AudioSynthWaveform waveform2;
AudioSynthWaveform waveform3;
AudioSynthNoiseWhite waveform4;
AudioMixer4 mixer1;
AudioConnection patchCord17(waveform1, 0, mixer1, 0);
AudioConnection patchCord18(waveform2, 0, mixer1, 1);
AudioConnection patchCord19(waveform3, 0, mixer1, 2);
AudioConnection patchCord20(waveform4, 0, mixer1, 3);
AudioOutputAnalog        dac0;
AudioConnection          patchCord1(mixer1, dac0);

boolean sndinit=false;

void emu_sndPlaySound(int chan, int volume, int freq)
{
  if (!sndinit) {
    emu_sndInit();
    sndinit=true;
  }
  double vol = (1.0*volume)/256.0;
  Serial.print(chan);
  Serial.print(":" );  
  Serial.println(freq);  
  switch (chan) {
    case 0:
      waveform1.amplitude(vol);
      waveform1.frequency(freq);
      break;
    case 1:
      waveform2.amplitude(vol);
      waveform2.frequency(freq);
      break;
    case 2:
      waveform3.amplitude(vol);
      waveform3.frequency(freq);
      break;
    case 3:
      waveform4.amplitude(vol);
      //waveform4.frequency(freq);
      break;
      
  }
}

void emu_sndInit() {
  Serial.println("sound init");  

  AudioMemory(2);
  int current_waveform = WAVEFORM_SQUARE;
  waveform1.begin(current_waveform);
  waveform2.begin(current_waveform);
  waveform3.begin(current_waveform);
  //waveform4.begin(current_waveform);
}

#endif

