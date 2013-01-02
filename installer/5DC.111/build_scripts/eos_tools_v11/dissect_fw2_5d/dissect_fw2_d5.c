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
  uint32 i,  j;

	char fl_nm[100];
	uint32 crc, model_id, data_offset;
	
  
  if (argc <2) {
    printf("Usage: dissect_fw2 inputfile out_dir files_prefix\n");
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

	unsigned char header[0x30];
	sprintf(fl_nm,"%s/%s_0_header.bin",out_dir,prefix);

  if ((out = fopen(fl_nm, "wb")) == NULL) {
    printf("Cant't open file name %s\n", fl_nm);
    fclose(in);
    return -1;
  }

	fread (header,sizeof(header), 1, in);
	fwrite(header,sizeof(header), 1, out);
	fclose(out);
	model_id=*(uint32*)header;
	crc=*(uint32*)&header[0x20];
	data_offset=*(uint32*)&header[0x28];
	printf("model_id: %8.8X, crc: %8.8X, data offset: %8.8X\n",model_id, crc, data_offset);

	int flasher_size=data_offset-0x30;
	unsigned char *flasher=malloc(flasher_size);
	sprintf(fl_nm,"%s/%s_1_flasher.bin",out_dir,prefix);

  if ((out = fopen(fl_nm, "wb")) == NULL) {
    printf("Cant't open file name %s\n", fl_nm);
    fclose(in);
    return -1;
  }
	fread(flasher,flasher_size,1,in);
	fwrite(flasher,flasher_size,1, out);
	fclose(out);

	fseek( in, 0, SEEK_END );
	int file_size = ftell( in );
	fseek( in, data_offset, SEEK_SET );

	sprintf(fl_nm,"%s/%s_2_data_all.bin",out_dir,prefix);

  if ((out = fopen(fl_nm, "wb")) == NULL) {
    printf("Cant't open file name %s\n", fl_nm);
    fclose(in);
    return -1;
  }
	uint32 data_size=file_size-data_offset;
	unsigned char *data=malloc(data_size);
	fread(data,data_size,1, in);
	fwrite(data,data_size,1, out);
	fclose(out);


	fseek( in, data_offset, SEEK_SET );
	
struct {
	uint32 crc;
	uint32 something;
	uint32 items;
	uint32 this_struct_size; ///??? always 0x18 ???
	uint32 header_and_dir_size;
	uint32 dir_size;
	uint32 data_size;

} data_header;
	fread(&data_header,sizeof(data_header),1,in);
	printf(
		"data section\n"
			"\tcrc: %8.8X\n"
			"\tsomething: %8.8X\n"
			"\tdirectory items: %X\n"
			"\theader+dir size: %X\n"
			"\tdata size: %X\n",
		data_header.crc,
		data_header.something,
		data_header.items,
		data_header.header_and_dir_size,
		data_header.data_size
	);

	struct {
		uint32 something;
		uint32 type;
		uint32 offset;
		uint32 size;
		char name[0x20];
	} item;

	uint32 item_pos;
	for(i=0;i<data_header.items;i++){
		fread(&item,sizeof(item),1,in);
		item_pos=ftell(in);
		printf("item[%2.2X] type: %8.8X, something: %8.8X, offset: %8.8X, size: %8.8X, name: %s\n",i, item.type, item.something, item.offset+data_offset, item.size, item.name);

		sprintf(fl_nm,"%s/%s_3_%2.2X_item_%2.2X_%s.bin",out_dir,prefix,i,item.type,item.name);

	  if ((out = fopen(fl_nm, "wb")) == NULL) {
	    printf("Cant't open file name %s\n", fl_nm);
	    fclose(in);
	    return -1;
	  }
		
		fseek( in, data_offset+item.offset, SEEK_SET );
		fread(data,item.size,1,in);
		fwrite(data,item.size,1,out);
		fclose(out);

		fseek( in, item_pos, SEEK_SET );
	}
	
  fclose(in);


  return 0;
}
