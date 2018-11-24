

#include <stdio.h>
#include <stdlib.h>

#define NOISEBSIZE 0x100
static int NoiseGen=1;

#define UBYTE unsigned char
//extern UBYTE POKEY_poly9_lookup[POKEY_POLY9_SIZE];
//extern UBYTE POKEY_poly17_lookup[POKEY_POLY17_SIZE];
#define POKEY_POLY9_SIZE  511//0x01ff
#define POKEY_POLY17_SIZE 16385//0x0001ffff
unsigned long reg;

int main(int argc, char *argv[]) {

  FILE *fp_wr = stdout;
  int i;

  if ((fp_wr = fopen ("noise.h", "wb")) == NULL)
  {
        fprintf (stderr, "Error:  can not create file %s\n", "noise.h");
        exit (1);
  }

  fprintf(fp_wr, "const UBYTE POKEY_poly9_lookup[] = {\n");


  /* initialise poly9_lookup */
  int cnt=0;

  reg = 0x1ff;
  for (i = 0; i < POKEY_POLY9_SIZE; i++) {

    reg = ((((reg >> 5) ^ reg) & 1) << 8) + (reg >> 1);
    //POKEY_poly9_lookup[i] = (UBYTE) reg;

    cnt++;
    if (cnt == 16) {
      fprintf(fp_wr, "%u,\n",(UBYTE)reg);
    }  
    else {
      fprintf(fp_wr, "%u,",(UBYTE)reg);
    }  
    cnt &= 15;

  }  
  fprintf(fp_wr, "};\n");

  fprintf(fp_wr, "const UBYTE POKEY_poly17_lookup[] = {\n");


  /* initialise poly17_lookup */
  cnt=0;

  reg = 0x1ffff;
  for (i = 0; i < POKEY_POLY17_SIZE; i++) {
    reg = ((((reg >> 5) ^ reg) & 0xff) << 9) + (reg >> 8);
    //POKEY_poly17_lookup[i] = (UBYTE) (reg >> 1);

    cnt++;
    if (cnt == 16) {
      fprintf(fp_wr, "%u,\n",(UBYTE)reg);
    }  
    else {
      fprintf(fp_wr, "%u,",(UBYTE)reg);
    }  
    cnt &= 15;
    
  }
  fprintf(fp_wr, "};\n");



  fclose (fp_wr);

  return 0;
}


