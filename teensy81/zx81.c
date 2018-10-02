
#include "z80.h"
#include "zx81rom.h"


#define WIDTH  320
#define HEIGHT 192
#define BORDER 32

#define CYCLES_PER_FRAME 50000//69888 //3500000/50

#define VMEM_PT 0x400c

/* the z80 state */
#ifdef ALT_Z80CORE
static struct Z80 z80;
#else
static Z80 z80;
#endif

/* the keyboard state and the memory */
static byte keyboard[ 9 ];
static byte memory[ 65536 ];
static byte * XBuf=0; 
static int zx80=0;


struct { unsigned char R,G,B; } Palette[16] = {
  {   0,   0,   0},
  { 255, 255, 255}
};

const byte map_qw[8][5] = {
    {224,29,27,6,25}, // vcxz<caps shift=Lshift>
    {10,22, 7, 9, 4}, // gfdsa
    {23,21, 8,26,20}, // trewq
    {34,33,32,31,30}, // 54321
    {35,36,37,38,39}, // 67890
    {28,24,12,18,19}, // yuiop
    {11,13,14,15,40}, // hjkl<enter>
    { 5,17,16,225,44}, // bnm <symbshift=RSHift> <space>
};

static char tapename[64]={0};
static const int kBuf[]={13,25,19,25,19,40}; //,21,40}; // LOAD "" (J shift p shift p, R ENTER) 
static const int tBuf[]={2,0,2,0,2,2};//200!,2,2};
static int kcount=0;
static int timeout=100;



void WrZ80(register word Addr,register byte Value)
{
  /* don't write to rom */
  if ( Addr >= 0x4000 )
  {
    memory[ Addr ] = Value;
  }
}

byte RdZ80(register word Addr)
{
  return memory[ Addr ];  
}

void OutZ80(register word Port,register byte Value)
{
}

byte InZ80(register word port)
{
  int i;
  
  /* any read where the 0th bit of the port is zero reads from the keyboard */
  if ( ( port & 1 ) == 0 )
  {
    /* get the keyboard row */
    port >>= 8;
    
    for ( i = 0; i < 8; i++ )
    {
      /* check the first zeroed bit to select the row */
      if ( ( port & 1 ) == 0 )
      {
        /* return the keyboard state for the row */
        return keyboard[ i ];
      }
      port >>= 1;
    }
  }    
}



/* fetches an opcode from memory */
byte z80_fetch( struct z80* z80, word a )
{
  /* opcodes fetched below 0x8000 are just read */
  if ( a < 0x8000 )
  {
    return memory[ a ];
  }

  /*
  opcodes fetched above 0x7fff are read modulo 0x8000 and as 0 if the 6th bit
  is reset
  */
  byte b = memory[ a & 0x7fff ];
  return ( b & 64 ) ? b : 0;
}

/* reads from memory */
byte z80_read( struct z80* z80, word a )
{
  return RdZ80(a);
}

/* writes to memory */
void z80_write( struct z80* z80, word a, byte b )
{
  WrZ80(a,b);
}

/* reads from a port */
byte z80_in( struct z80* z80, word a )
{
  return InZ80(a);
}

/* writes to a port */
void z80_out( struct z80* z80, word a, byte b )
{
  OutZ80(a,b);
}

void PatchZ80(register Z80 *R)
{
  // nothing to do
}




static void displayScreen(void) {
  int row, col,i,j;
  int y=0;
  int d_file = memory[ VMEM_PT ] | memory[ VMEM_PT + 1 ] << 8;
  byte *charset = &rom[0x1e00];
  for ( row = 0; row < 24; row++ )
  {
    memset( XBuf, 1, WIDTH*8 );    
    for ( col = 0; col < 32; col++ )
    {
      byte * dst=&XBuf[(col<<3)+BORDER];
      byte c = memory[ ++d_file ];
      if (c<64) {
         byte * ptr=&charset[(int)c<<3];
        for (j=0;j<8;j++)
        {
          byte b = *ptr++;
          for (i=0;i<8;i++)
          {
            if ( b & 128 )
            {
              *dst++=0;
            }
            else
            {
              *dst++=1;
            }
            b <<= 1;
          }
          dst+=(WIDTH-8);               
        }       
      }
      else {
        c &= 0x3F;
        byte * ptr=&charset[(int)c<<3];
        for (j=0;j<8;j++)
        {
          byte b = *ptr++;
          for (i=0;i<8;i++)
          {
            if ( b & 128 )
            {
              *dst++=1;
            }
            else
            {
              *dst++=0;
            }
            b <<= 1;
          }
          dst+=(WIDTH-8);               
        }       
      }

    }
    for (j=0;j<8;j++)
    {
      emu_DrawLine(&XBuf[j*WIDTH], WIDTH, HEIGHT, y++);   
    }
    /* skip the 0x76 at the end of the line */
    d_file++;
  }  

  emu_DrawVsync();    
}

static void updateKeyboard (void)
{
  int nb_keys=0;
  int k = emu_GetPad();
  int hk = emu_ReadI2CKeyboard();  
  if ( (k == 0) && (hk == 0) )  {
    memset(keyboard, 0xff, sizeof(keyboard));    
  }
  else {
    // scan all possibilities
    for (int j=0;j<8;j++) {
      for(int i=0;i<5;i++){
        if ( (k == map_qw[j][i]) || (hk == map_qw[j][i]) ) {
            keyboard[j] &= ~ (1<<(4-i));
            nb_keys++;
        }   
      }  
    }    
  } 
}

static void handleKeyBuf(void)
{
  if (timeout) {
    timeout--;
    if (timeout==0) {
      memset(keyboard, 0xff, sizeof(keyboard)); 
      emu_printf("key up");
    }
  }
  else {
    if (!(kcount == (sizeof(kBuf)/sizeof(int)))) {
      emu_printf("key dw");     
      timeout=tBuf[kcount];
      int k=kBuf[kcount++];
      // scan all possibilities
      for (int j=0;j<8;j++) {
        for(int i=0;i<5;i++){
          if ( (k == map_qw[j][i]) ) {
              keyboard[j] &= ~ (1<<(4-i));
          }   
        }  
      } 
      if (timeout == 0) {
        timeout=tBuf[kcount];
        int k=kBuf[kcount++];
        // scan all possibilities
        for (int j=0;j<8;j++) {
          for(int i=0;i<5;i++){
            if ( (k == map_qw[j][i]) ) {
                keyboard[j] &= ~ (1<<(4-i));
            }   
          }  
        }         
      }      
    }       
  }
}


void load_p(int a)
{
  emu_printf("loading tape");
  emu_printf(tapename);
  if ( emu_FileOpen(tapename) ) {
    int size = emu_FileSize();
    emu_FileRead(memory + (zx80?0x4000:0x4009), size);
    emu_FileClose();
  }      
}

static void hacks(void)
{
  byte *mem = memory;

  /* patch load routine */
  mem[0x347]=0xeb;
  mem[0x348]=0xed; mem[0x349]=0xfc;
  mem[0x34a]=0xc3; mem[0x34b]=0x07; mem[0x34c]=0x02;

  
#if UNUSED  
  /* remove fast mode stuff which screws it all up */
  mem[0x4cc]=mem[0x4cd]=mem[0x4ce]=0;

  /* replace kybd routine itself, with ld hl,(lastk):ret */
  mem[0x2bb]=0x2a; mem[0x2bc]=0x25; mem[0x2bd]=0x40; mem[0x2be]=0xc9;
  
  /* a 'force F/0' at 66h; ok, as we simulate NMI effects by other means */
  mem[0x66]=0xcf; mem[0x67]=0x0e;
  
  /* patch save routine */
  mem[0x2fc]=0xed; mem[0x2fd]=0xfd;
  mem[0x2fe]=0xc3; mem[0x2ff]=0x07; mem[0x300]=0x02;
  
  /* patch load routine */
  mem[0x347]=0xeb;
  mem[0x348]=0xed; mem[0x349]=0xfc;
  mem[0x34a]=0xc3; mem[0x34b]=0x07; mem[0x34c]=0x02;
  
  /* make fast/slow more traceable (directly modify bit 7 rather than
   * bit 6 of CDFLAGS)
   */
  mem[0xf29]=0xbe;
  mem[0xf2e]=0xfe;
  
  /* do 'out (0),a' when waiting for input (for fast mode display check) */
  mem[0x4ca]=0xd3; mem[0x4cb]=0;      /* out (0),a */
  mem[0x4cc]=0xcb; mem[0x4cd]=0x46;   /* loop: bit 0,(hl) */
  mem[0x4ce]=0x28; mem[0x4cf]=0xfc;   /* jr z,loop */
  mem[0x4d0]=0xd3; mem[0x4d1]=1;      /* out (1),a */
  mem[0x4d2]=0; /* nop, as filler */
  
  /* skip some more weird fast mode stuff */
  mem[0x2ec]=0xc9;
  
  /* we need the following crock to support 'PAUSE'... */
  
  mem[0xf3b]=0x29;  /* call 0229h */
  
  mem[0x229]=0x22;  /* ld (04034h),hl:nop */
  mem[0x22c]=0;
  
  mem[0x23e]=0x2a; mem[0x23f]=0x25; mem[0x240]=0x40;  /* ld hl,(0403bh) */
  mem[0x241]=0x23;          /* inc hl */
  mem[0x242]=0x3e; mem[0x243]=0x7f;     /* ld a,07fh */
  mem[0x244]=0xa4;          /* and h */
  mem[0x245]=0xb5;          /* or l */
  mem[0x246]=0xc0;          /* ret nz */
  
  mem[0x247]=0x2a;  /* ld hl,(04034h) */
  mem[0x248]=0x34;
  mem[0x249]=0x40;
  
  mem[0x24a]=0xc3;  /* jp 022dh */
  mem[0x24b]=0x2d;
  mem[0x24c]=0x02;
  
  /* nop out ld (04034h),hl */
  mem[0x23a]=0; mem[0x23a]=0; mem[0x23a]=0;
  
  /* nop out call to 207h */
  mem[0xf41]=0; mem[0xf42]=0; mem[0xf43]=0;

#endif  
}



void z81_Init(void) 
{
  if (XBuf == 0) XBuf = (byte *)emu_Malloc(WIDTH*8);
  /* Set up the palette */
  int J;
  for(J=0;J<2;J++)
    emu_SetPaletteEntry(Palette[J].R,Palette[J].G,Palette[J].B, J);

  /* load rom with ghosting at 0x2000 */
  memcpy( memory + 0x0000, rom, 0x2000  );
  hacks();
  /* patch DISPLAY-5 to a return */
  memory[ 0x02b5 + 0x0000 ] = 0xc9;
  memcpy( memory + 0x2000, memory, 0x2000 );

  /* reset the keyboard state */
  memset( keyboard, 255, sizeof( keyboard ) );  
  
#ifdef ALT_Z80CORE
  memset( &z80, 0, sizeof( z80 ) );
  /* setup the registers */
  z80.pc  = 0;
  z80.iff = 0;
  z80.af_sel = z80.regs_sel = 0;
#else
  ResetZ80(&z80, CYCLES_PER_FRAME);
#endif 
 }



void z81_Step(void)
{
  /*
  execute 100000 z80 instructions; the less instructions we execute here the
  slower the emulation gets, the more we execute the less responsive the
  keyboard gets
  */  
#ifdef ALT_Z80CORE
  int count;
  
  for ( count = 0; count < 100000; count++ )
  {
    z80_step( &z80 );
  }

#else
  ExecZ80(&z80,CYCLES_PER_FRAME); // 3.5MHz ticks for 6 lines @ 30 kHz = 700 cycles
  IntZ80(&z80,INT_IRQ); // must be called every 20ms
#endif  
  displayScreen();
  updateKeyboard();
  if (strlen(tapename)) handleKeyBuf();     
}


void z81_Start(char * filename)
{
  strncpy(tapename,filename,64);
}
