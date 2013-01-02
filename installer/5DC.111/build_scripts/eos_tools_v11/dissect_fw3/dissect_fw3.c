#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


typedef unsigned short uint16;
typedef unsigned int uint32;



int main(int argc, char *argv[])
{
  FILE *in;
  FILE *out;
	FILE *rep;
  uint32 i,  j;

	char fl_nm[100];
	uint32 crc, model_id, data_offset;





	
	
  
  if (argc <2) {
    printf("Usage: dissect_fw3 inputfile out_dir files_prefix\n");
    return -1;
  }

  char *out_dir=".";
  char *prefix=argv[1];

  if(argc>2){
	out_dir=argv[2];
  }

  if(argc>3){
	prefix=argv[3];
  }

  mkdir(out_dir);

  
  if ((in = fopen(argv[1], "rb")) == NULL) {
    printf("Cant't open file name %s\n", argv[1]);
    return -1;
  }

	strcpy(fl_nm,argv[1]);
	strcat(fl_nm,".csv");
  if ((rep = fopen(fl_nm, "wb")) == NULL) {
    printf("Cant't open file name %s\n", fl_nm);
    return -1;
  }



	fseek( in, 0, SEEK_END );
	uint32 file_size = ftell( in );
	fseek( in, 0, SEEK_SET );
	unsigned char *data=malloc(file_size);

	uint32 *arr=(uint32*)data;


	fprintf(rep,"head,,%s\n",argv[1]);
	fprintf(rep,"file size,,0x%8.8X\n",file_size);


	sprintf(fl_nm,"%s/%s_0_header.bin",out_dir,prefix);

  if ((out = fopen(fl_nm, "wb")) == NULL) {
    printf("Cant't open file name %s\n", fl_nm);
    fclose(in);
    return -1;
  }

	fread (data,0x120, 1, in);
	fwrite(data,0x120, 1, out);
	fclose(out);

	for(i=0;i<0x48;i++){
		fprintf(rep,",0x%2.2X,0x%8.8X\n",i*4,arr[i]);
	}

	data_offset=*(uint32*)&data[0x60];

	sprintf(fl_nm,"%s/%s_1_flasher.bin",out_dir,prefix);

  if ((out = fopen(fl_nm, "wb")) == NULL) {
    printf("Cant't open file name %s\n", fl_nm);
    fclose(in);
    return -1;
  }
	fread(data,data_offset-0x120,1,in);
	fwrite(data,data_offset-0x120,1, out);
	fclose(out);

	sprintf(fl_nm,"%s/%s_2_data_head.bin",out_dir,prefix);

  if ((out = fopen(fl_nm, "wb")) == NULL) {
    printf("Cant't open file name %s\n", fl_nm);
    fclose(in);
    return -1;
  }

	fread(data,0x18,1, in);
	fwrite(data,0x18,1, out);
	fclose(out);


	fprintf(rep,"data head\n");

	for(i=0;i<0x6;i++){
		fprintf(rep,",0x%2.2X,0x%8.8X\n",i*4,arr[i]);
	}


	sprintf(fl_nm,"%s/%s_3_data_body.bin",out_dir,prefix);

  if ((out = fopen(fl_nm, "wb")) == NULL) {
    printf("Cant't open file name %s\n", fl_nm);
    fclose(in);
    return -1;
  }

	
		
	fread(data,file_size-data_offset-0x18,1,in);
	fwrite(data,file_size-data_offset-0x18,1,out);
	fclose(out);

  fclose(in);


  return 0;
}
