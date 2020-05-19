/*
 * recompute a exFAT VBR checksum in sector 12
 */

#include<stdio.h>

typedef unsigned int UINT32;

UINT32 VBRChecksum( unsigned char octets[], long NumberOfBytes) {
   UINT32 Checksum = 0;
   int Index;
   for (Index = 0; Index < NumberOfBytes; Index++) {
     if (Index != 106 && Index != 107 && Index != 112)  // skip 'volume flags' and 'percent in use'
	 Checksum = ((Checksum <<31) | (Checksum>> 1)) + (UINT32) octets[Index];
   }
   return Checksum;
}

inline unsigned int endian_swap(unsigned int x) {
   return (x<<24) | ((x>>8) & 0x0000FF00) | ((x<<8) & 0x00FF0000) | (x>>24);
}

#define SIZE (512*11)

int main(int argc, char *argv[]) {
  FILE* f;
  unsigned int buffer[SIZE+512];
  int i=0;
  unsigned int sum, swappedSum;

  f=(FILE*)fopen(argv[1], "rb+");
  if (f) {
    fread(buffer, 1, SIZE+512, f);
    printf("old=0x%lx, ", buffer[ SIZE/4 ]);
    sum = VBRChecksum((unsigned char*)buffer, SIZE);
    printf("new=0x%lx\n", sum);
    swappedSum = endian_swap( sum );
    //    printf("0x%lx\n", swappedSum);
    for(i=0; i<512/4; i++)
      buffer[ i] = sum;
    fseek(f, SIZE, SEEK_SET);
    fwrite(buffer, 1, 512, f);
    fclose(f);
  }

}