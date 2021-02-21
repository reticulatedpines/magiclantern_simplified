#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


typedef unsigned char uchar;
typedef unsigned int uint32;

#define CRs_SIZE 512
#define CRb_SIZE 513



void fill_tables( uchar first_s, uchar *crs, uchar *decrs, uchar *decrb, uchar *seq){
	int i;

	decrs[0]=first_s;

	for(i=1;i<CRs_SIZE;i++){
		decrs[i] = first_s ^ crs[0] ^ crs[i];
	}

	for(i=0;i<CRb_SIZE;i++){
		
		decrb[i] = decrs[i%CRs_SIZE] ^ seq[i];
//		printf("decrb[%d][%2.2X]=crs[%d][%2.2X] ^ seq[%d][%2.2X]\n", i,decrb[i],  i%CRs_SIZE, decrs[i%CRs_SIZE], i, seq[i]);
	}

}

void generate_sequence( uchar *decrs, uchar *decrb, uchar *tseq){
	int i;

	for(i=0;i<CRs_SIZE*CRb_SIZE;i++){

		tseq[i] = decrs[ i % CRs_SIZE ] ^ decrb[ i % CRb_SIZE ];

	}

}





int main(int argc, char *argv[])
{
  FILE *in;

	uint32 i,n, sz;
	uchar crs[CRs_SIZE] , decrs[CRs_SIZE], decrb[CRb_SIZE], ok, *sep;

	uchar seq[CRs_SIZE*CRb_SIZE];
	uchar tseq[CRs_SIZE*CRb_SIZE];


	uint32 first;


	
  
  if (argc <2) {
    printf("Usage: recreate_tables inputfile [first]\n");
    return -1;
  }

  
  if ((in = fopen(argv[1], "rb")) == NULL) {
    printf("Cant't open file name %s\n", argv[1]);
    return -1;
  }
  

	for(sz=sizeof(seq),i=0; sz > 0 && !feof(in);){
		 n= fread(seq+i,1,sz,in);
//			printf("%d...",n);
			i+=n;
			sz-=n;
	}

	fclose(in);

	if(sz>0){
    printf("read err. bytes to read %d\n", sz);
    return -1;
	}

	for(i=0;i<CRs_SIZE*CRb_SIZE;i+=CRb_SIZE){
			crs[ i % sizeof(crs)]=seq[i];
//			printf("//[%5.5d] crs[%3.3d]=0x%2.2X\n", i,i % sizeof(crs), crs[ i % sizeof(crs)]);
	}


#if 1
	for(ok=0,i=0;i<256;i++){
		fprintf(stderr, "\rpass %2.2d of 256",i+1);
		fill_tables( i, crs, decrs, decrb, seq);
		generate_sequence( decrs, decrb, tseq);
		if(memcmp(seq,tseq,sizeof(seq))==0){
			ok=1;
			break;
		}
	}



	fprintf(stderr,"\x07");

	if(!ok){
		printf("tables have not been recreated\n");
		return -1;
	}
#endif

	if(argc>2){
		sscanf(argv[2],"%x",&first);
		fill_tables( first, crs, decrs, decrb, seq);
	}



	printf("#define CRYPT1_SIZE 512\n#define CRYPT2_SIZE 513\n\n\nunsigned char crypt1[CRYPT1_SIZE] = {");

	for(sep="\n ",i=0;i<CRs_SIZE;i++){
		if((i%8)==0 && i>0){
			sep=",\n ";
		}
		printf("%s0x%2.2x",sep,decrs[i]);
		sep=", ";
	}

	printf("\n};\n\n");

	printf("unsigned char crypt2[CRYPT2_SIZE] = {");

	for(sep="\n ",i=0;i<CRb_SIZE;i++){
		if((i%8)==0 && i>0){
			sep=",\n ";
		}
		printf("%s0x%2.2x",sep,decrb[i]);
		sep=", ";
	}
	printf("\n};\n\n");



  return 0;
}
