#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


typedef unsigned int uint32;


int main(int argc, char *argv[])
{
  FILE *in;
  uint32 csum=0, fsum=0, n, i=0;
	unsigned char val;

	
  
  if (argc != 2) {
    printf("Usage: check_sum inputfile\n");
    return -1;
  }

  
  if ((in = fopen(argv[1], "rb")) == NULL) {
    printf("Cant't open file name %s\n", argv[1]);
    return -1;
  }
  
  for(i=0; ! feof(in) ;i++) {

		if(i==0x20){
			fread(&fsum,sizeof(fsum),1,in);
		} else {
		 n=fread(&val, sizeof(val), 1, in);
		if(n)csum+=val;
			
		}
  }
  fclose(in);

	printf("calculated : 0x%8.8X ^ 0xFFFFFFFF = 0x%8.8X ( %s )\n",csum,0xffffffff^csum, argv[1]);
	printf("embedded : 0x%8.8X\n",fsum);

  return 0;
}
