#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


typedef unsigned int uint32;


int main(int argc, char *argv[])
{
  FILE *in;
  uint32 csum=0;
	unsigned char val, n;

	
  
  if (argc != 2) {
    printf("Usage: check_sum inputfile\n");
    return -1;
  }

  
  if ((in = fopen(argv[1], "rb")) == NULL) {
    printf("Cant't open file name %s\n", argv[1]);
    return -1;
  }
  
  while ( ! feof(in) ) {
	 n=fread(&val, sizeof(val), 1, in);
		if(n)csum+=val;		
  }
  fclose(in);

	printf("0x%8.8X ^ 0xFFFFFFFF = 0x%8.8X ( %s )\n",csum,0xffffffff^csum, argv[1]);

  return 0;
}
