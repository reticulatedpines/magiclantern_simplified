#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


int main(int argc, char *argv[])
{

	unsigned int z1,z2;
  
  if (argc != 3) {
    printf("Usage: xor n1 n2\n");
    return -1;
  }
	sscanf(argv[1], "%x",&z1);
	sscanf(argv[2], "%x",&z2);
  
	printf("%x ^ %x = %2.2x",z1,z2,(z1^z2));

  return 0;
}
