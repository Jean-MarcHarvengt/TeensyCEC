

#include <stdio.h>
#include <stdlib.h>


int processFile(char * infile, char * outfile, char * arrname) 
{
  FILE *fp_rd = stdin;
  FILE *fp_wr = stdout;

  int cnt=0;

  if ((fp_rd = fopen (infile, "rb")) == NULL)
  {
    fprintf (stderr, "Error:  can not open file %s\n", infile);
    return -1;  	
  }	
  if ((fp_wr = fopen (outfile, "wb")) == NULL)
  {
    fprintf (stderr, "Error:  can not create file %s\n", outfile);
    return -1;  	
  }
  

  fseek(fp_rd, 0L, SEEK_END);
  int size = ftell(fp_rd);
  fseek(fp_rd, 0L, SEEK_SET);

  printf ("Reading %d bytes\n", size);
  fprintf(fp_wr, "const UBYTE %s[%d] = {\n", arrname, size);

  cnt = 0;
  for (int i = 0; i < size; i++) {
  	unsigned char b;
  	if (fread(&b, 1, 1, fp_rd) != 1) {
  		fprintf (stderr, "Error:  can not read more bytes\n");
   		fclose (fp_wr);
  		fclose (fp_rd);   		
  		return -1;
  	}	
    cnt++;
    if (cnt == 16) {
      fprintf(fp_wr, "0x%02X,\n",b);
    }  
    else {
      fprintf(fp_wr, "0x%02X,",b);
    }  
    cnt &= 15;
  }  
  fprintf(fp_wr, "};\n");

  fclose (fp_wr);
  fclose (fp_rd);
  return 1;  
}


int main(int argc, char *argv[]) {

  if (processFile("ATARIBAS.ROM","romataribas.h","romataribas") < 0)
    return (-1);

  if (processFile("ATARIOSB.ROM","romatariosb.h","romos") < 0)
    return (-1);

  if (processFile("Atariosa.rom","romatariosa.h","romos") < 0)
    return (-1);

  if (processFile("ATARIXL.ROM","romatarixl.h","romos") < 0)
    return (-1);

  if (processFile("REV01.ROM","romrev1.h", "romos") < 0)
    return (-1);

  if (processFile("REVAPAL.ROM","romrevpal.h", "romos") < 0)
    return (-1);

  if (processFile("REV10.ROM","romrev10.h", "romos") < 0)
    return (-1);


  return 0;
}


