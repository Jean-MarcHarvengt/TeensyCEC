
#include "AudioPlaySystem.h"
#include <AudioStream.h>

extern "C" {
#include "emuapi.h"
}

#define SAMPLERATE AUDIO_SAMPLE_RATE_EXACT
#define CLOCKFREQ 985248

//#define ALT_BUZZER 1

static const short square[]={
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
};

const short noise[] {
-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,32767,-32767,
-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,
-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,-32767,32767,32767,-32767,
-32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,-32767,-32767,32767,32767,-32767,
-32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,-32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,-32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,-32767,32767,32767,32767,-32767,-32767,
32767,-32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,32767,-32767,32767,32767,32767,-32767,-32767,
32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,32767,32767,-32767,32767,-32767,32767,-32767,-32767,
32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,32767,-32767,32767,-32767,-32767,
};

#define NOISEBSIZE 0x100

typedef struct
{
  unsigned int spos;
  unsigned int sinc;
  unsigned int vol;
} Channel;

static bool ayMode=false;
volatile bool playing = false;
#if ALT_BUZZER
#define BUZZBSIZE  0x1000
static int * buzzer;
static int cnt=0;
static short b=-32767;
static int sinc=140;
static bool toggle=false;
#else
#define BUZZBSIZE  0x800
static short * buzzer;
#endif
static int wpos=0;
static int spos=0;


static Channel chan[6] = {
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0} };


static void snd_Reset(void)
{
  chan[0].vol = 0;
  chan[1].vol = 0;
  chan[2].vol = 0;
  chan[3].vol = 0;
  chan[4].vol = 0;
  chan[5].vol = 0;
  chan[0].sinc = 0;
  chan[1].sinc = 0;
  chan[2].sinc = 0;
  chan[3].sinc = 0;
  chan[4].sinc = 0;
  chan[5].sinc = 0;
  wpos = 0;
  spos = 0;
#if ALT_BUZZER
  sinc = (3500000*2)/(50*882); //((size*(AUDIO_SAMPLE_RATE_EXACT))/3500000);
  cnt  = 0,
  toggle = false;
#endif   
}


static void snd_Mixer(short *  stream, int len )
{
  int i;
  long s;  
  len = len >> 1;  
  if (ayMode) {
    short v0=chan[0].vol;
    short v1=chan[1].vol;
    short v2=chan[2].vol;
    short v3=chan[3].vol;
    short v4=chan[4].vol;
    short v5=chan[5].vol;
    for (i=0;i<len;i++)
    {
      s =((v0*square[(chan[0].spos>>8)&0x3f])>>11);
      s+=((v1*square[(chan[1].spos>>8)&0x3f])>>11);
      s+=((v2*square[(chan[2].spos>>8)&0x3f])>>11);
      s+=((v3*noise[(chan[3].spos>>8)&(NOISEBSIZE-1)])>>11);
      s+=((v4*noise[(chan[4].spos>>8)&(NOISEBSIZE-1)])>>11);
      s+=((v5*noise[(chan[5].spos>>8)&(NOISEBSIZE-1)])>>11);         
      *stream++ = (short)(s);
      *stream++ = (short)(s);
      chan[0].spos += chan[0].sinc;
      chan[1].spos += chan[1].sinc;
      chan[2].spos += chan[2].sinc;
      chan[3].spos += chan[3].sinc;  
      chan[4].spos += chan[4].sinc;  
      chan[5].spos += chan[5].sinc;  
    }    
  }
  else {

    for (i=0;i<len;i++)
    {
  #if ALT_BUZZER
      if (cnt != 0) {
        toggle=buzzer[spos]&1;
        if (buzzer[spos] > 0) {
          buzzer[spos] -= sinc;
          buzzer[spos] &= 0xfffffffe; 
          buzzer[spos] |= toggle;        
        } else {
          int t = buzzer[spos];
          spos = (spos + 1)&(BUZZBSIZE-1);
          buzzer[spos] -= t;
          //toggle=(toggle?false:true);
          cnt--;
        }
        s = ((!toggle)?32767:-32767);    
      }
  #else
      if (spos != wpos) {
        s = buzzer[spos];
        spos = (spos + 1)&(BUZZBSIZE-1);
      }
  #endif   
      *stream++ = (short)(s);
      *stream++ = (short)(s); 
    }      
  }
}
  
void AudioPlaySystem::begin(void)
{
  emu_printf("AudioPlaySystem constructor");
	this->reset();
	//setSampleParameters(CLOCKFREQ, SAMPLERATE);
}

void AudioPlaySystem::start(void)
{
  emu_printf("allocating sound buf");
#if ALT_BUZZER
  buzzer = (int*)emu_Malloc(BUZZBSIZE*sizeof(int));
#else
  buzzer = (short*)emu_Malloc(BUZZBSIZE*sizeof(short));
#endif  
  playing = true;  
}

void AudioPlaySystem::setSampleParameters(float clockfreq, float samplerate) {
}

void AudioPlaySystem::reset(void)
{
	snd_Reset();
}

void AudioPlaySystem::stop(void)
{
	__disable_irq();
	playing = false;	
	__enable_irq();
}

bool AudioPlaySystem::isPlaying(void) 
{ 
  return playing; 
}

void AudioPlaySystem::update(void) {
	audio_block_t *block;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

  snd_Mixer((short*)block->data,AUDIO_BLOCK_SAMPLES);
  //memset( (void*)block->data, 0, AUDIO_BLOCK_SAMPLES*2);

	transmit(block);
	release(block);
}

void AudioPlaySystem::sound(int C, int F, int V) {
  if ((F) || (V)) ayMode=true;
  if (C < 6) {
    chan[C].vol = V;
    chan[C].sinc = F>>1; 
  }
}

void AudioPlaySystem::step(void) {
}


void AudioPlaySystem::buzz(int size, int val) {
  if ( (playing) && (!ayMode) ) {
#if ALT_BUZZER
    if (val) 
      size |= 1;
    else
      size &= 0xfffffffe;      
    buzzer[(wpos++)&(BUZZBSIZE-1)] = size;
    cnt++;
#else
    size = (size*AUDIO_SAMPLE_RATE_EXACT)/3500000;
    while ( (size>=0)  ) 
    {         
      buzzer[wpos] = ((val==1)?32767:-32767);
      wpos = (wpos + 1)&(BUZZBSIZE-1);
      size--; 
    } 
#endif   
    //*(int16_t *)&(DAC0_DAT0L) = (val == 1) ? 3000 : 1000; 
  }
}

