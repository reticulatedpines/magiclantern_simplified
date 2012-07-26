#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


int main(int argc, char *argv[])
{

  FILE *in, *in2;
  FILE *out;
  int i = 0, j = 0, val, val2;
  
  if (argc != 4) {
    printf("Usage: fxor inputfile1 inputfile2 outfile\n");
    return -1;
  }
  
  if ((in = fopen(argv[1], "rb")) == NULL) {
    printf("Cant't open file name %s\n", argv[1]);
    return -1;
  }
  if ((in2 = fopen(argv[2], "rb")) == NULL) {
    printf("Cant't open file name %s\n", argv[2]);
    return -1;
  }
  
  if ((out = fopen(argv[3], "wb")) == NULL) {
    printf("Cant't open file name %s\n", argv[3]);
    fclose(in);
    return -1;
  }
  
  while ((val = fgetc(in)) != EOF) {
		val2=fgetc(in2);
    fputc(val ^ val2, out);
  }
  fclose(out);
  fclose(in);
  fclose(in2);

  return 0;
}
