
#include "z80.h"
#include "zx80rom.h"
#include "zx81rom.h"
#include "emuapi.h"
#include "common.h"
#include "AY8910.h"


static AY8910 ay;
byte mem[ 65536 ];
#ifdef ALT_Z80CORE
unsigned char *memptr[64];
int memattr[64];
int unexpanded=0;
int nmigen=0,hsyncgen=0,vsync=0;
int vsync_visuals=0;
volatile int signal_int_flag=0;
unsigned char *iptr=mem;
int interrupted=0;
#else
static Z80 z80;
#endif

/* the keyboard state and other */
static byte keyboard[ 9 ] = {0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff};;
static byte * XBuf=0; 
int zx80=0;
int autoload=1;


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
    mem[ Addr ] = Value;
  }
}

byte RdZ80(register word Addr)
{
  return mem[ Addr ];  
}

void OutZ80(register word Port,register byte Value)
{
}

byte InZ80(register word port)
{
  byte h = port >> 8;
  byte l = port & 0xff;
  
  if(l==0xfe) { /* keyboard */
    switch(h)
    {
      case 0xfe:  return(keyboard[0]);
      case 0xfd:  return(keyboard[1]);
      case 0xfb:  return(keyboard[2]);
      case 0xf7:  return(keyboard[3]);
      case 0xef:  return(keyboard[4]);
      case 0xdf:  return(keyboard[5]);
      case 0xbf:  return(keyboard[6]);
      case 0x7f:  return(keyboard[7]);
      default:  return(255);
    }    
  }
  return(255);
}


#ifdef ALT_Z80CORE
#else
void PatchZ80(register Z80 *R)
{
  // nothing to do
}

#endif


unsigned int in(int h, int l)

{

  int ts=0;    /* additional cycles*256 */
  int tapezeromask=0x80;  /* = 0x80 if no tape noise (?) */

  if(!(l&4)) l=0xfb;
  if(!(l&1)) l=0xfe;

  switch(l)
  {
  //case 0xfb:
  //  return(printer_inout(0,0));
    
  case 0xfe:
    /* also disables hsync/vsync if nmi off
     * (yes, vsync requires nmi off too, Flight Simulation confirms this)
     */
    if(!nmigen)
    {
      hsyncgen=0;

      /* if vsync was on before, record position */
      if(!vsync)
        vsync_raise();
      vsync=1;

    }

    switch(h)
    {
        case 0xfe:  return(ts|(keyboard[0]^tapezeromask));
        case 0xfd:  return(ts|(keyboard[1]^tapezeromask));
        case 0xfb:  return(ts|(keyboard[2]^tapezeromask));
        case 0xf7:  return(ts|(keyboard[3]^tapezeromask));
        case 0xef:  return(ts|(keyboard[4]^tapezeromask));
        case 0xdf:  return(ts|(keyboard[5]^tapezeromask));
        case 0xbf:  return(ts|(keyboard[6]^tapezeromask));
        case 0x7f:  return(ts|(keyboard[7]^tapezeromask));
        default:
          {
            int i,mask,retval=0xff;
          
            /* some games (e.g. ZX Galaxians) do smart-arse things
            * like zero more than one bit. What we have to do to
            * support this is AND together any for which the corresponding
            * bit is zero.
            */
            for(i=0,mask=1;i<8;i++,mask<<=1)
              if(!(h&mask))
                retval&=keyboard[i];
            return(ts|(retval^tapezeromask));
          }
    }
    break;
  }

  return(ts|255);
}

unsigned int out(int h,int l,int a)

{
  /* either in from fe or out to ff takes one extra cycle;
   * experimentation strongly suggests not only that out to
   * ff takes one extra, but that *all* outs do.
   */
  int ts=1;  /* additional cycles */



  /* the examples in the manual (using DF/0F) and the
   * documented ports (CF/0F) don't match, so decode is
   * important for that.
   */
  if(!(l&0xf0))   /* not sure how many needed, so assume all 4 */
    l=0x0f;
  else
    if(!(l&0x20))   /* bit 5 low is common to DF and CF */
      l=0xdf;


  if(!(l&4)) l=0xfb;
  if(!(l&2)) l=0xfd;
  if(!(l&1)) l=0xfe;


  switch(l)
  {
    case 0x0f:    /* Zon X data */   
      WrData8910(&ay,a);
      break;
    case 0xdf:    /* Zon X reg. select */  
      WrCtrl8910(&ay,(a &0x0F));
      break;
  
    case 0xfb:
      return(ts/*|printer_inout(1,a)*/);
    case 0xfd:
      nmigen=0;
      break;
    case 0xfe:
      if(!zx80)
      {
        nmigen=1;
        break;
      }
      /* falls through, if zx80 */
    case 0xff:  /* XXX should *any* out turn off vsync? */
      /* fill screen gap since last raising of vsync */
      if(vsync)
        vsync_lower();
      vsync=0;
      hsyncgen=1;
      break;
  }

  return(ts);
}







void sighandler(int a)
{
  signal_int_flag=1;
}

void frame_pause(void)
{
  signal_int_flag=0;

  if(interrupted<2)
    interrupted=1;
}

void do_interrupt()
{
  /* being careful here not to screw up any pending reset... */
  if(interrupted==1)
    interrupted=0;
}





void bitbufBlit(unsigned char * buf)
{
  memset( XBuf, 1, WIDTH*8 ); 
  buf = buf + (ZX_VID_MARGIN*(ZX_VID_FULLWIDTH/8));
  int y,x,i;
  byte d;
  for(y=0;y<192;y++)
  {
    byte * src = buf + 4;
    for(x=0;x<32;x++)
    {
      byte * dst=&XBuf[(x<<3)+BORDER];
      d = *src++;
      for (i=0;i<8;i++)
      {
        if ( d & 128 )
        {
          *dst++=0;
        }
        else
        {
          *dst++=1;
        }
        d <<= 1;
      }       
    }
    emu_DrawLine(&XBuf[0], WIDTH, HEIGHT, y);   
    buf += (ZX_VID_FULLWIDTH/8);
  }
}


static void displayScreen(void) {
  int row,i,j,x,y=0;
  byte * ptr;

  ptr=mem+fetch2(16396);
  /* since we can't just do "don't bother if it's junk" as we always
   * need to draw a screen, just draw *valid* junk that won't result
   * in a segfault or anything. :-)
   */
  if(ptr-mem<0 || ptr-mem>0xf000) ptr=mem+0xf000;
  ptr++;  /* skip first HALT */
  byte *charset = &zx81rom[0x1e00];
  for ( row = 0; row < 24; row++ )
  {
    memset( XBuf, 1, WIDTH*8 );    
    for ( x = 0; x < 32; x++ )
    {
      byte * dst=&XBuf[(x<<3)+BORDER];
      byte c = *ptr++;
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
    ptr++;
  }      
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

/* despite the name, this also works for the ZX80 :-) */
void reset81()
{
  interrupted=2;  /* will cause a reset */
  memset(mem+16384,0,49152);
}

void load_p(int a)
{
  emu_printf("load");

  int got_ascii_already=0;
  if(zx80) {
//    strcpy(fname,"zx80prog.p");    
  }
  else
  {
    if(a>=32768)   /* they did LOAD "" */
    {
      got_ascii_already=1;
      emu_printf("got ascii");
    } 
    if(!got_ascii_already)
    {
      /* test for Xtender-style LOAD " STOP " to quit */
      //if(*ptr==227) exit_program();    
      //memset(fname,0,sizeof(fname));
      //do
      //  *dptr++=zx2ascii[(*ptr)&63];
      //while((*ptr++)<128 && dptr<fname+sizeof(fname)-3);
      /* add '.p' */
      //strcat(fname,".p");
    }
  }
    

  emu_printf(tapename);
  if ( !emu_FileOpen(tapename) ) {
    /* the partial snap will crash without a file, so reset */
    if(autoload)
      reset81(),autoload=0;
    return;    

  }

  autoload=0;
  int size = emu_FileSize();
  emu_FileRead(mem + (zx80?0x4000:0x4009), size);
  emu_FileClose();

  if(zx80)
    store(0x400b,fetch(0x400b)+1);         
}

void save_p(int a)
{
  
}



void zx81hacks()
{
  /* patch save routine */
  mem[0x2fc]=0xed; mem[0x2fd]=0xfd;
  mem[0x2fe]=0xc3; mem[0x2ff]=0x07; mem[0x300]=0x02;

  /* patch load routine */
  mem[0x347]=0xeb;
  mem[0x348]=0xed; mem[0x349]=0xfc;
  mem[0x34a]=0xc3; mem[0x34b]=0x07; mem[0x34c]=0x02;
}

void zx80hacks()
{
  /* patch save routine */
  mem[0x1b6]=0xed; mem[0x1b7]=0xfd;
  mem[0x1b8]=0xc3; mem[0x1b9]=0x83; mem[0x1ba]=0x02;

  /* patch load routine */
  mem[0x206]=0xed; mem[0x207]=0xfc;
  mem[0x208]=0xc3; mem[0x209]=0x83; mem[0x20a]=0x02;
}


void z81_Init(void) 
{
#if HAS_SND
  emu_sndInit(); 
#endif 

  if (emu_ReadKeys() & MASK_KEY_USER2) setzx80mode(); 
  
  if (XBuf == 0) XBuf = (byte *)emu_Malloc(WIDTH*8);
  /* Set up the palette */
  int J;
  for(J=0;J<2;J++)
    emu_SetPaletteEntry(Palette[J].R,Palette[J].G,Palette[J].B, J);
  
  Reset8910(&ay,3500000,0);
  
  /* load rom with ghosting at 0x2000 */
  if(zx80)
  {
    memcpy( mem + 0x0000, zx80rom, 0x1000  );    
    memcpy(mem+0x1000*2,mem,0x1000*2);
  }
  else 
  {
    memcpy( mem + 0x0000, zx81rom, 0x2000  );
    memset(mem+0x4000,0,0xc000);     
  }

  int f;
  int ramsize;
  int count;
  /* ROM setup */
  count=0;
  for(f=0;f<16;f++)
  {
    memattr[f]=memattr[32+f]=0;
    memptr[f]=memptr[32+f]=mem+1024*count;
    count++;
    if(count>=(zx80?4:8)) count=0;
  }

  /* RAM setup */
  ramsize=16;
  if(unexpanded)
    ramsize=1;
  count=0;
  for(f=16;f<32;f++)
  {
    memattr[f]=memattr[32+f]=1;
    memptr[f]=memptr[32+f]=mem+1024*(16+count);
    count++;
    if(count>=ramsize) count=0;
  }
   
  if(zx80)
    zx80hacks();
  else
    zx81hacks();

  
  /* patch DISPLAY-5 to a return */
  //mem[ 0x02b5 + 0x0000 ] = 0xc9;
  //memcpy( mem + 0x2000, mem, 0x2000 );

  /* reset the keyboard state */
  memset( keyboard, 255, sizeof( keyboard ) );  
  
#ifdef ALT_Z80CORE
  ResetZ80();
#else
  ResetZ80(&z80, CYCLES_PER_FRAME);
#endif 
 }



void z81_Step(void)
{
#ifdef ALT_Z80CORE 
  ExecZ80();
  sighandler(0);
#else
  ExecZ80(&z80,CYCLES_PER_FRAME); // 3.5MHz ticks for 6 lines @ 30 kHz = 700 cycles
  IntZ80(&z80,INT_IRQ); // must be called every 20ms
  displayScreen();
  if (strlen(tapename)) handleKeyBuf();  
#endif  
  emu_DrawVsync(); 
  updateKeyboard();
  Loop8910(&ay,20);     
}


void z81_Start(char * filename)
{
  strncpy(tapename,filename,64);
}
