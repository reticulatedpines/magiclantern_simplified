
/*
 * Copyright (C) 2015 Magic Lantern Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */
 
/*
 This code was obtained from:
    http://chdk.wikia.com/wiki/Obtaining_a_firmware_dump
 which references to:
    https://drive.google.com/uc?id=0B08pqRtyrObjTGpKNHJJZld2ZzQ&export=download
 
 It does not mention any license, so it is probably either public domain or some
 other free license. But we cannot 100% guarantee this.
 
 As this code uses a CRC16 implementation that is 100% matching the linux kernel's
 CRC16 implementation at https://github.com/torvalds/linux/blob/master/lib/crc16.c
 just with linux-specific macros removed, it is to assume that the crc16.c is being
 licensed under GPL.
 Due to the nature of GPL, all other code then must be also GPL licensed.
 
 If this is an incorrect assumption, please tell us.
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static uint8_t *file_data = NULL;
static uint32_t file_length = 0;

#include "crc16.c"


struct dump_header
{
    uint32_t address;
    uint16_t blocksize;
    uint8_t blocktype;
} __attribute__((packed));


int32_t findsig(int32_t sp)
{
    int32_t i;

    for (i=0; i < (file_length - sp - 4); i++)
    {
        if ((file_data[sp+i+0] == 0x0a) &&
            (file_data[sp+i+1] == 0x55) &&
            (file_data[sp+i+2] == 0xaa) &&
            (file_data[sp+i+3] == 0x50) )
        {
            return sp + i;
        }
    }
    return -1;
}

int main(int argc, char *argv[])
{
    uint32_t addr = 0;
    uint32_t blk = 0;
    uint32_t block_size = 1024;
    uint32_t base = 0;
    
    FILE *file_out = NULL;
    FILE *file_in = NULL;
    char *out_filename = "dump.dat";
    char *in_filename = "-";
    
    if(argc > 1)
    {
        in_filename = argv[1];
    }
    
    if(argc > 2)
    {
        out_filename = argv[2];
    }
    
    /* open input file */
    if(!strcmp(in_filename, "-"))
    {
        printf("Reading from stdin\n");
        file_in = stdin;
    }
    else
    {
        file_in = fopen(in_filename, "r+b");
    }

    if(!file_in)
    {
        printf("Failed to open input file\n");
        return;
    }
    
    /* open output filename */
    if(!strcmp(out_filename, "-"))
    {
        printf("Writing to stdout\n");
        file_out = stdout;
    }
    else
    {
        file_out = fopen(out_filename, "w+b");
    }

    if(!file_out)
    {
        printf("Failed to open output file\n");
        return;
    }
    

    /* get the file size */
    fseek(file_in, 0, SEEK_END);
    file_length = ftell(file_in);
    fseek(file_in, 0, SEEK_SET);
    
    /* read it at once */
    file_data = malloc(file_length);
    int read = fread(file_data,1, file_length, file_in);
    printf("read %d bytes... \n", read);
    fclose(file_in);

    
    int32_t file_offset = findsig(0);
    
    if(file_offset < 0)
    {
        printf("Failed to find signature\n");
        return -1;
    }
    
    while(file_offset >= 0)
    {
        uint32_t block_start = file_offset;
        uint32_t offset = 0;
        uint16_t crc_read = 0;
        uint16_t crc_calc = 0;
        uint8_t block_type = 0;
        /* skip sync header */
        offset += 4;
        
        /* read CRC that comes in front of the header */
        crc_read = *(uint16_t *)(&file_data[block_start + offset]);
        offset += 2;

        /* read the header */
        struct dump_header *header = (struct dump_header *)(&file_data[block_start + offset]);
        addr = header->address;
        block_size = header->blocksize;
        block_type = header->blocktype;
        

        if(block_type == 1) /* we have a special block with only 0x00 - bytes */
        {
            printf("  offset 0x%08X: addr 0x%08X, len 0x%04X, crc 0x%04X, all bytes 0, ", 
                   file_offset, addr, block_size, crc_read);
            crc_calc = crc16(0, &file_data[block_start + offset], sizeof(struct dump_header));
            if(crc_calc == crc_read)
            {
                uint32_t size = block_size;
                uint8_t *buf = malloc(size);
                memset(buf, 0, size);
                fwrite(buf, 1, size, file_out);
                free(buf);
                printf("CRC ok\n");
            }
            else
            {
                printf("CRC fail\n");
            }
            file_offset = findsig(block_start + offset + sizeof(struct dump_header));
            continue;
        }
        printf("  offset 0x%08X: addr 0x%08X, len 0x%04X, crc 0x%04X, ", file_offset, addr, block_size, crc_read);
        
        /* check sizes */
        if(block_start + offset + block_size + sizeof(struct dump_header) > file_length)
        {
            printf("FAIL: size error\n");
            file_offset = findsig(block_start + 4);
            continue;
        }
        
        /* calculate CRC over the data */
        crc_calc = crc16(0, &file_data[block_start + offset], block_size + sizeof(struct dump_header));
        
        /* if CRC matches, the block is valid */
        if(crc_read == crc_calc)
        {
            /* skip header */
            offset += sizeof(struct dump_header);
            
            /* first block decides the dump file start address */
            if(!base)
            {
                base = addr;
            }
            
            /* if blocks are omitted, it is due to saving bandwidth */
            if(blk && (blk < addr))
            {
                uint32_t size = addr - blk;
                
                printf("miss block 0x%08X, assume 0x%04X bytes 0xFF, ", blk, size);
                uint8_t *buf = malloc(size);
                memset(buf, 0xFF, size);
                fwrite(buf, 1, size, file_out);
                free(buf);
                
                blk += size;
            }
            
            /* write data block to the file */
            fseek(file_out, addr - base, SEEK_SET);
            fwrite(&file_data[block_start + offset], 1, block_size, file_out);
        
            offset += block_size;
            blk = addr + block_size;
            
            file_offset = findsig(block_start + offset);
            printf("\n");
        }
        else 
        {
            printf("FAIL: CRC: 0x%04X\n", crc_calc);
            file_offset = findsig(block_start + 4);
            continue;
        }
    }

    fclose(file_out);
}
