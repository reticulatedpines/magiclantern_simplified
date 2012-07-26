#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "40D_table.h"

typedef unsigned short uint16;

int main(int argc, char *argv[])
{
  FILE *in;
  FILE *out;
  int i = 0, j = 0, val;
  
  if (argc != 3) {
    printf("Usage: decrypt inputfile outfile\n");
    return -1;
  }
  
  if ((in = fopen(argv[1], "rb")) == NULL) {
    printf("Cant't open file name %s\n", argv[1]);
    return -1;
  }
  
  if ((out = fopen(argv[2], "wb")) == NULL) {
    printf("Cant't open file name %s\n", argv[2]);
    fclose(in);
    return -1;
  }
  
//  fseek(in, 0x120, SEEK_SET);	//Skip the first 288 bytes for 40D they are not encrypted
#if 0
  for(i=0; i < 0x120 ;i++) {
		if((val = fgetc(in)) != EOF){
	    fputc(val, out);
		}
	}
#endif


  for(i=0;(val = fgetc(in)) != EOF;) {
    fputc(val ^ crypt1[i] ^ crypt2[j], out);
    i++;
    j++;
    if (i >= CRYPT1_SIZE) i=0;
    if (j >= CRYPT2_SIZE) j=0;
  }
  fclose(out);
  fclose(in);
  return 0;
}
