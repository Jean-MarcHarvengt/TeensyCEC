#include "Z80.h"
#include "spectrum.rom.h"
#include "emuapi.h"
#include "zx_filetyp_z80.h"


#define WIDTH  320
#define HEIGHT 192

struct { unsigned char R,G,B; } Palette[16] = {
  {   0,   0,   0},
  {   0,   0, 205},
  { 205,   0,   0},
  { 205,   0, 205},
  {   0, 205,   0},
  {   0, 205, 205},
  { 205, 205,   0},
  { 212, 212, 212},
  {   0,   0,   0},
  {   0,   0, 255},
  { 255,   0,   0},
  { 255,   0, 255},
  {   0, 255,   0},
  {   0, 255, 255},
  { 255, 255,   0},
  { 255, 255, 255}
};

const uint8_t map_qw[8][5] = {
    {25, 6,27,29,224}, // vcxz<caps shift=Lshift>
    {10, 9, 7,22, 4}, // gfdsa
    {23,21, 8,26,20}, // trewq
    {34,33,32,31,30}, // 54321
    {35,36,37,38,39}, // 67890
    {28,24,12,18,19}, // yuiop
    {11,13,14,15,40}, // hjkl<enter>
    { 5,17,16,225,44}, // bnm <symbshift=RSHift> <space>
};

static uint8_t Z80_RAM[0xC000];                    // 48k RAM
static Z80 myCPU;
static uint8_t * volatile VRAM=Z80_RAM;            // What will be displayed. Generally ZX VRAM, can be changed for alt screens.

//extern const uint8_t rom_zx48_rom[];        // 16k ROM
static uint8_t key_ram[8]={
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}; // Keyboard buffer
uint8_t out_ram;                            // Output (fe port)
static uint8_t kempston_ram;                       // Kempston-Joystick Buffer
//static int fila[5][5];


static int v_border=0;
static int h_border=32;
static int bordercolor=0;
static byte * XBuf=0; 

void displayscanline2 (int y, int f_flash)
{
  int x, row, col, dir_p, dir_a, pixeles, tinta, papel, atributos;

  row = y + v_border;    // 4 & 32 = graphical screen offset
  col = 0;              // 32+256+32=320  4+192+4=200  (res=320x200)

  for (x = 0; x < h_border; x++) {
    XBuf[col++] = bordercolor;
  }

  dir_p = ((y & 0xC0) << 5) + ((y & 0x07) << 8) + ((y & 0x38) << 2);
  dir_a = 0x1800 + (32 * (y >> 3));
  
  for (x = 0; x < 32; x++)
  {
    pixeles=  VRAM[dir_p++];
    atributos=VRAM[dir_a++];
    
    if (((atributos & 0x80) == 0) || (f_flash == 0))
    {
      tinta = (atributos & 0x07) + ((atributos & 0x40) >> 3);
      papel = (atributos & 0x78) >> 3;
    }
    else
    {
      papel = (atributos & 0x07) + ((atributos & 0x40) >> 3);
      tinta = (atributos & 0x78) >> 3;
    }
    XBuf[col++] = ((pixeles & 0x80) ? tinta : papel);
    XBuf[col++] = ((pixeles & 0x40) ? tinta : papel);
    XBuf[col++] = ((pixeles & 0x20) ? tinta : papel);
    XBuf[col++] = ((pixeles & 0x10) ? tinta : papel);
    XBuf[col++] = ((pixeles & 0x08) ? tinta : papel);
    XBuf[col++] = ((pixeles & 0x04) ? tinta : papel);
    XBuf[col++] = ((pixeles & 0x02) ? tinta : papel);
    XBuf[col++] = ((pixeles & 0x01) ? tinta : papel);
  }

  for (x = 0; x < h_border; x++) {
    XBuf[col++] = bordercolor;
  }
  
  emu_DrawLine(XBuf, WIDTH, HEIGHT, y);
}

static void displayScreen(void) {
  int y;
  static int f_flash = 1, f_flash2 = 0;
  f_flash2 = (f_flash2++) % 32;
  if (f_flash2 < 16)
    f_flash = 1;
  else
    f_flash = 0;
  
  for (y = 0; y < HEIGHT; y++)
    displayscanline2 (y, f_flash);

  emu_DrawVsync();    
}



static void InitKeyboard(void){
  memset(key_ram, 0xff, sizeof(key_ram));   
}

static void UpdateKeyboard (void)
{
  int nb_keys=0;
  int k = emu_GetPad();
  int hk = emu_ReadI2CKeyboard();  
  if ( (k == 0) && (hk == 0) )  {
    memset(key_ram, 0xff, sizeof(key_ram));    
  }
  else {
    // scan all possibilities
    for (int j=0;j<8;j++) {
      for(int i=0;i<5;i++){
        if ( (k == map_qw[j][i]) || (hk == map_qw[j][i]) ) {
            key_ram[j] &= ~ (1<<(4-i));
            nb_keys++;
        }   
      }  
    }    
  } 
}

/*  
static void InitKeyboard2(void){
  fila[1][1] = fila[1][2] = fila[2][2] = fila[3][2] = fila[4][2] =
    fila[4][1] = fila[3][1] = fila[2][1] = 0xFF;
}

void UpdateKeyboard2 (void)
{
  fila[1][1] = fila[1][2] = fila[2][2] = fila[3][2] =
    fila[4][2] = fila[4][1] = fila[3][1] = fila[2][1] = 0xFF;

  unsigned char serial_in;
  serial_in = emu_GetPad();
  if (serial_in!=0)
  {
     if (serial_in=='Z')
      fila[4][1] &= (0xFD);
    if (serial_in=='X')
      fila[4][1] &= (0xFB);
    if (serial_in=='C')
      fila[4][1] &= (0xF7);
    if (serial_in=='V')
      fila[4][1] &= (0xEF);
    if (serial_in=='RSHIFT] || key[KEY_LSHIFT')
      fila[4][1] &= (0xFE);
  
    if (serial_in=='A')
      fila[3][1] &= (0xFE);
    if (serial_in=='S')
      fila[3][1] &= (0xFD);
    if (serial_in=='D')
      fila[3][1] &= (0xFB);
    if (serial_in=='F')
      fila[3][1] &= (0xF7);
    if (serial_in=='G')
      fila[3][1] &= (0xEF);
  
    if (serial_in=='Q')
      fila[2][1] &= (0xFE);
    if (serial_in=='W')
      fila[2][1] &= (0xFD);
    if (serial_in=='E')
      fila[2][1] &= (0xFB);
    if (serial_in=='R')
      fila[2][1] &= (0xF7);
    if (serial_in=='T')
      fila[2][1] &= (0xEF);
  
    if (serial_in=='1')
      fila[1][1] &= (0xFE);
    if (serial_in=='2')
      fila[1][1] &= (0xFD);
    if (serial_in=='3')
      fila[1][1] &= (0xFB);
    if (serial_in=='4')
      fila[1][1] &= (0xF7);
    if (serial_in=='5')
      fila[1][1] &= (0xEF);
  
    if (serial_in=='0')
      fila[1][2] &= (0xFE);
    if (serial_in=='9')
      fila[1][2] &= (0xFD);
    if (serial_in=='8')
      fila[1][2] &= (0xFB);
    if (serial_in=='7')
      fila[1][2] &= (0xF7);
    if (serial_in=='6')
      fila[1][2] &= (0xEF);
  
    if (serial_in=='P')
      fila[2][2] &= (0xFE);
    if (serial_in=='O')
      fila[2][2] &= (0xFD);
    if (serial_in=='I')
      fila[2][2] &= (0xFB);
    if (serial_in=='U')
      fila[2][2] &= (0xF7);
    if (serial_in=='Y')
      fila[2][2] &= (0xEF);  
    if (serial_in=='\r')
      fila[3][2] &= (0xFE);

  
    if (serial_in=='L')
      fila[3][2] &= (0xFD);
    if (serial_in=='K')
      fila[3][2] &= (0xFB);
    if (serial_in=='J')
      fila[3][2] &= (0xF7);
    if (serial_in=='H')
      fila[3][2] &= (0xEF);
  
    if (serial_in==' ')
      fila[4][2] &= (0xFE);
    if (serial_in=='ALT] || key[KEY_ALT')
      fila[4][2] &= (0xFD);
    if (serial_in=='M')
      fila[4][2] &= (0xFB);
    if (serial_in=='N')
      fila[4][2] &= (0xF7);
    if (serial_in=='B')
      fila[4][2] &= (0xEF);
  
    if (serial_in=='BACKSPACE')
      {
        fila[4][1] &= (0xFE);
        fila[1][2] &= (0xFE);
      }
    if (serial_in=='TAB')
      {
        fila[4][1] &= (0xFE);
        fila[4][2] &= (0xFD);
      }
    if (serial_in=='CAPSLOCK')
    {
      fila[1][1] &= (0xFD);
      fila[4][1] &= (0xFE);
    }
  
    if (serial_in=='UP')
    {
      fila[1][2] &= (0xF7);
      fila[4][1] &= (0xFE);
    }   
    if (serial_in=='DOWN')
    {
      fila[1][2] &= (0xEF);
      fila[4][1] &= (0xFE);
    }   
    if (serial_in=='LEFT')
    {
      fila[1][1] &= (0xEF);
      fila[4][1] &= (0xFE);
    }   
    if (serial_in=='RIGHT')
    {
      fila[1][2] &= (0xFB);
      fila[4][1] &= (0xFE);
    }   
  }
}
*/


#define gamesize 48000
unsigned char * game;


void spec_Start(char * filename) {
  game = emu_Malloc(gamesize);
  int size = emu_LoadFile(filename, game, gamesize);  
  memset(Z80_RAM, 0, 0xC000);
  ZX_ReadFromFlash_Z80(&myCPU, game,size); 
  JumpZ80(R->PC.W);
  emu_Free(game);
}


void spec_Init(void) {
  int J;
  /* Set up the palette */
  for(J=0;J<16;J++)
    emu_SetPaletteEntry(Palette[J].R,Palette[J].G,Palette[J].B, J);
  
  InitKeyboard();
  
  if (XBuf == 0) XBuf = (byte *)emu_Malloc(WIDTH);
  VRAM = Z80_RAM;
  memset(Z80_RAM, 0, sizeof(Z80_RAM));
  ResetZ80(&myCPU);  
}

void spec_Step(void) {
  
  //ExecZ80(&myCPU,3500*6/30); // 3.5MHz ticks for 6 lines @ 30 kHz = 700 cycles
 // ExecZ80(&myCPU,3500*192/30); // 3.5MHz ticks for 6 lines @ 30 kHz = 700 cycles
  ExecZ80(&myCPU,3500000/80); 
  IntZ80(&myCPU,INT_IRQ); // must be called every 20ms

  displayScreen();

  int k=emu_ReadKeys();
  
  kempston_ram = 0x00;
  if (k & MASK_JOY2_BTN)
          kempston_ram |= 0x10; //Fire
  if (k & MASK_JOY2_UP)
          kempston_ram |= 0x8; //Up
  if (k & MASK_JOY2_DOWN)
          kempston_ram |= 0x4; //Down
  if (k & MASK_JOY2_RIGHT)
          kempston_ram |= 0x2; //Right
  if (k & MASK_JOY2_LEFT)
          kempston_ram |= 0x1; //Left


  UpdateKeyboard();          
 
}






#define BASERAM 0x4000


void WrZ80(register word Addr,register byte Value)
{
  if (Addr >= BASERAM)
    Z80_RAM[Addr-BASERAM]=Value;
}

uint8_t RdZ80(register word Addr)
{
  if (Addr<BASERAM) 
    return rom_zx48_rom[Addr];
  else
    return Z80_RAM[Addr-BASERAM];
}

#define DAC_ADDR 0x40007428 // 2xu8 output of DAC.
void OutZ80(register uint16_t Port,register uint8_t Value)
{
    if (!(Port & 0x01)) {
      bordercolor = (Value & 0x07);
    }

    if((Port&0xFF)==0xFE) {
        out_ram=Value; // update it
    }

}
byte InZ80(register uint16_t port)
{  
    if((port&0xFF)==0x1F) {
        // kempston RAM
        return kempston_ram;
    }
    /* 
    if (!(port & 0x01))
    {    
      int code = 0xFF;      
      if (!(port & 0x0100))
        code &= fila[4][1];
      if (!(port & 0x0200))
        code &= fila[3][1];
      if (!(port & 0x0400))
        code &= fila[2][1];
      if (!(port & 0x0800))
        code &= fila[1][1];
      if (!(port & 0x1000))
        code &= fila[1][2];
      if (!(port & 0x2000))
        code &= fila[2][2];
      if (!(port & 0x4000))
        code &= fila[3][2];
      if (!(port & 0x8000))
        code &= fila[4][2];
      return(code);
    }
    */
    if ((port&0xFF)==0xFE) {
        switch(port>>8) {
            case 0xFE : return key_ram[0]; break;
            case 0xFD : return key_ram[1]; break;
            case 0xFB : return key_ram[2]; break;
            case 0xF7 : return key_ram[3]; break;
            case 0xEF : return key_ram[4]; break;
            case 0xDF : return key_ram[5]; break;
            case 0xBF : return key_ram[6]; break;
            case 0x7F : return key_ram[7]; break;
        }
    } 
    return 0xFF;
}



void PatchZ80(register Z80 *R)
{
  // nothing to do
}


word LoopZ80(register Z80 *R)
{
  // no interrupt triggered
  return INT_NONE;
}
