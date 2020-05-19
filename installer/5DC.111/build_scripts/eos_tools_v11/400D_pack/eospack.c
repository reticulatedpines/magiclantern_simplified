#include <stdio.h>
#include <unistd.h>

#define CK(X) -1-(X)

typedef unsigned long t_dword;
typedef unsigned char t_byte;

typedef struct
{ t_dword modelID;
  t_byte unknown[12];
  char version[16];
  t_dword cksum;
  t_dword firstOffset;
  t_dword secondOffset;
  t_dword unknown_0;
} s_main_head;

typedef struct
{ t_dword cksum;
  t_dword rec_num;
  t_dword tableOffset;
  t_dword packOffset;
  t_dword tableLenght;
  t_dword packLenght;
} s_pack_head;

typedef struct
{ t_dword flags;
  t_dword Offset;
  t_dword lenght;
  char name[28];
} s_file_rec;

void f_clear(void *p, t_dword s)
{ t_byte *cp;  cp=p;
  while (s--) *(cp++)=0;
}
t_dword f_cksum(void *p, t_dword s)
{ t_byte *cp; t_dword sum=0; cp=p;
  while (s--) sum += *(cp++);
  return sum;
}

t_dword f_filecpy(s_file_rec *rec, FILE* out, FILE* in)
{ t_byte buf[256];
  t_dword red,sum=0,len=0;
  while (! feof(in))
  { red = fread(buf, sizeof(t_byte), 256, in);
    fwrite(buf, sizeof(t_byte), red, out);
    sum += f_cksum(buf, red);
    len += red;
//    printf("red:%d ", red);
  }  
  rec->lenght=len;
  return sum; 
}

int f_add(t_dword* cksum, s_file_rec *rec, FILE* out, t_dword flags, char* infile, char* name)
{ FILE* in;
  printf("Adding %s->%s\n",infile, name);
  if ((in = fopen(infile, "rb")) == NULL)
  {	printf("Can't open file name %s\n", infile);
        return 0;
  }
  *cksum += f_filecpy(rec, out, in);
  fclose(in);
  strcpy(rec->name, name);
  return rec->lenght;
}

main(int argc, char *argv[])
{
        FILE* out;
        FILE* pack;
        s_main_head mhead;
        s_pack_head phead;
        s_file_rec rec[20];
        s_file_rec frec;
        t_dword len, cksum=0, i=0,j;
        f_clear(&mhead,sizeof(s_main_head));
        f_clear(&phead,sizeof(s_pack_head));
        f_clear(rec,sizeof(s_file_rec)*20);
        
        
        if (argc < 2)
                printf("eospack outfile [firm version]");
        if (argc > 2)
                strcpy(mhead.version,	argv[2]);
        else
                strcpy(mhead.version,	"eos test firm1");
                
        if ((out = fopen(argv[1], "wb")) == NULL) {
                printf("Cant't open file name %s\n", argv[1]);
                exit(-1);
        }
        
        if ((pack = fopen("tmp.pack", "wb+")) == NULL) {
                printf("Cant't open file name tmp.pack\n");
                exit(-1);
        }

        printf("File to write: %s\n", argv[1]);
        printf("Writing pack\n");
        
        cksum = 0;
        if (f_add(&cksum, &(rec[i]), pack, 8, "firm", "MAIN_FIRMWARE")) i++;
        if (f_add(&cksum, &(rec[i]), pack, 8, "ver", "FirmwareVersion")) i++;
        fseek(pack, 0, SEEK_SET);
        
        phead.tableOffset = sizeof(s_pack_head);
        phead.rec_num = i;
        phead.tableLenght = i*sizeof(s_file_rec);
        phead.packOffset = phead.tableLenght+phead.tableOffset;
        for (j=0; j<i; j++)
        { 
          rec[j].Offset = phead.packLenght + phead.packOffset;
          phead.packLenght += rec[j].lenght;
          cksum += f_cksum(&(rec[j]), sizeof(s_file_rec));
        }
        phead.cksum = CK(cksum + f_cksum(&phead,sizeof(s_pack_head)));
        
        printf("Writing main\n");
        
        cksum=0;
        len=0;

        mhead.modelID=	0x80000236;
        mhead.cksum= 	0x00000000;
        mhead.firstOffset = sizeof(s_main_head);
        
        fwrite(&mhead, sizeof(s_main_head), 1, out);
        
        len =f_add(&cksum, &frec, out, 0, "loader", "LOADER");
        mhead.secondOffset = mhead.firstOffset + len;
        
        fwrite (&phead, sizeof(s_pack_head), 1, out);
        fwrite (rec, sizeof(s_file_rec), phead.rec_num, out);
        cksum += f_filecpy(&frec, out, pack);        
        
        mhead.cksum = CK(cksum + 
          f_cksum(&mhead, sizeof(s_main_head)) + 
          f_cksum(&phead, sizeof(s_pack_head)) +
          f_cksum(rec, sizeof(s_file_rec)*phead.rec_num));
        
        fseek(out, 0, SEEK_SET);
        fwrite(&mhead, sizeof(s_main_head), 1, out);
        
        fclose(pack);        
        fclose(out);

        exit(0);
}

