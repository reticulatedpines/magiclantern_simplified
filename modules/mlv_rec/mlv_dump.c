/*
 * Copyright (C) 2013 Magic Lantern Team
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

/* system includes */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>

/* some compile warning, why? */
char *strdup(const char *s);

//#define MLV_USE_LZMA

#ifdef MLV_USE_LZMA
#include <LzmaLib.h>
#endif

/* project includes */
#include "../lv_rec/lv_rec.h"
#include "../../src/raw.h"
#include "mlv.h"

/* helper macros */
#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define COERCE(val,min,max) MIN(MAX((value),(min)),(max))

/* this structure is used to build the mlv_xref_t table */
typedef struct 
{
    uint64_t    frameTime;
    uint64_t    frameOffset;
    uint32_t    fileNumber;
} frame_xref_t;





void xref_resize(frame_xref_t **table, int entries, int *allocated)
{
    /* make sure there is no crappy pointer before using */
    if(*allocated == 0)
    {
        *table = NULL;
    }
    
    /* only resize if the buffer is too small */
    if(entries * sizeof(frame_xref_t) > *allocated)
    {
        *allocated += (entries + 1) * sizeof(frame_xref_t);
        *table = realloc(*table, *allocated);
    }
}

void xref_sort(frame_xref_t *table, int entries)
{
    int n = entries;
    do
    {
        int newn = 1;
        for (int i = 0; i < n-1; ++i)
        {
            if (table[i].frameTime > table[i+1].frameTime)
            {
                frame_xref_t tmp = table[i+1];
                table[i+1] = table[i];
                table[i] = tmp;
                newn = i + 1;
            }
        }
        n = newn;
    } while (n > 1);
}

void bitinsert(uint16_t *dst, int position, int depth, uint16_t new_value)
{
    uint16_t old_value = 0;
    int dst_pos = position * depth / 16;
    int bits_to_left = ((depth * position) - (16 * dst_pos)) % 16;
    int shift_right = 16 - depth - bits_to_left;

    old_value = dst[dst_pos];
    if(shift_right >= 0)
    {
        /* this case is a bit simpler. the word fits into this uint16_t */
        uint16_t mask = ((1<<depth)-1) << shift_right;
        
        /* shift and mask out */
        new_value <<= shift_right;
        new_value &= mask;
        old_value &= ~mask;
        
        /* now combine */
        new_value |= old_value;
        dst[dst_pos] = new_value;
    }
    else
    {
        /* here we need two operations as the bits are split over two words */
        uint16_t mask1 = ((1<<(depth + shift_right))-1);
        uint16_t mask2 = ((1<<(-shift_right))-1) << (16+shift_right);
        
        /* write the upper bits */
        old_value &= ~mask1;
        old_value |= (new_value >> (-shift_right)) & mask1;
        dst[dst_pos] = old_value;
        
        /* write the lower bits */
        old_value = dst[dst_pos + 1];
        old_value &= ~mask2;
        old_value |= (new_value << (16+shift_right)) & mask2;
        dst[dst_pos + 1] = old_value;
    }
}

uint16_t bitextract(uint16_t *src, int position, int depth)
{
    uint16_t value = 0;
    int src_pos = position * depth / 16;
    int bits_to_left = ((depth * position) - (16 * src_pos)) % 16;
    int shift_right = 16 - depth - bits_to_left;

    value = src[src_pos];

    if(shift_right >= 0)
    {
        value >>= shift_right;
    }
    else
    {
        value <<= -shift_right;
        value |= src[src_pos + 1] >> (16 + shift_right);
    }
    value &= (1<<depth) - 1;
    
    return value;
}

int load_frame(char *filename, int frame_number, uint8_t *frame_buffer)
{
    FILE *in_file = NULL;
    int ret = 0;
    
    /* open files */
    in_file = fopen(filename, "rb");
    if(!in_file)
    {
        fprintf(stderr, "[E] Failed to open file '%s'\n", filename);
        return 1;
    }

    do
    {
        mlv_hdr_t buf;
        uint64_t position = 0;
        
        position = ftello(in_file);
        
        if(fread(&buf, sizeof(mlv_hdr_t), 1, in_file) != 1)
        {
            fprintf(stderr, "[E] Failed to read from file '%s'\n", filename);
            ret = 2;
            goto load_frame_finish;
        }
        
        /* jump back to the beginning of the block just read */
        fseeko(in_file, position, SEEK_SET);

        position = ftello(in_file);
        
        if(!memcmp(buf.blockType, "MLVI", 4))
        {
            mlv_file_hdr_t file_hdr;
            uint32_t hdr_size = MIN(sizeof(mlv_file_hdr_t), buf.blockSize);

            /* read the whole header block, but limit size to either our local type size or the written block size */
            if(fread(&file_hdr, hdr_size, 1, in_file) != 1)
            {
                fprintf(stderr, "[E] File ends in the middle of a block\n");
                ret = 3;
                goto load_frame_finish;
            }
            fseeko(in_file, position + file_hdr.blockSize, SEEK_SET);

            if(file_hdr.videoClass & MLV_VIDEO_CLASS_FLAG_LZMA)
            {
                fprintf(stderr, "[E] Compressed formats not supported for frame extraction\n");
                ret = 5;
                goto load_frame_finish;
            }
        }
        else if(!memcmp(buf.blockType, "VIDF", 4))
        {
            mlv_vidf_hdr_t block_hdr;
            uint32_t hdr_size = MIN(sizeof(mlv_vidf_hdr_t), buf.blockSize);

            if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
            {
                fprintf(stderr, "[E] File '%s' ends in the middle of a block\n", filename);
                ret = 3;
                goto load_frame_finish;
            }
            
            int frame_size = block_hdr.blockSize - sizeof(mlv_vidf_hdr_t) - block_hdr.frameSpace;

            fseeko(in_file, block_hdr.frameSpace, SEEK_CUR);
            if(fread(frame_buffer, frame_size, 1, in_file) != 1)
            {
                fprintf(stderr, "[E] File '%s' ends in the middle of a block\n", filename);
                ret = 4;
                goto load_frame_finish;
            }
            
            ret = 0;
            goto load_frame_finish;
        }
        else
        {
            fseeko(in_file, position + buf.blockSize, SEEK_SET);
        }
    }
    while(!feof(in_file));
    
load_frame_finish:

    fclose(in_file);
    
    return ret;
}

mlv_xref_hdr_t *load_index(char *base_filename)
{
    mlv_xref_hdr_t *block_hdr = NULL;
    char filename[128];
    FILE *in_file = NULL;

    strncpy(filename, base_filename, sizeof(filename));
    strcpy(&filename[strlen(filename) - 3], "IDX");
    
    in_file = fopen(filename, "rb");
    
    if(!in_file)
    {
        return NULL;
    }

    printf("[i] File %s opened (XREF)\n", filename);
    
    do
    {
        mlv_hdr_t buf;
        uint64_t position = 0;
        
        position = ftello(in_file);
        
        if(fread(&buf, sizeof(mlv_hdr_t), 1, in_file) != 1)
        {
            break;
        }
        
        /* jump back to the beginning of the block just read */
        fseeko(in_file, position, SEEK_SET);

        position = ftello(in_file);
        
        /* we should check the MLVI header for matching UID value to make sure its the right index... */
        if(!memcmp(buf.blockType, "XREF", 4))
        {
            block_hdr = malloc(buf.blockSize);

            if(fread(block_hdr, buf.blockSize, 1, in_file) != 1)
            {
                fprintf(stderr, "[E] File '%s' ends in the middle of a block\n", filename);
                free(block_hdr);
                block_hdr = NULL;
            }
        }
        else
        {
            fseeko(in_file, position + buf.blockSize, SEEK_SET);
        }
        
        /* we are at the same position as before, so abort */
        if(position == ftello(in_file))
        {
            fprintf(stderr, "[E] File '%s' has invalid blocks\n", filename);
            break;
        }
    }
    while(!feof(in_file));
    
    fclose(in_file);
    
    return block_hdr;
}

void save_index(char *base_filename, mlv_file_hdr_t *ref_file_hdr, int fileCount, frame_xref_t *index, int entries)
{
    char filename[128];
    FILE *out_file = NULL;

    strncpy(filename, base_filename, sizeof(filename));
    strcpy(&filename[strlen(filename) - 3], "IDX");
    
    out_file = fopen(filename, "wb+");
    
    if(!out_file)
    {
        fprintf(stderr, "[E] Failed writing into output file\n");
        return;
    }

    printf("[i] File %s opened for writing\n", filename);
    
    
    /* first write MLVI header */
    mlv_file_hdr_t file_hdr = *ref_file_hdr;
    
    /* update fields */
    file_hdr.blockSize = sizeof(mlv_file_hdr_t);
    file_hdr.videoFrameCount = 0;
    file_hdr.audioFrameCount = 0;
    file_hdr.fileNum = fileCount + 1;
    
    fwrite(&file_hdr, sizeof(mlv_file_hdr_t), 1, out_file);

    
    /* now write XREF block */
    mlv_xref_hdr_t hdr;
    
    memset(&hdr, 0x00, sizeof(mlv_vidf_hdr_t));
    memcpy(hdr.blockType, "XREF", 4);
    hdr.blockSize = sizeof(mlv_xref_hdr_t) + entries * sizeof(mlv_xref_t);
    hdr.entryCount = entries;
    
    if(fwrite(&hdr, sizeof(mlv_xref_hdr_t), 1, out_file) != 1)
    {
        fprintf(stderr, "[E] Failed writing into output file\n");
        fclose(out_file);
        return;
    }
    
    /* and then the single entries */
    for(int entry = 0; entry < entries; entry++)
    {
        mlv_xref_t field;
        
        memset(&field, 0x00, sizeof(mlv_xref_t));
        
        field.frameOffset = index[entry].frameOffset;
        field.fileNumber = index[entry].fileNumber;
        
        if(fwrite(&field, sizeof(mlv_xref_t), 1, out_file) != 1)
        {
            fprintf(stderr, "[E] Failed writing into output file\n");
            fclose(out_file);
            return;
        }
    }
    
    fclose(out_file);
}


FILE **load_all_chunks(char *base_filename, int *entries)
{
    int seq_number = 0;
    char filename[128];
    
    strncpy(filename, base_filename, sizeof(filename));
    FILE **files = malloc(sizeof(FILE*));
    
    files[0] = fopen(filename, "rb");
    if(!files[0])
    {
        return NULL;
    }
    
    printf("[i] File %s opened\n", filename);
    
    (*entries)++;
    while(seq_number < 99)
    {
        files = realloc(files, (*entries + 1) * sizeof(FILE*));
        
        /* check for the next file M00, M01 etc */
        char seq_name[3];

        sprintf(seq_name, "%02d", seq_number);
        seq_number++;

        strcpy(&filename[strlen(filename) - 2], seq_name);

        /* try to open */
        files[*entries] = fopen(filename, "rb");
        if(files[*entries])
        {
            printf("[i] File %s opened\n", filename);
            (*entries)++;
        }
        else
        {
            break;
        }
    }
    return files;
}

void show_usage(char *executable)
{
    fprintf(stderr, "Usage: %s [-o output_file] [-rscd] [-l compression_level(0-9)] <inputfile>\n", executable);
    fprintf(stderr, "Parameters:\n");
    fprintf(stderr, " -o output_file      set the filename to write into\n");
    fprintf(stderr, " -v                  verbose output\n");
    fprintf(stderr, " -r                  output into a legacy raw file for e.g. raw2dng\n");
    fprintf(stderr, " -b bits             convert image data to given bit depth per channel (1-16)\n");
    fprintf(stderr, " -z bits             zero the lowest bits, so we have only specified number of bits containing data (1-16) (improves compression rate)\n");
    fprintf(stderr, " -f frames           stop after that number of frames\n");
    
    fprintf(stderr, " -x                  build xref file (indexing)\n");
    
    fprintf(stderr, " -m                  write only metadata, no audio or video frames\n");
    fprintf(stderr, " -n                  write no metadata, only audio and video frames\n");
    
    fprintf(stderr, " -a                  average all frames in <inputfile> and output a single-frame MLV from it\n");
    fprintf(stderr, " -s mlv_file         subtract the reference frame in given file from every single frame during processing\n");
    
    fprintf(stderr, " -e                  delta-encode frames to improve compression, but lose random access capabilities\n");
    
    /* yet unclear which format to choose, so keep that as reminder */
    //fprintf(stderr, " -u lut_file         look-up table with 4 * xRes * yRes 16-bit words that is applied before bit depth conversion\n");
#ifdef MLV_USE_LZMA
    fprintf(stderr, " -c                  (re-)compress video and audio frames using LZMA (set bpp to 16 to improve compression rate)\n");
    fprintf(stderr, " -d                  decompress compressed video and audio frames using LZMA\n");
    fprintf(stderr, " -l level            set compression level from 0=fastest to 9=best compression\n");
#else
    fprintf(stderr, " -c, -d, -l          NOT AVAILABLE: compression support was not compiled into this release\n");
#endif
}

int main (int argc, char *argv[])
{
    char *input_filename = NULL;
    char *output_filename = NULL;
    char *subtract_filename = NULL;
    char *lut_filename = NULL;
    int blocks_processed = 0;
    
    int frame_limit = 0;
    int vidf_frames_processed = 0;
    int vidf_max_number = 0;
    
    int delta_encode_mode = 0;
    int xref_mode = 0;
    int average_mode = 0;
    int subtract_mode = 0;
    int no_metadata_mode = 0;
    int only_metadata_mode = 0;
    int average_samples = 0;
    
    int mlv_output = 0;
    int raw_output = 0;
    int bit_depth = 0;
    int bit_zap = 0;
    int compress_output = 0;
    int decompress_output = 0;
    int verbose = 0;
    int lzma_level = 5;
    char opt = ' ';
    
    int video_xRes = 0;
    int video_yRes = 0;
    
#ifdef MLV_USE_LZMA
    /* this may need some tuning */
    int lzma_dict = 1<<27;
    int lzma_lc = 0;
    int lzma_lp = 1;
    int lzma_pb = 1;
    int lzma_fb = 16;
    int lzma_threads = 8;
#endif
    
    printf("\n"); 
    printf(" MLV Dumper v1.0\n"); 
    printf("-----------------\n"); 
    printf("\n"); 
    
    if(sizeof(mlv_file_hdr_t) != 52)
    {
        printf("Error: Your compiler setup is weird. sizeof(mlv_file_hdr_t) is %d on your machine. Expected: 52\n", sizeof(mlv_file_hdr_t));
        return 0;
    }
    
    while ((opt = getopt(argc, argv, "xz:emnas:uvrcdo:l:b:f:")) != -1) 
    {
        switch (opt)
        {
            case 'x':
                xref_mode = 1;
                break;
                
            case 'm':
                only_metadata_mode = 1;
                break;
                
            case 'n':
                no_metadata_mode = 1;
                break;
                
            case 'e':
                delta_encode_mode = 1;
                break;
                
            case 'a':
                average_mode = 1;
                decompress_output = 1;
                no_metadata_mode = 1;
                break;
                
            case 's':
                if(!optarg)
                {
                    fprintf(stderr, "Error: Missing subtract frame filename\n");
                    return 0;
                }
                subtract_filename = strdup(optarg);
                subtract_mode = 1;
                decompress_output = 1;
                break;
                
            case 'u':
                if(!optarg)
                {
                    fprintf(stderr, "Error: Missing LUT filename\n");
                    return 0;
                }
                lut_filename = strdup(optarg);
                break;
                
            case 'v':
                verbose = 1;
                break;
                
            case 'r':
                raw_output = 1;
                bit_depth = 14;
                break;
                
            case 'c':
#ifdef MLV_USE_LZMA
                compress_output = 1;
#else
                fprintf(stderr, "Error: Compression support was not compiled into this release\n");
                return 0;
#endif                
                break;
                
            case 'd':
#ifdef MLV_USE_LZMA
                decompress_output = 1;
#else
                fprintf(stderr, "Error: Compression support was not compiled into this release\n");
                return 0;
#endif                
                break;
                
            case 'o':
                if(!optarg)
                {
                    fprintf(stderr, "Error: Missing output filename\n");
                    return 0;
                }
                output_filename = strdup(optarg);
                break;
                
            case 'l':
                lzma_level = MIN(9, MAX(0, atoi(optarg)));
                break;
                
            case 'f':
                frame_limit = MAX(0, atoi(optarg));
                break;
                
            case 'b':
                if(!raw_output)
                {
                    bit_depth = MIN(16, MAX(1, atoi(optarg)));
                }
                break;
                
            case 'z':
                if(!raw_output)
                {
                    bit_zap = MIN(16, MAX(1, atoi(optarg)));
                }
                break;
                
            default:
                show_usage(argv[0]);
                return 0;
        }
    }   
    
    if(optind >= argc)
    {
        fprintf(stderr, "Error: Missing input filename\n");
        show_usage(argv[0]);
        return 0;
    }
    
    
    /* get first file */
    input_filename = argv[optind];
    
    printf("[i] Mode of operation:\n"); 
    printf("   - Input MLV file: '%s'\n", input_filename); 
    
    if(verbose)
    {
        printf("   - Verbose messages\n"); 
    }
    
    /* display and set/unset variables according to parameters to have a consistent state */
    if(output_filename)
    {
        if(raw_output)
        {
            printf("   - Convert to legacy RAW\n"); 
            
            delta_encode_mode = 0;
            compress_output = 0;
            mlv_output = 0;
            if(average_mode)
            {
                printf("   - disabled average mode, not possible\n");
                average_mode = 0;
            }
        }
        else
        {
            mlv_output = 1;
            printf("   - Rewrite MLV\n"); 
            if(bit_zap)
            {
                printf("   - Only store %d bits of information per pixel\n", bit_zap); 
            }
            if(bit_depth)
            {
                printf("   - Convert to %d bpp\n", bit_depth); 
            }
            if(delta_encode_mode)
            {
                printf("   - Only store changes to previous frame\n"); 
            }
            if(compress_output)
            {
                printf("   - Compress frame data\n"); 
            }
            if(average_mode)
            {
                printf("   - Output only one frame with averaged pixel values\n");
                subtract_mode = 0;
            }
            if(subtract_mode)
            {
                printf("   - Subtract reference frame '%s' from single images\n", subtract_filename); 
            }
        }
        
        printf("   - Output into '%s'\n", output_filename); 
    }
    else
    {
        /* those dont make sense then */
        raw_output = 0;
        compress_output = 0;
        
        printf("   - Verify file structure\n"); 
        if(verbose)
        {
            printf("   - Dump all block information\n"); 
        }
    }
    
    if(xref_mode)
    {
        printf("   - Output .idx file for faster processing\n"); 
    }

    /* start processing */
    lv_rec_file_footer_t lv_rec_footer;
    mlv_file_hdr_t main_header;
    
    /* this table contains the XREF chunk read from idx file, if existing */
    mlv_xref_hdr_t *block_xref = NULL;
    int block_xref_pos = 0;

    uint32_t frame_buffer_size = 32*1024*1024;
    
    uint32_t *frame_arith_buffer = NULL;
    uint8_t *frame_buffer = NULL;
    uint8_t *prev_frame_buffer = NULL;
    
    FILE *out_file = NULL;
    FILE **in_files = NULL;
    FILE *in_file = NULL;
    
    int in_file_count = 0;
    int in_file_num = 0;

    /* this is for our generated XREF table */
    frame_xref_t *frame_xref_table = NULL;
    int frame_xref_allocated = 0;
    int frame_xref_entries = 0;

    /* initialize stuff */
    memset(&lv_rec_footer, 0x00, sizeof(lv_rec_file_footer_t));
    memset(&main_header, 0x00, sizeof(mlv_file_hdr_t));

    /* open files */
    in_files = load_all_chunks(input_filename, &in_file_count);
    if(!in_files || !in_file_count)
    {
        fprintf(stderr, "[E] Failed to open file '%s'\n", input_filename);
        return 0;
    }
    else
    {
        in_file_num = 0;
        in_file = in_files[in_file_num];
    }
    
    if(!xref_mode)
    {
        block_xref = load_index(input_filename);
        
        if(block_xref)
        {   
            printf("[i] XREF table contains %d entries\n", block_xref->entryCount);
        }
        else
        {
            if(delta_encode_mode)
            {
                fprintf(stderr, "[E] Delta encoding is not possible without an index file. Please create one using -x option.\n");
                return 0;
            }
        }
    }
    
    if(average_mode || subtract_mode)
    {
        frame_arith_buffer = malloc(frame_buffer_size);
        if(!frame_arith_buffer)
        {
            fprintf(stderr, "[E] Failed to alloc mem\n");
            return 0;
        }
        memset(frame_arith_buffer, 0x00, frame_buffer_size);
    }
    
    if(subtract_mode)
    {
        int ret = load_frame(subtract_filename, 0, (uint8_t*)frame_arith_buffer);
        
        if(ret)
        {
            fprintf(stderr, "[E] Failed to load subtract frame (%d)\n", ret);
            return 0;
        }
    }
    
    //if(delta_encode_mode)
    {
        prev_frame_buffer = malloc(frame_buffer_size);
        if(!prev_frame_buffer)
        {
            fprintf(stderr, "[E] Failed to alloc mem\n");
            return 0;
        }
        memset(prev_frame_buffer, 0x00, frame_buffer_size);
    }
    
    if(output_filename)
    {
        frame_buffer = malloc(frame_buffer_size);
        if(!frame_buffer)
        {
            fprintf(stderr, "[E] Failed to alloc mem\n");
            return 0;
        }
        
        out_file = fopen(output_filename, "wb+");
        if(!out_file)
        {
            fprintf(stderr, "[E] Failed to open file '%s'\n", output_filename);
            return 0;
        }
    }

    printf("[i] Processing...\n"); 
    do
    {
        mlv_hdr_t buf;
        uint64_t position = 0;
        
read_headers:

        if(block_xref)
        {
            block_xref_pos++;
            if(block_xref_pos >= block_xref->entryCount)
            {
                printf("[i] Reached end of all files after %i blocks\n", blocks_processed);
                break;
            }
            
            /* get the file and position of the next block */
            in_file_num = ((mlv_xref_t*)&block_xref->xrefEntries)[block_xref_pos].fileNumber;
            position = ((mlv_xref_t*)&block_xref->xrefEntries)[block_xref_pos].frameOffset;
            
            /* select file and seek to the right position */
            in_file = in_files[in_file_num];
            fseeko(in_file, position, SEEK_SET);
        }
        
        position = ftello(in_file);
        
        if(fread(&buf, sizeof(mlv_hdr_t), 1, in_file) != 1)
        {
            if(block_xref)
            {
                printf("[i] Reached EOF of chunk %d/%d after %i blocks total. This should never happen or your index file is wrong.\n", in_file_num, in_file_count, blocks_processed);
                break;
            }
            printf("[i] Reached end of chunk %d/%d after %i blocks\n", in_file_num, in_file_count, blocks_processed);
            
            if(in_file_num < (in_file_count - 1))
            {
                in_file_num++;
                in_file = in_files[in_file_num];
            }
            else
            {
                break;
            }
            
            blocks_processed = 0;

            goto read_headers;
        }
        
        /* jump back to the beginning of the block just read */
        fseeko(in_file, position, SEEK_SET);

        position = ftello(in_file);
        
        /* file header */
        if(!memcmp(buf.blockType, "MLVI", 4))
        {
            mlv_file_hdr_t file_hdr;
            uint32_t hdr_size = MIN(sizeof(mlv_file_hdr_t), buf.blockSize);

            /* read the whole header block, but limit size to either our local type size or the written block size */
            if(fread(&file_hdr, hdr_size, 1, in_file) != 1)
            {
                fprintf(stderr, "[E] File ends in the middle of a block\n");
                goto abort;
            }
            fseeko(in_file, position + file_hdr.blockSize, SEEK_SET);

            if(verbose)
            {
                printf("File Header (MLVI)\n");
                printf("    Size        : 0x%08X\n", file_hdr.blockSize);
                printf("    Ver         : %s\n", file_hdr.versionString);
                printf("    GUID        : %08" PRIu64 "\n", file_hdr.fileGuid);
                printf("    FPS         : %f\n", (double)file_hdr.sourceFpsNom / (double)file_hdr.sourceFpsDenom);
                printf("    File        : %d / %d\n", file_hdr.fileNum, file_hdr.fileCount);
                printf("    Frames Video: %d\n", file_hdr.videoFrameCount);
                printf("    Frames Audio: %d\n", file_hdr.audioFrameCount);
            }

            /* is this the first file? */
            if(file_hdr.fileNum == 0)
            {
                /* in xref mode, use every block and get its timestamp etc */
                if(xref_mode)
                {
                    xref_resize(&frame_xref_table, frame_xref_entries + 1, &frame_xref_allocated);
                    
                    /* add xref data */
                    frame_xref_table[frame_xref_entries].frameTime = 0;
                    frame_xref_table[frame_xref_entries].frameOffset = position;
                    frame_xref_table[frame_xref_entries].fileNumber = in_file_num;
                    
                    frame_xref_entries++;
                }
                
                memcpy(&main_header, &file_hdr, sizeof(mlv_file_hdr_t));
                
                if(mlv_output)
                {
                    /* correct header size if needed */
                    file_hdr.blockSize = sizeof(mlv_file_hdr_t);
                    
                    if(average_mode)
                    {
                        file_hdr.videoFrameCount = 1;
                    }
                    
                    /* set the output compression flag */
                    if(compress_output)
                    {
                        file_hdr.videoClass |= MLV_VIDEO_CLASS_FLAG_LZMA;
                    }
                    else
                    {
                        file_hdr.videoClass &= ~MLV_VIDEO_CLASS_FLAG_LZMA;
                    }
                    
                    if(delta_encode_mode)
                    {
                        file_hdr.videoClass |= MLV_VIDEO_CLASS_FLAG_DELTA;
                    }
                    else
                    {
                        file_hdr.videoClass &= ~MLV_VIDEO_CLASS_FLAG_DELTA;
                    }
                    
                    if(fwrite(&file_hdr, file_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else
            {
                /* no, its another chunk */
                if(main_header.fileGuid != file_hdr.fileGuid)
                {
                    printf("Error: GUID within the file chunks mismatch!\n");
                    break;
                }
            }
            
            if(raw_output)
            {
                lv_rec_footer.frameCount += file_hdr.videoFrameCount;
                lv_rec_footer.sourceFpsx1000 = (double)file_hdr.sourceFpsNom / (double)file_hdr.sourceFpsDenom * 1000;
                lv_rec_footer.frameSkip = 0;
            }
        }
        else
        {
            /* in xref mode, use every block and get its timestamp etc */
            if(xref_mode)
            {
                xref_resize(&frame_xref_table, frame_xref_entries + 1, &frame_xref_allocated);
                
                /* add xref data */
                frame_xref_table[frame_xref_entries].frameTime = buf.timestamp;
                frame_xref_table[frame_xref_entries].frameOffset = position;
                frame_xref_table[frame_xref_entries].fileNumber = in_file_num;
                
                frame_xref_entries++;
            }
                
            if(main_header.blockSize == 0)
            {
                fprintf(stderr, "[E] Missing file header\n");
                goto abort;
            }
            
            if(verbose)
            {
                printf("Block: %c%c%c%c\n", buf.blockType[0], buf.blockType[1], buf.blockType[2], buf.blockType[3]);
                printf("  Offset: 0x%08" PRIx64 "\n", position);
                printf("    Size: %d\n", buf.blockSize);

                /* NULL blocks don't have timestamps */
                if(memcmp(buf.blockType, "NULL", 4))
                {
                    printf("    Time: %f ms\n", (double)buf.timestamp / 1000.0f);
                }
            }

            if(!memcmp(buf.blockType, "VIDF", 4))
            {
                mlv_vidf_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_vidf_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }

                if(verbose)
                {
                    printf("   Frame: #%d\n", block_hdr.frameNumber);
                    printf("    Crop: %dx%d\n", block_hdr.cropPosX, block_hdr.cropPosY);
                    printf("     Pan: %dx%d\n", block_hdr.panPosX, block_hdr.panPosY);
                    printf("   Space: %d\n", block_hdr.frameSpace);
                }
                
                if(raw_output || mlv_output)
                {
                    /* if already compressed, we have to decompress it first */
                    int compressed = main_header.videoClass & MLV_VIDEO_CLASS_FLAG_LZMA;
                    int recompress = compressed && compress_output;
                    int decompress = compressed && decompress_output;
                    
                    int frame_size = block_hdr.blockSize - sizeof(mlv_vidf_hdr_t) - block_hdr.frameSpace;
                    int prev_frame_size = frame_size;
                    
                    fseeko(in_file, block_hdr.frameSpace, SEEK_CUR);
                    if(fread(frame_buffer, frame_size, 1, in_file) != 1)
                    {
                        fprintf(stderr, "[E] File ends in the middle of a block\n");
                        goto abort;
                    }
                    
                    if(recompress || decompress || (raw_output && compressed))
                    {
#ifdef MLV_USE_LZMA
                        size_t lzma_out_size = *(uint32_t *)frame_buffer;
                        size_t lzma_in_size = frame_size - LZMA_PROPS_SIZE - 4;
                        size_t lzma_props_size = LZMA_PROPS_SIZE;
                        unsigned char *lzma_out = malloc(lzma_out_size);

                        int ret = LzmaUncompress(
                            lzma_out, &lzma_out_size, 
                            (unsigned char *)&frame_buffer[4 + LZMA_PROPS_SIZE], &lzma_in_size, 
                            (unsigned char *)&frame_buffer[4], lzma_props_size
                            );

                        if(ret == SZ_OK)
                        {
                            frame_size = lzma_out_size;
                            memcpy(frame_buffer, lzma_out, frame_size);
                            if(verbose)
                            {
                                printf("    LZMA: %d -> %d  (%2.2f%%)\n", lzma_in_size, lzma_out_size, ((float)lzma_out_size * 100.0f) / (float)lzma_in_size);
                            }
                        }
                        else
                        {
                            printf("    LZMA: Failed (%d)\n", ret);
                            goto abort;
                        }
#else
                        printf("    LZMA: not compiled into this release, aborting.\n");
                        goto abort;
#endif
                    }
                    
                    int old_depth = lv_rec_footer.raw_info.bits_per_pixel;
                    int new_depth = bit_depth;
                    
                    /* this value changes in this context */
                    int current_depth = old_depth;
                    
                    /* in average mode, sum up all pixel values of a pixel position */
                    if(average_mode)
                    {
                        int pitch = video_xRes * lv_rec_footer.raw_info.bits_per_pixel / 8;
                        
                        for(int y = 0; y < video_yRes; y++)
                        {
                            uint16_t *src_line = (uint16_t *)&frame_buffer[y * pitch];
                            
                            for(int x = 0; x < video_xRes; x++)
                            {
                                uint16_t value = bitextract(src_line, x, lv_rec_footer.raw_info.bits_per_pixel);
                                
                                frame_arith_buffer[y * video_xRes + x] += value;
                            }
                        }
                        
                        average_samples++;
                    }
                    
                    /* in subtract mode, subtrace reference frame */
                    if(subtract_mode)
                    {
                        int pitch = video_xRes * lv_rec_footer.raw_info.bits_per_pixel / 8;
                        
                        for(int y = 0; y < video_yRes; y++)
                        {
                            uint16_t *src_line = (uint16_t *)&frame_buffer[y * pitch];
                            uint16_t *sub_line = (uint16_t *)&((uint8_t*)frame_arith_buffer)[y * pitch];
                            
                            for(int x = 0; x < video_xRes; x++)
                            {
                                int32_t value = bitextract(src_line, x, lv_rec_footer.raw_info.bits_per_pixel);
                                int32_t sub_value = bitextract(sub_line, x, lv_rec_footer.raw_info.bits_per_pixel);
                                
                                value -= sub_value;
                                value += lv_rec_footer.raw_info.black_level;
                                value = COERCE(value, lv_rec_footer.raw_info.black_level, lv_rec_footer.raw_info.white_level);
                                
                                bitinsert(src_line, x, lv_rec_footer.raw_info.bits_per_pixel, value);
                            }
                        }
                    }
                    
                    /* now resample bit depth if requested */
                    if(new_depth && (old_depth != new_depth))
                    {
                        int new_size = (video_xRes * video_yRes * new_depth + 7) / 8;
                        unsigned char *new_buffer = malloc(new_size);
                        
                        if(verbose)
                        {
                            printf("   depth: %d -> %d, size: %d -> %d (%2.2f%%)\n", old_depth, new_depth, frame_size, new_size, ((float)new_depth * 100.0f) / (float)old_depth);
                        }
                        
                        int calced_size = ((video_xRes * video_yRes * old_depth + 7) / 8);
                        if(calced_size > frame_size)
                        {
                            printf("Error: old frame size is too small for %dx%d at %d bpp. Input data corrupt. (%d < %d)\n", video_xRes, video_yRes, old_depth, frame_size, calced_size);
                            break;
                        }

                        int old_pitch = video_xRes * old_depth / 8;
                        int new_pitch = video_xRes * new_depth / 8;
                        
                        for(int y = 0; y < video_yRes; y++)
                        {
                            uint16_t *src_line = (uint16_t *)&frame_buffer[y * old_pitch];
                            uint16_t *dst_line = (uint16_t *)&new_buffer[y * new_pitch];
                            
                            for(int x = 0; x < video_xRes; x++)
                            {
                                uint16_t value = bitextract(src_line, x, old_depth);
                                
                                /* normalize the old value to 16 bits */
                                value <<= (16-old_depth);
                                
                                /* convert the old value to destination depth */
                                value >>= (16-new_depth);
                                
                                bitinsert(dst_line, x, new_depth, value);
                            }
                        }
                        
                        frame_size = new_size;
                        current_depth = new_depth;
                        
                        memcpy(frame_buffer, new_buffer, frame_size);
                        free(new_buffer);
                    }
                    
                    if(bit_zap)
                    {
                        int pitch = video_xRes * current_depth / 8;
                        uint32_t mask = ~((1 << (16 - bit_zap)) - 1);
                        
                        for(int y = 0; y < video_yRes; y++)
                        {
                            uint16_t *src_line = (uint16_t *)&frame_buffer[y * pitch];
                            
                            for(int x = 0; x < video_xRes; x++)
                            {
                                int32_t value = bitextract(src_line, x, current_depth);
                                
                                /* normalize the old value to 16 bits */
                                value <<= (16-current_depth);
                                
                                value &= mask;
                                
                                /* convert the old value to destination depth */
                                value >>= (16-current_depth);
                                
                                
                                bitinsert(src_line, x, current_depth, value);
                            }
                        }
                    }
                    
                    if(delta_encode_mode)
                    {
                        /* only delta encode, if not already encoded */
                        if(!(main_header.videoClass & MLV_VIDEO_CLASS_FLAG_DELTA))
                        {
                            uint8_t *current_frame_buffer = malloc(frame_size);
                            int pitch = video_xRes * current_depth / 8;
                            
                            /* backup current frame for later */
                            memcpy(current_frame_buffer, frame_buffer, frame_size);
                            
                            for(int y = 0; y < video_yRes; y++)
                            {
                                uint16_t *src_line = (uint16_t *)&frame_buffer[y * pitch];
                                uint16_t *ref_line = (uint16_t *)&prev_frame_buffer[y * pitch];
                                int32_t offset = 1 << (current_depth - 1);
                                int32_t max_val = (1 << current_depth) - 1;
                                
                                for(int x = 0; x < video_xRes; x++)
                                {
                                    int32_t value = bitextract(src_line, x, current_depth);
                                    int32_t ref_value = bitextract(ref_line, x, current_depth);
                                    
                                    /* when e.g. using 16 bit values:
                                           delta =  1      -> encode to 0x8001
                                           delta =  0      -> encode to 0x8000
                                           delta = -1      -> encode to 0x7FFF
                                           delta = -0xFFFF -> encode to 0x0001
                                           delta =  0xFFFF -> encode to 0x7FFF
                                       so this is basically a signed int with overflow and a max/2 offset.
                                       this offset makes the frames uniform grey when viewing non-decoded frames and improves compression rate a bit.
                                    */
                                    int32_t delta = offset + value - ref_value;
                                    
                                    uint16_t new_value = (uint16_t)(delta & max_val);
                                    
                                    bitinsert(src_line, x, current_depth, new_value);
                                }
                            }
                            
                            /* save current original frame to prev buffer */
                            memcpy(prev_frame_buffer, current_frame_buffer, frame_size);
                            free(current_frame_buffer);
                        }
                    }
                    else
                    {
                        /* delta decode, if input data is encoded */
                        if(main_header.videoClass & MLV_VIDEO_CLASS_FLAG_DELTA)
                        {
                            int pitch = video_xRes * current_depth / 8;
                           
                            for(int y = 0; y < video_yRes; y++)
                            {
                                uint16_t *src_line = (uint16_t *)&frame_buffer[y * pitch];
                                uint16_t *ref_line = (uint16_t *)&prev_frame_buffer[y * pitch];
                                int32_t offset = 1 << (current_depth - 1);
                                int32_t max_val = (1 << current_depth) - 1;
                                
                                for(int x = 0; x < video_xRes; x++)
                                {
                                    int32_t value = bitextract(src_line, x, current_depth);
                                    int32_t ref_value = bitextract(ref_line, x, current_depth);
                                    
                                    /* when e.g. using 16 bit values:
                                           delta =  1      -> encode to 0x8001
                                           delta =  0      -> encode to 0x8000
                                           delta = -1      -> encode to 0x7FFF
                                           delta = -0xFFFF -> encode to 0x0001
                                           delta =  0xFFFF -> encode to 0x7FFF
                                       so this is basically a signed int with overflow and a max/2 offset.
                                       this offset makes the frames uniform grey when viewing non-decoded frames and improves compression rate a bit.
                                    */
                                    int32_t delta = offset + value + ref_value;
                                    
                                    uint16_t new_value = (uint16_t)(delta & max_val);
                                    
                                    bitinsert(src_line, x, current_depth, new_value);
                                }
                            }
                            
                            /* save current original frame to prev buffer */
                            memcpy(prev_frame_buffer, frame_buffer, frame_size);
                        }
                    }
                    
                    if(raw_output)
                    {
                        if(!lv_rec_footer.frameSize)
                        {
                            lv_rec_footer.frameSize = frame_size;
                        }

                        fseeko(out_file, (uint64_t)block_hdr.frameNumber * (uint64_t)frame_size, SEEK_SET);
                        fwrite(frame_buffer, frame_size, 1, out_file);
                    }
                    
                    if(mlv_output && !only_metadata_mode && !average_mode)
                    {
                        if(compress_output)
                        {
#ifdef MLV_USE_LZMA
                            size_t lzma_out_size = 2 * frame_size;
                            size_t lzma_in_size = frame_size;
                            size_t lzma_props_size = LZMA_PROPS_SIZE;
                            unsigned char *lzma_out = malloc(lzma_out_size + LZMA_PROPS_SIZE);

                            int ret = LzmaCompress(
                                &lzma_out[LZMA_PROPS_SIZE], &lzma_out_size, 
                                (unsigned char *)frame_buffer, lzma_in_size, 
                                &lzma_out[0], &lzma_props_size, 
                                lzma_level, lzma_dict, lzma_lc, lzma_lp, lzma_pb, lzma_fb, lzma_threads
                                );

                            if(ret == SZ_OK)
                            {
                                /* store original frame size */
                                *(uint32_t *)frame_buffer = frame_size;
                                
                                /* set new compressed size and copy buffers */
                                frame_size = lzma_out_size + LZMA_PROPS_SIZE + 4;
                                memcpy(&frame_buffer[4], lzma_out, frame_size - 4);
                                
                                if(verbose)
                                {
                                    printf("    LZMA: %d -> %d  (%2.2f%%)\n", lzma_in_size, frame_size, ((float)lzma_out_size * 100.0f) / (float)lzma_in_size);
                                }
                            }
                            else
                            {
                                printf("    LZMA: Failed (%d)\n", ret);
                                goto abort;
                            }
                            free(lzma_out);
#else
                            printf("    LZMA: not compiled into this release, aborting.\n");
                            goto abort;
#endif
                        }
                        
                        if(frame_size != prev_frame_size)
                        {
                            printf("  saving: %d -> %d  (%2.2f%%)\n", prev_frame_size, frame_size, ((float)frame_size * 100.0f) / (float)prev_frame_size);
                        }
                        
                        /* delete free space and correct header size if needed */
                        block_hdr.blockSize = sizeof(mlv_vidf_hdr_t) + frame_size;
                        block_hdr.frameSpace = 0;
                        
                        if(fwrite(&block_hdr, sizeof(mlv_vidf_hdr_t), 1, out_file) != 1)
                        {
                            fprintf(stderr, "[E] Failed writing into output file\n");
                            goto abort;
                        }
                        if(fwrite(frame_buffer, frame_size, 1, out_file) != 1)
                        {
                            fprintf(stderr, "[E] Failed writing into output file\n");
                            goto abort;
                        }
                    }
                }
                else
                {
                    fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);
                }

                vidf_max_number = MAX(vidf_max_number, block_hdr.frameNumber);
                
                vidf_frames_processed++;
                if(frame_limit && vidf_frames_processed > frame_limit)
                {
                    printf("[i] Reached limit of %i frames\n", frame_limit);
                    break;
                }
            }
            else if(!memcmp(buf.blockType, "LENS", 4))
            {
                mlv_lens_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_lens_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);

                if(verbose)
                {
                    printf("     Name:        '%s'\n", block_hdr.lensName);
                    printf("     Serial:      '%s'\n", block_hdr.lensSerial);
                    printf("     Focal Len:   %d mm\n", block_hdr.focalLength);
                    printf("     Focus Dist:  %d mm\n", block_hdr.focalDist);
                    printf("     Aperture:    f/%.2f\n", (double)block_hdr.aperture / 100.0f);
                    printf("     IS Mode:     %d\n", block_hdr.stabilizerMode);
                    printf("     AF Mode:     %d\n", block_hdr.autofocusMode);
                    printf("     Lens ID:     0x%08X\n", block_hdr.lensID);
                    printf("     Flags:       0x%08X\n", block_hdr.flags);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_lens_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "INFO", 4))
            {
                mlv_info_hdr_t block_hdr;
                int32_t hdr_size = MIN(sizeof(mlv_info_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }

                /* get the string length and malloc a buffer for that string */
                int str_length = block_hdr.blockSize - hdr_size;

                if(str_length)
                {
                    char *buf = malloc(str_length + 1);

                    if(fread(buf, str_length, 1, in_file) != 1)
                    {
                        fprintf(stderr, "[E] File ends in the middle of a block\n");
                        goto abort;
                    }

                    if(verbose)
                    {
                        buf[str_length] = '\000';
                        printf("     String:   '%s'\n", buf);
                    }
                    
                    /* only output this block if there is any data */
                    if(mlv_output && !no_metadata_mode)
                    {
                        /* correct header size if needed */
                        block_hdr.blockSize = sizeof(mlv_info_hdr_t) + str_length;
                        if(fwrite(&block_hdr, sizeof(mlv_info_hdr_t), 1, out_file) != 1)
                        {
                            fprintf(stderr, "[E] Failed writing into output file\n");
                            goto abort;
                        }
                        if(fwrite(buf, str_length, 1, out_file) != 1)
                        {
                            fprintf(stderr, "[E] Failed writing into output file\n");
                            goto abort;
                        }
                    }

                    free(buf);
                }
            }
            else if(!memcmp(buf.blockType, "ELVL", 4))
            {
                mlv_elvl_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_elvl_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);

                if(verbose)
                {
                    printf("     Roll:    %2.2f\n", (double)block_hdr.roll / 100.0f);
                    printf("     Pitch:   %2.2f\n", (double)block_hdr.pitch / 100.0f);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_elvl_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "STYL", 4))
            {
                mlv_styl_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_styl_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);

                if(verbose)
                {
                    printf("     picStyle:   %d\n", block_hdr.picStyle);
                    printf("     contrast:   %d\n", block_hdr.contrast);
                    printf("     sharpness:  %d\n", block_hdr.sharpness);
                    printf("     saturation: %d\n", block_hdr.saturation);
                    printf("     colortone:  %d\n", block_hdr.colortone);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_styl_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "WBAL", 4))
            {
                mlv_wbal_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_wbal_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);

                if(verbose)
                {
                    printf("     Mode:   %d\n", block_hdr.wb_mode);
                    printf("     Kelvin:   %d\n", block_hdr.kelvin);
                    printf("     Gain R:   %d\n", block_hdr.wbgain_r);
                    printf("     Gain G:   %d\n", block_hdr.wbgain_g);
                    printf("     Gain B:   %d\n", block_hdr.wbgain_b);
                    printf("     Shift GM:   %d\n", block_hdr.wbs_gm);
                    printf("     Shift BA:   %d\n", block_hdr.wbs_ba);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_wbal_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "IDNT", 4))
            {
                mlv_idnt_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_idnt_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);

                if(verbose)
                {
                    printf("     Camera Name:   '%s'\n", block_hdr.cameraName);
                    printf("     Camera Serial: '%s'\n", block_hdr.cameraSerial);
                    printf("     Camera Model:  0x%08X\n", block_hdr.cameraModel);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_idnt_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "RTCI", 4))
            {
                mlv_rtci_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_rtci_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);

                if(verbose)
                {
                    printf("     Date:        %02d.%02d.%04d\n", block_hdr.tm_mday, block_hdr.tm_mon, 1900 + block_hdr.tm_year);
                    printf("     Time:        %02d:%02d:%02d (GMT+%d)\n", block_hdr.tm_hour, block_hdr.tm_min, block_hdr.tm_sec, block_hdr.tm_gmtoff);
                    printf("     Zone:        '%s'\n", block_hdr.tm_zone);
                    printf("     Day of week: %d\n", block_hdr.tm_wday);
                    printf("     Day of year: %d\n", block_hdr.tm_yday);
                    printf("     Daylight s.: %d\n", block_hdr.tm_isdst);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_rtci_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "MARK", 4))
            {
                mlv_mark_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_mark_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);

                if(verbose)
                {
                    printf("  Button: 0x%02X\n", block_hdr.type);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_mark_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "EXPO", 4))
            {
                mlv_expo_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_expo_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);

                if(verbose)
                {
                    printf("     ISO Mode:   %d\n", block_hdr.isoMode);
                    printf("     ISO:        %d\n", block_hdr.isoValue);
                    printf("     ISO Analog: %d\n", block_hdr.isoAnalog);
                    printf("     ISO DGain:  %d/1024 EV\n", block_hdr.digitalGain);
                    printf("     Shutter:    %" PRIu64 " µs (1/%.2f)\n", block_hdr.shutterValue, 1000000.0f/(float)block_hdr.shutterValue);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_expo_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "RAWI", 4))
            {
                mlv_rawi_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_rawi_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    fprintf(stderr, "[E] File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);

                video_xRes = block_hdr.xRes;
                video_yRes = block_hdr.yRes;
                if(verbose)
                {
                    printf("    Res:  %dx%d\n", block_hdr.xRes, block_hdr.yRes);
                    printf("    raw_info:\n");
                    printf("      api_version      0x%08X\n", block_hdr.raw_info.api_version);
                    printf("      height           %d\n", block_hdr.raw_info.height);
                    printf("      width            %d\n", block_hdr.raw_info.width);
                    printf("      pitch            %d\n", block_hdr.raw_info.pitch);
                    printf("      frame_size       0x%08X\n", block_hdr.raw_info.frame_size);
                    printf("      bits_per_pixel   %d\n", block_hdr.raw_info.bits_per_pixel);
                    printf("      black_level      %d\n", block_hdr.raw_info.black_level);
                    printf("      white_level      %d\n", block_hdr.raw_info.white_level);
                    printf("      active_area.y1   %d\n", block_hdr.raw_info.active_area.y1);
                    printf("      active_area.x1   %d\n", block_hdr.raw_info.active_area.x1);
                    printf("      active_area.y2   %d\n", block_hdr.raw_info.active_area.y2);
                    printf("      active_area.x2   %d\n", block_hdr.raw_info.active_area.x2);
                    printf("      exposure_bias    %d, %d\n", block_hdr.raw_info.exposure_bias[0], block_hdr.raw_info.exposure_bias[1]);
                    printf("      cfa_pattern      0x%08X\n", block_hdr.raw_info.cfa_pattern);
                    printf("      calibration_ill  %d\n", block_hdr.raw_info.calibration_illuminant1);
                }

                /* cache these bits when we convert to legacy or resample bit depth */
                //if(raw_output || bit_depth)
                {
                    strncpy((char*)lv_rec_footer.magic, "RAWM", 4);
                    lv_rec_footer.xRes = block_hdr.xRes;
                    lv_rec_footer.yRes = block_hdr.yRes;
                    lv_rec_footer.raw_info = block_hdr.raw_info;
                }
            
                /* always output RAWI blocks, its not just metadata, but important frame format data */
                if(mlv_output /*&& !no_metadata_mode*/)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_rawi_hdr_t);
                    
                    if(bit_depth)
                    {
                        block_hdr.raw_info.bits_per_pixel = bit_depth;
                    }
                    
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        fprintf(stderr, "[E] Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else
            {
                printf("[i] Unknown Block: %c%c%c%c, skipping\n", buf.blockType[0], buf.blockType[1], buf.blockType[2], buf.blockType[3]);
                fseeko(in_file, position + buf.blockSize, SEEK_SET);
            }
        }
        
        /* count any read block, no matter if header or video frame */
        blocks_processed++;
    }
    while(!feof(in_file));
    
abort:

    printf("[i] Processed %d video frames\n", vidf_frames_processed);
    
    /* in average mode, finalize average calculation and output the resulting average */
    if(average_mode)
    {
        int new_pitch = video_xRes * lv_rec_footer.raw_info.bits_per_pixel / 8;
        for(int y = 0; y < video_yRes; y++)
        {
            uint16_t *dst_line = (uint16_t *)&frame_buffer[y * new_pitch];
            for(int x = 0; x < video_xRes; x++)
            {
                uint32_t value = frame_arith_buffer[y * video_xRes + x];
                
                value /= average_samples;
                bitinsert(dst_line, x, lv_rec_footer.raw_info.bits_per_pixel, value);
            }
        }
        
        int frame_size = ((video_xRes * video_yRes * lv_rec_footer.raw_info.bits_per_pixel + 7) / 8);
        
        mlv_vidf_hdr_t hdr;
        
        memset(&hdr, 0x00, sizeof(mlv_vidf_hdr_t));
        memcpy(hdr.blockType, "VIDF", 4);
        hdr.blockSize = sizeof(mlv_vidf_hdr_t) + frame_size;
        
        fwrite(&hdr, sizeof(mlv_vidf_hdr_t), 1, out_file);
        fwrite(frame_buffer, frame_size, 1, out_file);
    }

    if(raw_output)
    {
        lv_rec_footer.frameCount = vidf_max_number + 1;
        lv_rec_footer.raw_info.bits_per_pixel = 14;
        
        fseeko(out_file, 0, SEEK_END);
        fwrite(&lv_rec_footer, sizeof(lv_rec_file_footer_t), 1, out_file);
    }
    
    if(xref_mode)
    {
        printf("[i] XREF table contains %d entries\n", frame_xref_entries);
        xref_sort(frame_xref_table, frame_xref_entries);
        save_index(input_filename, &main_header, in_file_count, frame_xref_table, frame_xref_entries);
    }

    /* free list of input files */
    for(in_file_num = 0; in_file_num < in_file_count; in_file_num++)
    {
        fclose(in_files[in_file_num]);
    }
    free(in_files);
    
    if(out_file)
    {
        fclose(out_file);
    }

    /* passing NULL to free is absolutely legal, so no check required */
    free(lut_filename);
    free(subtract_filename);
    free(output_filename);
    free(prev_frame_buffer);
    free(frame_arith_buffer);
    free(block_xref);
    

    printf("[i] Done\n"); 
    printf("\n"); 

    return 0;
}