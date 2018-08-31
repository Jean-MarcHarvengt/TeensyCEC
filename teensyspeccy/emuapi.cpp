#define KEYMAP_PRESENT 1

extern "C" {
  #include "emuapi.h"
}

#include "ili9341_t3dma.h"
#include "logo.h"
#include "bmpjoy.h"
#include "bmpvbar.h"
#include "bmpvga.h"
#include "bmptft.h"
#include <SdFat.h>

extern ILI9341_t3DMA tft;
static SdFatSdio sd;
static File file;
static int16_t calMinX=-1,calMinY=-1,calMaxX=-1,calMaxY=-1;

#define CALIBRATION_FILE    "/cal.cfg"

#define MAX_FILENAME_SIZE   28
#define MAX_MENULINES       (MKEY_L9)
#define TEXT_HEIGHT         16
#define TEXT_WIDTH          8
#define MENU_FILE_XOFFSET   (6*TEXT_WIDTH)
#define MENU_FILE_YOFFSET   (2*TEXT_HEIGHT)
#define MENU_FILE_W         (MAX_FILENAME_SIZE*TEXT_WIDTH)
#define MENU_FILE_H         (MAX_MENULINES*TEXT_HEIGHT)
#define MENU_FILE_BGCOLOR   RGBVAL16(0x00,0x00,0x20)
#define MENU_JOYS_YOFFSET   (12*TEXT_HEIGHT)
#define MENU_VBAR_XOFFSET   (0*TEXT_WIDTH)
#define MENU_VBAR_YOFFSET   (MENU_FILE_YOFFSET)

#define MENU_TFT_XOFFSET    (MENU_FILE_XOFFSET+MENU_FILE_W+8)
#define MENU_TFT_YOFFSET    (MENU_VBAR_YOFFSET+32)
#define MENU_VGA_XOFFSET    (MENU_FILE_XOFFSET+MENU_FILE_W+8)
#define MENU_VGA_YOFFSET    (MENU_VBAR_YOFFSET+MENU_FILE_H-32-37)


#define MKEY_L1             1
#define MKEY_L2             2
#define MKEY_L3             3
#define MKEY_L4             4
#define MKEY_L5             5
#define MKEY_L6             6
#define MKEY_L7             7
#define MKEY_L8             8
#define MKEY_L9             9
#define MKEY_UP             20
#define MKEY_DOWN           21
#define MKEY_JOY            22
#define MKEY_TFT            23
#define MKEY_VGA            24

const unsigned short menutouchareas[] = {
  TAREA_XY,MENU_FILE_XOFFSET,MENU_FILE_YOFFSET,
  TAREA_WH,MENU_FILE_W, TEXT_HEIGHT,
  TAREA_NEW_COL,TEXT_HEIGHT,TEXT_HEIGHT,TEXT_HEIGHT,TEXT_HEIGHT,TEXT_HEIGHT,TEXT_HEIGHT,TEXT_HEIGHT,TEXT_HEIGHT,TEXT_HEIGHT,
  
  TAREA_XY,MENU_VBAR_XOFFSET,MENU_VBAR_YOFFSET,
  TAREA_WH,32,48,
  TAREA_NEW_COL, 72,72,8,40,

  TAREA_XY,MENU_TFT_XOFFSET,MENU_TFT_YOFFSET,
  TAREA_WH,32,37,
  TAREA_NEW_COL, 38,38,
    
  TAREA_END};
   
const unsigned char menutouchactions[] = {
  MKEY_L1,MKEY_L2,MKEY_L3,MKEY_L4,MKEY_L5,MKEY_L6,MKEY_L7,MKEY_L8,MKEY_L9,
  MKEY_UP,MKEY_DOWN,ACTION_NONE,MKEY_JOY,
  MKEY_TFT,MKEY_VGA}; 

  
static bool menuOn=true;
static bool callibrationOn=false;
static int callibrationStep=0;
static bool menuRedraw=true;
static int nbFiles=0;
static int curFile=0;
static int topFile=0;
static char selection[MAX_FILENAME_SIZE+1]="";
static uint8_t prev_zt=0; 

static int readNbFiles(void) {
  int totalFiles = 0;
  File entry;  
  file = sd.open(ROMSDIR);
  while (true) {
      entry = file.openNextFile();
      if (! entry) {
          // no more files
          break;
      }
      if (!entry.isDirectory()) {
          totalFiles++;
      }
      entry.close();
  }
  file.close();
  return totalFiles;  
}

static char captureTouchZone(const unsigned short * areas, const unsigned char * actions, int *rx, int *ry, int *rw, int * rh) {
    uint16_t xt=0;
    uint16_t yt=0;
    uint16_t zt=0;
    boolean hDir=true;  
  
    if (tft.isTouching())
    {
        if (prev_zt == 0) {
            prev_zt =1;
            tft.readCal(&xt,&yt,&zt);
            int i=0;
            int k=0;
            int y2=0, y1=0;
            int x2=0, x1=0;
            int x=KEYBOARD_X,y=KEYBOARD_Y;
            int w=TAREA_W_DEF,h=TAREA_H_DEF;
            uint8_t s;
            while ( (s=areas[i++]) != TAREA_END ) {
                if (s == TAREA_XY) {
                    x = areas[i++];
                    y = areas[i++];                    
                    x2 = x;
                    y2 = y;  
                }
                else if (s == TAREA_WH) {
                    w = areas[i++];
                    h = areas[i++];
                }                     
                else if (s == TAREA_NEW_ROW) {
                  hDir = true;
                  y1 = y2;
                  y2 = y1 + h;
                  x2 = x;
                }  
                else if (s == TAREA_NEW_COL) {
                  hDir = false;
                  x1 = x2;
                  x2 = x1 + w;
                  y2 = y;                  
                }
                else { 
                    if (hDir) {
                      x1 = x2;
                      x2 = x1+s;                                                            
                    } else {
                      y1 = y2;
                      y2 = y1+s;                      
                    }
                    if ( (yt >= y1) && (yt < y2) && (xt >= x1) && (xt < x2)  ) {
                        *rx = x1;
                        *ry = y1;
                        *rw = x2-x1;
                        *rh = y2-y1;
                        return (actions[k]);  
                    }
                    k++;
                }                
            }
        } 
        prev_zt =1; 
    } else {
        prev_zt=0; 
    } 
  
    return ACTION_NONE;   
} 

void toggleMenu(bool on) {
  if (on) {
    callibrationOn=false;
    menuOn=true;
    menuRedraw=true;  
    tft.fillScreenNoDma(RGBVAL16(0x00,0x00,0x00));
    tft.drawTextNoDma(0,0, TITLE, RGBVAL16(0x00,0xff,0xff), RGBVAL16(0x00,0x00,0xff), true);  
    tft.drawSpriteNoDma(MENU_VBAR_XOFFSET,MENU_VBAR_YOFFSET,(uint16_t*)bmpvbar);
    tft.drawSpriteNoDma(MENU_TFT_XOFFSET,MENU_TFT_YOFFSET,(uint16_t*)bmptft);
    tft.drawSpriteNoDma(MENU_VGA_XOFFSET,MENU_VGA_YOFFSET,(uint16_t*)bmpvga);
  } else {
    menuOn = false;    
  }
}


static void callibrationInit(void) 
{
  callibrationOn=true;
  menuOn=false;
  callibrationStep = 0;
  calMinX=0,calMinY=0,calMaxX=0,calMaxY=0;
  tft.fillScreenNoDma(RGBVAL16(0xff,0xff,0xff));
  tft.drawTextNoDma(0,100, "          Callibration process:", RGBVAL16(0x00,0x00,0x00), RGBVAL16(0xff,0xff,0xff), true);
  tft.drawTextNoDma(0,116, "     Hit the red cross at each corner", RGBVAL16(0x00,0x00,0x00), RGBVAL16(0xff,0xff,0xff), true);
  tft.drawTextNoDma(0,0, "+", RGBVAL16(0xff,0x00,0x00), RGBVAL16(0xff,0xff,0xff), true);
  prev_zt = 1;  
}

static void readCallibration(void) 
{
  char fileBuffer[64];
  File file(CALIBRATION_FILE, O_READ);
  if (file.isOpen()) {
    if ( file.read(fileBuffer, 64) ) {
      sscanf(fileBuffer,"%d %d %d %d", &calMinX,&calMinY,&calMaxX,&calMaxY);
    }
    file.close();
    Serial.println("Current callibration params:");
    Serial.println(calMinX);
    Serial.println(calMinY);
    Serial.println(calMaxX);
    Serial.println(calMaxY);                 
  }
  else {
    Serial.println("Callibration read error");
  }  
  tft.callibrateTouch(calMinX,calMinY,calMaxX,calMaxY);   
}

static void writeCallibration(void) 
{
  tft.callibrateTouch(calMinX,calMinY,calMaxX,calMaxY);
  File file = sd.open(CALIBRATION_FILE, O_WRITE | O_CREAT | O_TRUNC);
  if (file.isOpen()) {
    file.print(calMinX);
    file.print(" ");
    file.print(calMinY);
    file.print(" ");
    file.print(calMaxX);
    file.print(" ");
    file.println(calMaxY);
    file.close();
  }
  else {
    Serial.println("Callibration write error");
  }  
}


bool callibrationActive(void) 
{
  return (callibrationOn);
}



int handleCallibration(uint16_t bClick) {
  uint16_t xt=0;
  uint16_t yt=0;
  uint16_t zt=0;  
  if (tft.isTouching()) {
    if (prev_zt == 0) {
      prev_zt = 1;
      tft.readRaw(&xt,&yt,&zt);
      switch (callibrationStep) 
      {
        case 0:
          callibrationStep++;
          tft.drawTextNoDma(0,0, " ", RGBVAL16(0xff,0xff,0xff), RGBVAL16(0xff,0xff,0xff), true);
          tft.drawTextNoDma(ILI9341_TFTREALWIDTH-8,0, "+", RGBVAL16(0xff,0x00,0x00), RGBVAL16(0xff,0xff,0xff), true);
          calMinX += xt;
          calMinY += yt;          
          break;
        case 1:
          callibrationStep++;
          tft.drawTextNoDma(ILI9341_TFTREALWIDTH-8,0, " ", RGBVAL16(0xff,0xff,0xff), RGBVAL16(0xff,0xff,0xff), true);
          tft.drawTextNoDma(ILI9341_TFTREALWIDTH-8,ILI9341_TFTREALHEIGHT-16, "+", RGBVAL16(0xff,0x00,0x00), RGBVAL16(0xff,0xff,0xff), true);
          calMaxX += xt;
          calMinY += yt;           
          break;
        case 2:
          callibrationStep++;
          tft.drawTextNoDma(ILI9341_TFTREALWIDTH-8,ILI9341_TFTREALHEIGHT-16, " ", RGBVAL16(0xff,0xff,0xff), RGBVAL16(0xff,0xff,0xff), true);
          tft.drawTextNoDma(0,ILI9341_TFTREALHEIGHT-16, "+", RGBVAL16(0xff,0x00,0x00), RGBVAL16(0xff,0xff,0xff), true);
          calMaxX += xt;
          calMaxY += yt;
          break;
        case 3:
          tft.fillScreenNoDma(RGBVAL16(0xff,0xff,0xff));
          tft.drawTextNoDma(0,100, "          Callibration done!", RGBVAL16(0x00,0x00,0x00), RGBVAL16(0xff,0xff,0xff), true);
          tft.drawTextNoDma(0,116, "        (Click center to exit)", RGBVAL16(0xff,0x00,0x00), RGBVAL16(0xff,0xff,0xff), true);           
          callibrationStep++;
          calMinX += xt;
          calMaxY += yt;       
          break;                 
        case 4:
          //Serial.println(xt);
          //Serial.println(yt);
          if ( (xt > (ILI9341_TFTREALWIDTH/4)) && (xt < (ILI9341_TFTREALWIDTH*3)/4) 
            && (yt > (ILI9341_TFTREALHEIGHT/4)) && (yt < (ILI9341_TFTREALHEIGHT*3)/4) ) {
            calMinX /= 2;
            calMinY /= 2;
            calMaxX /= 2;
            calMaxY /= 2;
            writeCallibration();                       
            toggleMenu(true);
          }
          else {
            callibrationInit();              
          }
          break; 
                           
      }
      delay(100);
    }  
  }
  else {
    prev_zt = 0;
  }  
}



bool menuActive(void) 
{
  return (menuOn);
}

int handleMenu(uint16_t bClick)
{
  int action = ACTION_NONE;
  
  int rx=0,ry=0,rw=0,rh=0;
  char c = captureTouchZone(menutouchareas, menutouchactions, &rx,&ry,&rw,&rh);
  if ( (c >= MKEY_L1) && (c <= MKEY_L9) ) {
   
    if ( (topFile+(int)c-1) <= (nbFiles-1)  )
    {
      curFile = topFile + (int)c -1;
      menuRedraw=true;
     //tft.drawRectNoDma( rx,ry,rw,rh, KEYBOARD_HIT_COLOR );
    }
  }
  else if (c == MKEY_TFT) {
      menuRedraw=true;
      action = ACTION_RUNTFT;    
  }
  else if (c == MKEY_VGA) {
      menuRedraw=true;
      action = ACTION_RUNVGA;    
  }  
  else if ( (bClick & MASK_JOY2_UP) || (c == MKEY_UP) ) {
    if (curFile!=0) {
      menuRedraw=true;
      curFile--;
    }
  }
  else if ( (bClick & MASK_JOY2_DOWN) || (c == MKEY_DOWN) ) {
    if ((curFile<(nbFiles-1)) && (nbFiles)) {
      curFile++;
      menuRedraw=true;
    }
  }
  else if ( (bClick & MASK_JOY2_RIGHT) || (bClick & MASK_JOY2_LEFT) || (c == MKEY_JOY) ) {
    emu_SwapJoysticks(0);
    menuRedraw=true;  
  }   
    
  if (menuRedraw && nbFiles) {
         
    int fileIndex = 0;
    char filename[MAX_FILENAME_SIZE+1];
    File entry;    
    file = sd.open(ROMSDIR); 
    tft.drawRectNoDma(MENU_FILE_XOFFSET,MENU_FILE_YOFFSET, MENU_FILE_W, MENU_FILE_H, MENU_FILE_BGCOLOR);
//    if (curFile <= (MAX_MENULINES/2-1)) topFile=0;
//    else topFile=curFile-(MAX_MENULINES/2);
    if (curFile <= (MAX_MENULINES-1)) topFile=0;
    else topFile=curFile-(MAX_MENULINES/2);

    //Serial.print("curfile: ");
    //Serial.println(curFile);
    //Serial.print("topFile: ");
    //Serial.println(topFile);
    
    int i=0;
    while (i<MAX_MENULINES) {
      entry = file.openNextFile();
      if (!entry) {
          // no more files
          break;
      }
      if (!entry.isDirectory()) {
        if (fileIndex >= topFile) {
          entry.getName(&filename[0], MAX_FILENAME_SIZE);            
          if ((i+topFile) < nbFiles ) {
            if ((i+topFile)==curFile) {
              tft.drawTextNoDma(MENU_FILE_XOFFSET,i*TEXT_HEIGHT+MENU_FILE_YOFFSET, filename, RGBVAL16(0xff,0xff,0x00), RGBVAL16(0xff,0x00,0x00), true);
              strcpy(selection,filename);            
            }
            else {
              tft.drawTextNoDma(MENU_FILE_XOFFSET,i*TEXT_HEIGHT+MENU_FILE_YOFFSET, filename, 0xFFFF, 0x0000, true);      
            }
          }
          i++; 
        }
        fileIndex++;    
      }
      entry.close();
    }

    tft.drawSpriteNoDma(0,MENU_JOYS_YOFFSET,(uint16_t*)bmpjoy);  
    tft.drawTextNoDma(48,MENU_JOYS_YOFFSET+8, (emu_SwapJoysticks(1)?(char*)"SWAP=1":(char*)"SWAP=0"), RGBVAL16(0x00,0xff,0xff), RGBVAL16(0xff,0x00,0x00), false);
    file.close();
    menuRedraw=false;     
  }

  return (action);  
}

char * menuSelection(void)
{
  return (selection);  
}
  

 

void emu_init(void)
{
  Serial.begin(9600);
  //while (!Serial) {}

  if (!sd.begin()) {
    emu_printf("SdFat.begin() failed");
  }
  nbFiles = readNbFiles(); 

  Serial.print("SD initialized, files found: ");
  Serial.println(nbFiles);

  emu_InitJoysticks();
  readCallibration();
 
  if (tft.isTouching()) {
    callibrationInit();
  } else  {
    toggleMenu(true);
  }
 
}


void emu_printf(char * text)
{
  Serial.println(text);
}

void emu_printf(int val)
{
  Serial.println(val);
}

void * emu_Malloc(int size)
{
  void * retval =  malloc(size);
  if (!retval) {
    emu_printf("failled to allocate ");
    emu_printf(size);
  }
  else {
    emu_printf("could allocate ");
    emu_printf(size);    
  }
  
  return retval;
}

void emu_Free(void * pt)
{
  free(pt);
}

int emu_LoadFile(char * filename, char * buf, int size)
{
  int filesize = 0;
    
  emu_printf("Loading ");
  emu_printf(filename);
  String filepath = String(ROMSDIR) + String("/") + String(filename);
  if (file.open(filepath.c_str(), O_READ)) 
  {
    filesize = file.fileSize(); 
    emu_printf(filesize);
    if (size >= filesize)
    {
      if (file.read(buf, filesize) != filesize) {
        emu_printf("File read failed");
      }        
    }
    file.close();
  }
  
  return(filesize);
}

int emu_LoadFileSeek(char * filename, char * buf, int size, int seek)
{
  int filesize = 0;
    
  //emu_printf("Loading ");
  emu_printf(filename);
  String filepath = String(ROMSDIR) + String("/") + String(filename);
  if (file.open(filepath.c_str(), O_READ)) 
  {
    file.seek(seek);
    //filesize = file.fileSize(); 
    //emu_printf(filesize);
    emu_printf(size);
    //emu_printf(seek);
    //if (size >= filesize)
    //{
      if (file.read(buf, size) != size) {
        emu_printf("File read failed");
      }        
    //}
    file.close();
  }
  
  return(filesize);
}

static int keypadval=0; 
static boolean joySwapped = false;
static uint16_t bLastState;
static int xRef;
static int yRef;
const int range = 12;               // output range of X or Y movement
const int threshold = range / 4;    // resting threshold
const int center = range / 2;       // resting position value

static int readAxis(int thisAxis, int max) {
  //analogReadResolution(11);
  // read the analog input:
  int reading = analogRead(thisAxis);
  //Serial.print(reading);
  if(max<0) {
    max = -max;
    reading = max-reading;
  }

  // map the reading from the analog input range to the output range:
  reading = map(reading, 0, max, 0, range);

  // if the output reading is outside from the
  // rest position threshold,  use it:
  int distance = reading - center;

  if (abs(distance) < threshold) {
    distance = 0;
  }

  //Serial.print(",");
  //Serial.println(distance);

  // return the distance for this axis:
  return distance;
}

static uint16_t readAnalogJoystick(void)
{
  uint16_t joysval = 0;

  int xReading = readAxis(PIN_JOY2_A1X, xRef*2);
  if (xReading > 0) joysval |= MASK_JOY2_LEFT;
  else if (xReading < 0) joysval |= MASK_JOY2_RIGHT;
  int yReading = readAxis(PIN_JOY2_A2Y, -yRef*2);
  if (yReading < 0) joysval |= MASK_JOY2_UP;
  else if (yReading > 0) joysval |= MASK_JOY2_DOWN;
  joysval |= (digitalRead(PIN_JOY2_BTN) == HIGH ? 0 : MASK_JOY2_BTN);

  return (joysval);     
}


int emu_SwapJoysticks(int statusOnly) {
  if (!statusOnly) {
    if (joySwapped) {
      joySwapped = false;
    }
    else {
      joySwapped = true;
    }
  }
  return(joySwapped?1:0);
}

unsigned short emu_DebounceLocalKeys(void)
{
  uint16_t bCurState = readAnalogJoystick();
  bCurState |= (digitalRead(PIN_KEY_ESCAPE) == HIGH ? 0 : MASK_KEY_ESCAPE);
  bCurState |= (digitalRead(PIN_KEY_USER1) == HIGH ? 0 : MASK_KEY_USER1);
  bCurState |= (digitalRead(PIN_KEY_USER2) == HIGH ? 0 : MASK_KEY_USER2);
  bCurState |= (digitalRead(PIN_KEY_USER3) == HIGH ? 0 : MASK_KEY_USER3);
  bCurState |= (digitalRead(PIN_KEY_USER4) == HIGH ? 0 : MASK_KEY_USER4);

  uint16_t bClick = bCurState & ~bLastState;
  bLastState = bCurState;

  return (bClick);
}

int emu_GetPad(void) 
{
  return(keypadval|((joySwapped?1:0)<<7));
}

int emu_ReadKeys(void) 
{
  uint16_t retval;
  uint16_t j1 = readAnalogJoystick();
  uint16_t j2 = 0;
  // Second joystick
  if ( digitalRead(PIN_JOY1_1) == LOW ) j2 |= MASK_JOY2_UP;
  if ( digitalRead(PIN_JOY1_2) == LOW ) j2 |= MASK_JOY2_DOWN;
  if ( digitalRead(PIN_JOY1_3) == LOW ) j2 |= MASK_JOY2_RIGHT;
  if ( digitalRead(PIN_JOY1_4) == LOW ) j2 |= MASK_JOY2_LEFT;
  if ( digitalRead(PIN_JOY1_BTN) == LOW ) j2 |= MASK_JOY2_BTN;

  if (joySwapped) {
    retval = ((j1 << 8) | j2);
  }
  else {
    retval = ((j2 << 8) | j1);
  }
  
  if ( digitalRead(PIN_KEY_USER1) == LOW ) retval |= MASK_KEY_USER1;
  if ( digitalRead(PIN_KEY_USER2) == LOW ) retval |= MASK_KEY_USER2;
  if ( digitalRead(PIN_KEY_USER3) == LOW ) retval |= MASK_KEY_USER3;

  return (retval);
}


void emu_InitJoysticks(void) {    
  pinMode(PIN_JOY1_1, INPUT_PULLUP);
  pinMode(PIN_JOY1_2, INPUT_PULLUP);
  pinMode(PIN_JOY1_3, INPUT_PULLUP);
  pinMode(PIN_JOY1_4, INPUT_PULLUP);
  pinMode(PIN_JOY1_BTN, INPUT_PULLUP);

  pinMode(PIN_KEY_USER1, INPUT_PULLUP);
  pinMode(PIN_KEY_USER2, INPUT_PULLUP);
  pinMode(PIN_KEY_USER3, INPUT_PULLUP);
  pinMode(PIN_KEY_USER4, INPUT_PULLUP);
  pinMode(PIN_KEY_ESCAPE, INPUT_PULLUP);
  pinMode(PIN_JOY2_BTN, INPUT_PULLUP);
  xRef = analogRead(PIN_JOY2_A1X);
  yRef = analogRead(PIN_JOY2_A2Y);  
}



static bool vkbKeepOn = false;
static bool vkbActive = false;
static bool vkeyRefresh=false;
static bool exitVkbd = false;
static uint8_t keyPressCount=0; 


bool virtualkeyboardIsActive(void) {
    return (vkbActive);
}

void toggleVirtualkeyboard(bool keepOn) {     
    if (keepOn) {      
        tft.drawSpriteNoDma(0,0,(uint16_t*)logo);
        //prev_zt = 0;
        vkbKeepOn = true;
        vkbActive = true;
        exitVkbd = false;  
    }
    else {
        vkbKeepOn = false;
        if ( (vkbActive) /*|| (exitVkbd)*/ ) {
            tft.fillScreenNoDma( RGBVAL16(0x00,0x00,0x00) );
#ifdef DMA_FULL
            tft.begin();
            tft.refresh();
#endif                        
            //prev_zt = 0; 
            vkbActive = false;
            exitVkbd = false;
        }
        else {
#ifdef DMA_FULL          
            tft.stop();
            tft.begin();      
            tft.start();
#endif                       
            tft.drawSpriteNoDma(0,0,(uint16_t*)logo);           
            //prev_zt = 0;
            vkbActive = true;
            exitVkbd = false;
        }
    }   
}


void handleVirtualkeyboard() {
    if (keyPressCount == 0) {
      keypadval = 0;      
    } else {
      keyPressCount--;
    }

    if ( (!virtualkeyboardIsActive()) && (tft.isTouching()) ) {
        toggleVirtualkeyboard(false);
        return;
    }
    
    int rx=0,ry=0,rw=0,rh=0;
    if ( (vkbKeepOn) || (virtualkeyboardIsActive())  ) {
        char c = captureTouchZone(keysw, keys, &rx,&ry,&rw,&rh);
        if (c) {
            tft.drawRectNoDma( rx,ry,rw,rh, KEYBOARD_HIT_COLOR );
            if ( (c >=1) && (c <= ACTION_MAXKBDVAL) ) {
              keypadval = c;
              keyPressCount = 5;
              delay(50);
              vkeyRefresh = true;
              exitVkbd = true;
            }
            else if (c == ACTION_EXITKBD) {
              vkeyRefresh = true;
              exitVkbd = true;  
            }
        }   
     }    
     
    if (vkeyRefresh) {
        vkeyRefresh = false;
        tft.drawSpriteNoDma(0,0,(uint16_t*)logo, rx, ry, rw, rh);
    }  
         
    if ( (exitVkbd) && (vkbActive) ) {      
        if (!vkbKeepOn) {             
            toggleVirtualkeyboard(false);
        }
        else {         
            toggleVirtualkeyboard(true);           
        } 
    }
          
}



