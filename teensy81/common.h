
typedef unsigned char  byte;
typedef unsigned short  word;

#define WIDTH  320
#define HEIGHT 192
#define BORDER 32

#define CYCLES_PER_FRAME 65000//3500000/50


/* full internal image with overscan (but not hsync/vsync areas) */
#define ZX_VID_MARGIN		55
#define ZX_VID_HMARGIN		(8*8)
#define ZX_VID_FULLWIDTH	(2*ZX_VID_HMARGIN+32*8)	/* sic */
#define ZX_VID_FULLHEIGHT	(2*ZX_VID_MARGIN+192)


/* AY board types */
#define AY_TYPE_NONE		0
#define AY_TYPE_QUICKSILVA	1
#define AY_TYPE_ZONX		2


extern unsigned char mem[];
extern unsigned char *memptr[64];
extern int memattr[64];
extern unsigned long tstates,tsmax;
extern int sound_ay,sound_ay_type,vsync_visuals;
extern int invert_screen;

extern int interrupted;
extern int nmigen,hsyncgen,vsync;
extern int taguladisp;
extern int autoload;
extern int scrn_freq;
extern int fakedispx,fakedispy;

extern int refresh_screen;
extern int zx80;
extern int ignore_esc;


extern void sighandler(int a);
extern char *libdir(char *file);
extern void startsigsandtimer();
extern void exit_program(void);
extern void initmem();
extern void loadhelp(void);
extern void zxpopen(void);
extern void zxpclose(void);
extern unsigned int in(int h,int l);
extern unsigned int out(int h,int l,int a);
extern void do_interrupt();
extern void save_p(int a);
extern void load_p(int a);
extern void update_kybd();
extern void do_interrupt();
extern void reset81();
extern void parseoptions(int argc,char *argv[]);
extern void frame_pause(void);
extern void bitbufBlit(unsigned char * buf);
