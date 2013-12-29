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
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>
#include <time.h>

/* dng related headers */
#include <chdk-dng.h>
#include "../dual_iso/wirth.h"  /* fast median, generic implementation (also kth_smallest) */
#include "../dual_iso/optmed.h" /* fast median for small common array sizes (3, 7, 9...) */

#ifdef __WIN32
#define FMT_SIZE "%u"
#else
#define FMT_SIZE "%zd"
#endif

#define ERR_OK              0
#define ERR_STRUCT_ALIGN    1
#define ERR_PARAM           2
#define ERR_FILE            3
#define ERR_INDEX_REQ       4
#define ERR_MALLOC          5

#if defined(USE_LUA)
#define LUA_LIB
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#else
typedef void lua_State;
#endif

/* based on http://www.lua.org/pil/25.3.html */
int32_t lua_call_va(lua_State *L, const char *func, const char *sig, ...)
{
    va_list vl;
    int narg, nres;  /* number of arguments and results */
    int verbose = 0;

    va_start(vl, sig);
    
#if defined(USE_LUA)
    lua_getglobal(L, func);  /* get function */

    /* push arguments */
    narg = 0;
    while (*sig)
    {  
        /* push arguments */
        switch (*sig++)
        {
            case 'v':
                verbose = 1;
                break;

            case 'd':  /* double argument */
                lua_pushnumber(L, va_arg(vl, double));
                break;

            case 'i':  /* int argument */
                lua_pushnumber(L, va_arg(vl, int));
                break;

            case 's':  /* string argument */
                lua_pushstring(L, va_arg(vl, char *));
                break;

            case 'l':  /* lstring argument */
            {
                char *ptr = va_arg(vl, char *);
                int len = va_arg(vl, int);
                lua_pushlstring(L, ptr, len);
                break;
            }

            case '>':
                goto endwhile;

            default:
                return -5;
        }
        narg++;
        luaL_checkstack(L, 1, "too many arguments");
    }
    endwhile:

    /* do the call */
    nres = strlen(sig);  /* number of expected results */
    int ret = lua_pcall(L, narg, nres, 0);
    
    if (ret != 0)  /* do the call */
    {
        if (lua_isstring(L, -1))
        {
            //if(1 ||verbose)
            {
                print_msg(MSG_INFO, "LUA: Error while calling '%s': '%s'\n", func, lua_tostring(L, -1));
            }
        }
        lua_pop(L, -1);
        return -4;
    }
    

    /* retrieve results */
    nres = -nres;  /* stack index of first result */
    while (lua_gettop(L) && *sig)
    {  
        /* get results */
        switch (*sig++)
        {
            case 'd':  /* double result */
                if (!lua_isnumber(L, nres))
                {
                    if(verbose)
                    {
                        print_msg(MSG_INFO, "LUA: Error while calling '%s': Expected double, got %d\n", func, lua_type(L, nres));
                    }
                    lua_pop(L, -1);
                    break;
                }
                *va_arg(vl, double *) = lua_tonumber(L, nres);
                lua_pop(L, -1);
                break;

            case 'i':  /* int result */
                if (!lua_isnumber(L, nres))
                {
                    if(verbose)
                    {
                        print_msg(MSG_INFO, "LUA: Error while calling '%s': Expected number, got %d\n", func, lua_type(L, nres));
                    }
                    lua_pop(L, -1);
                    break;
                }
                *va_arg(vl, int *) = (int)lua_tonumber(L, nres);
                lua_pop(L, -1);
                break;

            case 's':  /* string result */
                if (!lua_isstring(L, nres))
                {
                    if(verbose)
                    {
                        print_msg(MSG_INFO, "LUA: Error while calling '%s': Expected string, got %d\n", func, lua_type(L, nres));
                    }
                    lua_pop(L, -1);
                    break;
                }
                *va_arg(vl, const char **) = lua_tostring(L, nres);
                lua_pop(L, -1);
                break;

            case 'l':  /* lstring result */
            {
                if (!lua_isstring(L, nres))
                {
                    if(verbose)
                    {
                        print_msg(MSG_INFO, "LUA: Error while calling '%s': Expected string, got %d\n", func, lua_type(L, nres));
                    }
                    lua_pop(L, -1);
                    break;
                }
                const char **ret_data = va_arg(vl, const char **);
                size_t *ret_length = va_arg(vl, size_t *);
                
                *ret_data = lua_tolstring(L, nres, ret_length);
                lua_pop(L, -1);
                break;
            }
            
            default:
                print_msg(MSG_INFO, "LUA: Error while calling '%s': Expected unknown, got %d\n", func, lua_type(L, nres));
                lua_pop(L, -1);
                break;
        }
        nres++;
    }
#else

    /* consume all vararg arguments */
    while (*sig)
    {
        switch (*sig++)
        {
            case 'd':
                va_arg(vl, double);
                break;
            case 'i':
                va_arg(vl, int);
                break;
            case 's':
                va_arg(vl, char *);
                break;
            case 'l':
                va_arg(vl, char *);
                va_arg(vl, int);
                break;
            case '>':
                break;
        }
    }
#endif

    va_end(vl);
    
    return 0;
}


int32_t lua_handle_hdr_suffix(lua_State *lua_state, uint8_t *type, char *suffix, void *hdr, int hdr_len, void *data, int data_len)
{
#if defined(USE_LUA)
    uint8_t *ret_hdr = NULL;
    uint8_t *ret_data = NULL;
    int ret_hdr_len = 0;
    int ret_data_len = 0;
    char func[128];
    
    snprintf(func, sizeof(func), "handle_%.4s%s", type, suffix);
    
    if(data)
    {
        lua_call_va(lua_state, func, "ll>ll", hdr, hdr_len, data, data_len, &ret_hdr, &ret_hdr_len, &ret_data, &ret_data_len);
    }
    else
    {
        lua_call_va(lua_state, func, "l>l", hdr, hdr_len, &ret_hdr, &ret_hdr_len);
    }
    
    /* callee updated block header */
    if(ret_hdr_len > 0 && ret_hdr_len == hdr_len)
    {
        print_msg(MSG_INFO, "LUA: Function '%s' updated hdr data\n", func);
        memcpy(hdr, ret_hdr, hdr_len);
    }
    else if(ret_hdr_len)
    {
        print_msg(MSG_INFO, "LUA: Error while calling '%s': Returned header size mismatch - %d instead of %d\n", func, ret_hdr_len, hdr_len);
    }
    
    /* callee updated block data */
    if(ret_data_len > 0 && ret_data_len == data_len)
    {
        print_msg(MSG_INFO, "LUA: Function '%s' updated hdr data\n", func);
        memcpy(data, ret_data, data_len);
    }
    else if(ret_data_len)
    {
        print_msg(MSG_INFO, "LUA: Error while calling '%s': Returned data size mismatch - %d instead of %d\n", func, ret_data_len, data_len);
    }
    
    
#endif
    return 0;
}

int32_t lua_handle_hdr(lua_State *lua_state, uint8_t *type, void *hdr, int hdr_len)
{
    return lua_handle_hdr_suffix(lua_state, type, "", hdr, hdr_len, NULL, 0);
}

int32_t lua_handle_hdr_data(lua_State *lua_state, uint8_t *type, char *suffix, void *hdr, int hdr_len, void *data, int data_len)
{
    return lua_handle_hdr_suffix(lua_state, type, suffix, hdr, hdr_len, data, data_len);
}

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
     
#define ABS(a) \
   ({ __typeof__ (a) _a = (a); \
     _a > 0 ? _a : -_a; })

#define SGN(a) \
   ((a) > 0 ? 1 : -1 )

#define COERCE(val,min,max) MIN(MAX((val),(min)),(max))
#define COUNT(x)        ((int)(sizeof(x)/sizeof((x)[0])))

#define MSG_INFO     0
#define MSG_ERROR    1
#define MSG_PROGRESS 2

int batch_mode = 0;

void print_msg(uint32_t type, const char* format, ... )
{
    va_list args;
    va_start( args, format );
    char *fmt_str = malloc(strlen(format) + 32);
    
    switch(type)
    {
        case MSG_INFO:
            if(!batch_mode)
            {
                vfprintf(stdout, format, args);
            }
            else
            {
                strcpy(fmt_str, "[I] ");
                strcat(fmt_str, format);
                vfprintf(stdout, fmt_str, args);
            }
            break;
            
        case MSG_ERROR:
            if(!batch_mode)
            {
                strcpy(fmt_str, "[ERROR] ");
                strcat(fmt_str, format);
                vfprintf(stderr, fmt_str, args);
            }
            else
            {
                strcpy(fmt_str, "[E] ");
                strcat(fmt_str, format);
                vfprintf(stdout, fmt_str, args);
                fflush(stdout);
            }
            break;
            
        case MSG_PROGRESS:
            if(!batch_mode)
            {
            }
            else
            {
                strcpy(fmt_str, "[P] ");
                strcat(fmt_str, format);
                vfprintf(stdout, fmt_str, args);
            }
            break;
    }
    
    free(fmt_str);
    va_end( args );
}

/* this structure is used to build the mlv_xref_t table */
typedef struct 
{
    uint64_t    frameTime;
    uint64_t    frameOffset;
    uint32_t    fileNumber;
} PACKED frame_xref_t;

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

void xref_dump(mlv_xref_hdr_t *xref)
{
    mlv_xref_t *xrefs = (mlv_xref_t*)&(((unsigned char *)xref)[sizeof(mlv_xref_hdr_t)]);
    
    for(int pos = 0; pos < xref->entryCount; pos++)
    {
        print_msg(MSG_INFO, "Entry %d/%d\n", pos, xref->entryCount);
        print_msg(MSG_INFO, "    File   #%d\n", xrefs[pos].fileNumber);
        print_msg(MSG_INFO, "    Offset 0x%08X\n", xrefs[pos].frameOffset);
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
        print_msg(MSG_ERROR, "Failed to open file '%s'\n", filename);
        return 1;
    }

    do
    {
        mlv_hdr_t buf;
        uint64_t position = 0;
        
        position = ftello(in_file);
        
        if(fread(&buf, sizeof(mlv_hdr_t), 1, in_file) != 1)
        {
            print_msg(MSG_ERROR, "Failed to read from file '%s'\n", filename);
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
                print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                ret = 3;
                goto load_frame_finish;
            }
            fseeko(in_file, position + file_hdr.blockSize, SEEK_SET);

            if(file_hdr.videoClass & MLV_VIDEO_CLASS_FLAG_LZMA)
            {
                print_msg(MSG_ERROR, "Compressed formats not supported for frame extraction\n");
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
                print_msg(MSG_ERROR, "File '%s' ends in the middle of a block\n", filename);
                ret = 3;
                goto load_frame_finish;
            }
            
            int frame_size = block_hdr.blockSize - sizeof(mlv_vidf_hdr_t) - block_hdr.frameSpace;

            fseeko(in_file, block_hdr.frameSpace, SEEK_CUR);
            if(fread(frame_buffer, frame_size, 1, in_file) != 1)
            {
                print_msg(MSG_ERROR, "File '%s' ends in the middle of a block\n", filename);
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

    print_msg(MSG_INFO, "File %s opened (XREF)\n", filename);
    
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
                print_msg(MSG_ERROR, "File '%s' ends in the middle of a block\n", filename);
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
            print_msg(MSG_ERROR, "File '%s' has invalid blocks\n", filename);
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
        print_msg(MSG_ERROR, "Failed writing into output file\n");
        return;
    }

    print_msg(MSG_INFO, "File %s opened for writing\n", filename);
    
    
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
    
    memset(&hdr, 0x00, sizeof(mlv_xref_hdr_t));
    memcpy(hdr.blockType, "XREF", 4);
    hdr.blockSize = sizeof(mlv_xref_hdr_t) + entries * sizeof(mlv_xref_t);
    hdr.entryCount = entries;
    
    if(fwrite(&hdr, sizeof(mlv_xref_hdr_t), 1, out_file) != 1)
    {
        print_msg(MSG_ERROR, "Failed writing into output file\n");
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
            print_msg(MSG_ERROR, "Failed writing into output file\n");
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
    
    print_msg(MSG_INFO, "File %s opened\n", filename);
    
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
            print_msg(MSG_INFO, "File %s opened\n", filename);
            (*entries)++;
        }
        else
        {
            break;
        }
    }
    return files;
}


#define EV_RESOLUTION 32768

#define CHROMA_SMOOTH_2X2
#include "../dual_iso/chroma_smooth.c"
#undef CHROMA_SMOOTH_2X2

#define CHROMA_SMOOTH_3X3
#include "../dual_iso/chroma_smooth.c"
#undef CHROMA_SMOOTH_3X3

#define CHROMA_SMOOTH_5X5
#include "../dual_iso/chroma_smooth.c"
#undef CHROMA_SMOOTH_5X5


void chroma_smooth(int method, struct raw_info *info)
{
    int black = info->black_level;
    static int raw2ev[16384];
    static int _ev2raw[24*EV_RESOLUTION];
    int* ev2raw = _ev2raw + 10*EV_RESOLUTION;
    
    int i;
    for (i = 0; i < 16384; i++)
    {
        raw2ev[i] = log2(MAX(1, i - black)) * EV_RESOLUTION;
    }

    for (i = -10*EV_RESOLUTION; i < 14*EV_RESOLUTION; i++)
    {
        ev2raw[i] = black + pow(2, (float)i / EV_RESOLUTION);
    }

    int w = info->width;
    int h = info->height;

    unsigned short * aux = malloc(w * h * sizeof(short));
    unsigned short * aux2 = malloc(w * h * sizeof(short));

    int x,y;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            aux[x + y*w] = aux2[x + y*w] = raw_get_pixel(x, y);
        }
    }
    
    switch(method)
    {
        case 0:
            break;
        case 2:
            chroma_smooth_2x2(aux, aux2, raw2ev, ev2raw);
            break;
        case 3:
            chroma_smooth_3x3(aux, aux2, raw2ev, ev2raw);
            break;
        case 5:
            chroma_smooth_5x5(aux, aux2, raw2ev, ev2raw);
            break;
    }
    
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            raw_set_pixel(x, y, aux2[x + y*w]);
        }
    }

    free(aux);
    free(aux2);
}

void show_usage(char *executable)
{
    print_msg(MSG_INFO, "Usage: %s [-o output_file] [-rscd] [-l compression_level(0-9)] <inputfile>\n", executable);
    print_msg(MSG_INFO, "Parameters:\n");
    print_msg(MSG_INFO, " -o output_file      set the filename to write into\n");
    print_msg(MSG_INFO, " -v                  verbose output\n");
    print_msg(MSG_INFO, " --batch             output message suitable for batch processing\n");
    
    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- DNG output --\n");
    print_msg(MSG_INFO, " --dng               output frames into separate .dng files. set prefix with -o\n");
    print_msg(MSG_INFO, " --no-cs             no chroma smoothing\n");
    print_msg(MSG_INFO, " --cs2x2             2x2 chroma smoothing\n");
    print_msg(MSG_INFO, " --cs3x3             3x3 chroma smoothing\n");
    print_msg(MSG_INFO, " --cs5x5             5x5 chroma smoothing\n");
    
    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- RAW output --\n");
    print_msg(MSG_INFO, " -r                  output into a legacy raw file for e.g. raw2dng\n");
    
    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- MLV output --\n");
    print_msg(MSG_INFO, " -b bits             convert image data to given bit depth per channel (1-16)\n");
    print_msg(MSG_INFO, " -z bits             zero the lowest bits, so we have only specified number of bits containing data (1-16) (improves compression rate)\n");
    print_msg(MSG_INFO, " -f frames           frames to save. e.g. '12' saves the first 12 frames, '12-40' saves frames 12 to 40.\n");
    print_msg(MSG_INFO, " -x                  build xref file (indexing)\n");
    print_msg(MSG_INFO, " -m                  write only metadata, no audio or video frames\n");
    print_msg(MSG_INFO, " -n                  write no metadata, only audio and video frames\n");
    
    print_msg(MSG_INFO, " -a                  average all frames in <inputfile> and output a single-frame MLV from it\n");
    print_msg(MSG_INFO, " -s mlv_file         subtract the reference frame in given file from every single frame during processing\n");
    
    print_msg(MSG_INFO, " -e                  delta-encode frames to improve compression, but lose random access capabilities\n");
    
    /* yet unclear which format to choose, so keep that as reminder */
    //print_msg(MSG_INFO, " -u lut_file         look-up table with 4 * xRes * yRes 16-bit words that is applied before bit depth conversion\n");
#ifdef MLV_USE_LZMA
    print_msg(MSG_INFO, " -c                  (re-)compress video and audio frames using LZMA (set bpp to 16 to improve compression rate)\n");
    print_msg(MSG_INFO, " -d                  decompress compressed video and audio frames using LZMA\n");
    print_msg(MSG_INFO, " -l level            set compression level from 0=fastest to 9=best compression\n");
#else
    print_msg(MSG_INFO, " -c, -d, -l          NOT AVAILABLE: compression support was not compiled into this release\n");
#endif
    print_msg(MSG_INFO, "\n");
}

int main (int argc, char *argv[])
{
    char *input_filename = NULL;
    char *output_filename = NULL;
    char *subtract_filename = NULL;
    char *lut_filename = NULL;
    int blocks_processed = 0;
    
    int frame_start = 0;
    int frame_end = 0;
    int audf_frames_processed = 0;
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

    lua_State *lua_state = NULL;

    /* long options */
    int chroma_smooth_method = 0;
    int dng_output = 0;
    int dump_xrefs = 0;

    struct option long_options[] = {
        {"lua",    required_argument, NULL,  'L' },
        {"batch",  no_argument, &batch_mode,  1 },
        {"dump-xrefs",   no_argument, &dump_xrefs,  1 },
        {"dng",    no_argument, &dng_output,  1 },
        {"no-cs",  no_argument, &chroma_smooth_method,  0 },
        {"cs2x2",  no_argument, &chroma_smooth_method,  2 },
        {"cs3x3",  no_argument, &chroma_smooth_method,  3 },
        {"cs5x5",  no_argument, &chroma_smooth_method,  5 },
        {0,         0,                 0,  0 }
    };    
    
    /* disable stdout buffering */
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    
    if(sizeof(mlv_file_hdr_t) != 52)
    {
        print_msg(MSG_INFO, "Error: Your compiler setup is weird. sizeof(mlv_file_hdr_t) is "FMT_SIZE" on your machine. Expected: 52\n", sizeof(mlv_file_hdr_t));
        return ERR_STRUCT_ALIGN;
    }
    
    int index = 0;
    while ((opt = getopt_long(argc, argv, "L:txz:emnas:uvrcdo:l:b:f:", long_options, &index)) != -1) 
    {
        switch (opt)
        {
            case 'L':
#ifdef USE_LUA
                if(!optarg)
                {
                    print_msg(MSG_ERROR, "Error: Missing LUA script filename\n");
                    return ERR_PARAM;
                }
                lua_state = luaL_newstate();
                if(!lua_state)
                {
                    print_msg(MSG_ERROR, "LUA: Failed to init LUA library\n");
                    return ERR_PARAM;
                }
                
                luaL_openlibs(lua_state);

                if(luaL_loadfile(lua_state, optarg) != 0 || lua_pcall(lua_state, 0, 0, 0) != 0)
                {
                    print_msg(MSG_ERROR, "LUA: Failed to load script\n");
                }
                
                if(lua_call_va(lua_state, "init", "", 0) != 0)
                {
                    print_msg(MSG_ERROR, "LUA: Failed to call 'init' in script\n");
                }
                break;
#else
                print_msg(MSG_ERROR, "LUA support not compiled into this binary\n");
                return ERR_PARAM;
#endif
                
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
                    print_msg(MSG_ERROR, "Error: Missing subtract frame filename\n");
                    return ERR_PARAM;
                }
                subtract_filename = strdup(optarg);
                subtract_mode = 1;
                decompress_output = 1;
                break;
                
            case 'u':
                if(!optarg)
                {
                    print_msg(MSG_ERROR, "Error: Missing LUT filename\n");
                    return ERR_PARAM;
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
                print_msg(MSG_ERROR, "Error: Compression support was not compiled into this release\n");
                return ERR_PARAM;
#endif                
                break;
                
            case 'd':
#ifdef MLV_USE_LZMA
                decompress_output = 1;
#else
                print_msg(MSG_ERROR, "Error: Compression support was not compiled into this release\n");
                return ERR_PARAM;
#endif                
                break;
                
            case 'o':
                if(!optarg)
                {
                    print_msg(MSG_ERROR, "Error: Missing output filename\n");
                    return ERR_PARAM;
                }
                output_filename = strdup(optarg);
                break;
                
            case 'l':
                lzma_level = MIN(9, MAX(0, atoi(optarg)));
                break;
                
            case 'f':
                {
                    char *dash = strchr(optarg, '-');
                    
                    /* try to parse "1-10" */
                    if(dash)
                    {
                        *dash = '\000';
                        frame_start = atoi(optarg);
                        frame_end = atoi(&dash[1]);
                        
                        /* to makse sure it is a valid range */
                        if(frame_start > frame_end)
                        {
                            frame_end = frame_start;
                        }
                    }
                    else
                    {
                        /* only the frame end is specified */
                        frame_end = MAX(0, atoi(optarg));
                    }
                }
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
            
            case 0:
                break;
                
            default:
                show_usage(argv[0]);
                return ERR_PARAM;
        }
    }   
    
    if(optind >= argc)
    {
        print_msg(MSG_ERROR, "Error: Missing input filename\n");
        show_usage(argv[0]);
        return ERR_PARAM;
    }
    
    
    
    print_msg(MSG_INFO, "\n"); 
    print_msg(MSG_INFO, " MLV Dumper v1.0\n"); 
    print_msg(MSG_INFO, "-----------------\n"); 
    print_msg(MSG_INFO, "\n"); 
    
    /* get first file */
    input_filename = argv[optind];
    
    print_msg(MSG_INFO, "Mode of operation:\n"); 
    print_msg(MSG_INFO, "   - Input MLV file: '%s'\n", input_filename); 
    
    if(verbose)
    {
        print_msg(MSG_INFO, "   - Verbose messages\n"); 
    }
    
    /* special case - splitting into frames doesnt require a specific output file */
    if(dng_output && !output_filename)
    {
        int len = strlen(input_filename) + 16;
        output_filename = malloc(len);
        
        strcpy(output_filename, input_filename);
        
        char *ext_dot = strrchr(output_filename, '.');
        if(ext_dot)
        {
            *ext_dot = '\000';
        }
        
        strcat(output_filename, "_frame_");
    }
    
    /* display and set/unset variables according to parameters to have a consistent state */
    if(output_filename)
    {
        if(dng_output)
        {
            print_msg(MSG_INFO, "   - Convert to DNG frames\n"); 
            
            delta_encode_mode = 0;
            compress_output = 0;
            mlv_output = 0;
            raw_output = 0;
        }
        else if(raw_output)
        {
            print_msg(MSG_INFO, "   - Convert to legacy RAW\n"); 
            
            delta_encode_mode = 0;
            compress_output = 0;
            mlv_output = 0;
            dng_output = 0;
            
            if(average_mode)
            {
                print_msg(MSG_INFO, "   - disabled average mode, not possible\n");
                average_mode = 0;
            }
        }
        else
        {
            mlv_output = 1;
            dng_output = 0;
            
            print_msg(MSG_INFO, "   - Rewrite MLV\n"); 
            if(bit_zap)
            {
                print_msg(MSG_INFO, "   - Only store %d bits of information per pixel\n", bit_zap); 
            }
            if(bit_depth)
            {
                print_msg(MSG_INFO, "   - Convert to %d bpp\n", bit_depth); 
            }
            if(delta_encode_mode)
            {
                print_msg(MSG_INFO, "   - Only store changes to previous frame\n"); 
            }
            if(compress_output)
            {
                print_msg(MSG_INFO, "   - Compress frame data\n"); 
            }
            if(average_mode)
            {
                print_msg(MSG_INFO, "   - Output only one frame with averaged pixel values\n");
                subtract_mode = 0;
            }
            if(subtract_mode)
            {
                print_msg(MSG_INFO, "   - Subtract reference frame '%s' from single images\n", subtract_filename); 
            }
        }
        
        print_msg(MSG_INFO, "   - Output into '%s'\n", output_filename); 
    }
    else
    {
        /* those dont make sense then */
        raw_output = 0;
        compress_output = 0;
        
        print_msg(MSG_INFO, "   - Verify file structure\n"); 
        if(verbose)
        {
            print_msg(MSG_INFO, "   - Dump all block information\n"); 
        }
    }
    
    if(xref_mode)
    {
        print_msg(MSG_INFO, "   - Output .idx file for faster processing\n"); 
    }

    /* start processing */
    lv_rec_file_footer_t lv_rec_footer;
    mlv_file_hdr_t main_header;
    mlv_lens_hdr_t lens_info;
    mlv_expo_hdr_t expo_info;
    mlv_idnt_hdr_t idnt_info;
    mlv_wbal_hdr_t wbal_info;
    mlv_wavi_hdr_t wavi_info;
    mlv_rtci_hdr_t rtci_info;
    
    /* initialize stuff */
    memset(&lv_rec_footer, 0x00, sizeof(lv_rec_file_footer_t));
    memset(&lens_info, 0x00, sizeof(mlv_lens_hdr_t));
    memset(&expo_info, 0x00, sizeof(mlv_expo_hdr_t));
    memset(&idnt_info, 0x00, sizeof(mlv_idnt_hdr_t));
    memset(&wbal_info, 0x00, sizeof(mlv_wbal_hdr_t));
    memset(&wavi_info, 0x00, sizeof(mlv_wavi_hdr_t));
    memset(&rtci_info, 0x00, sizeof(mlv_rtci_hdr_t));
    
    char info_string[256] = "(MLV Video without INFO blocks)";
    
    /* this table contains the XREF chunk read from idx file, if existing */
    mlv_xref_hdr_t *block_xref = NULL;
    mlv_xref_t *xrefs = NULL;
    int block_xref_pos = 0;

    uint32_t frame_buffer_size = 32*1024*1024;
    
    uint32_t *frame_arith_buffer = NULL;
    uint8_t *frame_buffer = NULL;
    uint8_t *prev_frame_buffer = NULL;
    
    FILE *out_file = NULL;
    FILE *out_file_wav = NULL;
    FILE **in_files = NULL;
    FILE *in_file = NULL;
    
    int in_file_count = 0;
    int in_file_num = 0;

    uint32_t wav_file_size = 0; /* WAV format supports only 32-bit size */
    uint32_t wav_header_size = 0;

    /* this is for our generated XREF table */
    frame_xref_t *frame_xref_table = NULL;
    int frame_xref_allocated = 0;
    int frame_xref_entries = 0;

    /* open files */
    in_files = load_all_chunks(input_filename, &in_file_count);
    if(!in_files || !in_file_count)
    {
        print_msg(MSG_ERROR, "Failed to open file '%s'\n", input_filename);
        return ERR_FILE;
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
            print_msg(MSG_INFO, "XREF table contains %d entries\n", block_xref->entryCount);
            xrefs = (mlv_xref_t *)((uint32_t)block_xref + sizeof(mlv_xref_hdr_t));
            
            if(dump_xrefs)
            {
                xref_dump(block_xref);
            }
        }
        else
        {
            if(delta_encode_mode)
            {
                print_msg(MSG_ERROR, "Delta encoding is not possible without an index file. Please create one using -x option.\n");
                return ERR_INDEX_REQ;
            }
        }
    }
    
    if(average_mode || subtract_mode)
    {
        frame_arith_buffer = malloc(frame_buffer_size);
        if(!frame_arith_buffer)
        {
            print_msg(MSG_ERROR, "Failed to alloc mem\n");
            return ERR_MALLOC;
        }
        memset(frame_arith_buffer, 0x00, frame_buffer_size);
    }
    
    if(subtract_mode)
    {
        int ret = load_frame(subtract_filename, 0, (uint8_t*)frame_arith_buffer);
        
        if(ret)
        {
            print_msg(MSG_ERROR, "Failed to load subtract frame (%d)\n", ret);
            return ERR_FILE;
        }
    }
    
    //if(delta_encode_mode)
    {
        prev_frame_buffer = malloc(frame_buffer_size);
        if(!prev_frame_buffer)
        {
            print_msg(MSG_ERROR, "Failed to alloc mem\n");
            return ERR_MALLOC;
        }
        memset(prev_frame_buffer, 0x00, frame_buffer_size);
    }
    
    if(output_filename || lua_state)
    {
        frame_buffer = malloc(frame_buffer_size);
        if(!frame_buffer)
        {
            print_msg(MSG_ERROR, "Failed to alloc mem\n");
            return ERR_MALLOC;
        }
        
        if(!dng_output && output_filename)
        {
            out_file = fopen(output_filename, "wb+");
            if(!out_file)
            {
                print_msg(MSG_ERROR, "Failed to open file '%s'\n", output_filename);
                return ERR_FILE;
            }
        }
    }

    print_msg(MSG_INFO, "Processing...\n"); 
    do
    {
        mlv_hdr_t buf;
        uint64_t position = 0;
        
read_headers:

        print_msg(MSG_PROGRESS, "B:%d/%d V:%d/%d A:%d/%d\n", blocks_processed, block_xref?block_xref->entryCount:0, vidf_frames_processed, main_header.videoFrameCount, audf_frames_processed, main_header.audioFrameCount);
        
        if(block_xref)
        {
            /* get the file and position of the next block */
            in_file_num = xrefs[block_xref_pos].fileNumber;
            position = xrefs[block_xref_pos].frameOffset;
            
            /* select file and seek to the right position */
            in_file = in_files[in_file_num];
            fseeko(in_file, position, SEEK_SET);
        }
        
        position = ftello(in_file);
        
        if(fread(&buf, sizeof(mlv_hdr_t), 1, in_file) != 1)
        {
            if(block_xref)
            {
                print_msg(MSG_INFO, "Reached EOF of chunk %d/%d after %i blocks total. This should never happen or your index file is wrong.\n", in_file_num, in_file_count, blocks_processed);
                break;
            }
            print_msg(MSG_INFO, "Reached end of chunk %d/%d after %i blocks\n", in_file_num + 1, in_file_count, blocks_processed);
            
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
                print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                goto abort;
            }
            fseeko(in_file, position + file_hdr.blockSize, SEEK_SET);
            
            lua_handle_hdr(lua_state, buf.blockType, &file_hdr, sizeof(file_hdr));

            if(verbose)
            {
                print_msg(MSG_INFO, "File Header (MLVI)\n");
                print_msg(MSG_INFO, "    Size        : 0x%08X\n", file_hdr.blockSize);
                print_msg(MSG_INFO, "    Ver         : %s\n", file_hdr.versionString);
                print_msg(MSG_INFO, "    GUID        : %08" PRIu64 "\n", file_hdr.fileGuid);
                print_msg(MSG_INFO, "    FPS         : %f\n", (double)file_hdr.sourceFpsNom / (double)file_hdr.sourceFpsDenom);
                print_msg(MSG_INFO, "    File        : %d / %d\n", file_hdr.fileNum, file_hdr.fileCount);
                print_msg(MSG_INFO, "    Frames Video: %d\n", file_hdr.videoFrameCount);
                print_msg(MSG_INFO, "    Frames Audio: %d\n", file_hdr.audioFrameCount);
            }

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
            
            /* is this the first file? */
            if(file_hdr.fileNum == 0)
            {
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
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else
            {
                /* no, its another chunk */
                if(main_header.fileGuid != file_hdr.fileGuid)
                {
                    print_msg(MSG_INFO, "Error: GUID within the file chunks mismatch!\n");
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
            if(xref_mode && memcmp(buf.blockType, "NULL", 4))
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
                print_msg(MSG_ERROR, "Missing file header\n");
                goto abort;
            }
            
            if(verbose)
            {
                print_msg(MSG_INFO, "Block: %c%c%c%c\n", buf.blockType[0], buf.blockType[1], buf.blockType[2], buf.blockType[3]);
                print_msg(MSG_INFO, "  Offset: 0x%08" PRIx64 "\n", position);
                print_msg(MSG_INFO, "    Size: %d\n", buf.blockSize);

                /* NULL blocks don't have timestamps */
                if(memcmp(buf.blockType, "NULL", 4))
                {
                    print_msg(MSG_INFO, "    Time: %f ms\n", (double)buf.timestamp / 1000.0f);
                }
            }

            if(!memcmp(buf.blockType, "AUDF", 4))
            {
                mlv_audf_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_audf_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                lua_handle_hdr(lua_state, buf.blockType, &block_hdr, sizeof(block_hdr));
                if(verbose)
                {
                    print_msg(MSG_INFO, "   Frame: #%d\n", block_hdr.frameNumber);
                    print_msg(MSG_INFO, "   Space: %d\n", block_hdr.frameSpace);
                }
                
                /* skip frame space */
                fseeko(in_file, block_hdr.frameSpace, SEEK_CUR);
                
                int frame_size = block_hdr.blockSize - sizeof(mlv_audf_hdr_t) - block_hdr.frameSpace;
                void *buf = malloc(frame_size);
                
                if(fread(buf, frame_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }

                if(!wavi_info.timestamp)
                {
                    print_msg(MSG_ERROR, "Received AUDF without WAVI, the .wav file might be corrupt\n");
                }
                
                /* only write WAV if the WAVI header created a file */
                if(out_file_wav)
                {
                    /* assume block size is uniform, this allows random access */
                    fseeko(out_file_wav, wav_header_size + frame_size * block_hdr.frameNumber, SEEK_SET);
                    fwrite(buf, frame_size, 1, out_file_wav);
                    wav_file_size += frame_size;
                }
                free(buf);
                
                audf_frames_processed++;
            }
            else if(!memcmp(buf.blockType, "VIDF", 4))
            {
                mlv_vidf_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_vidf_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                lua_handle_hdr(lua_state, buf.blockType, &block_hdr, sizeof(block_hdr));
                
                if(verbose)
                {
                    print_msg(MSG_INFO, "   Frame: #%d\n", block_hdr.frameNumber);
                    print_msg(MSG_INFO, "    Crop: %dx%d\n", block_hdr.cropPosX, block_hdr.cropPosY);
                    print_msg(MSG_INFO, "     Pan: %dx%d\n", block_hdr.panPosX, block_hdr.panPosY);
                    print_msg(MSG_INFO, "   Space: %d\n", block_hdr.frameSpace);
                }
                
                if(raw_output || mlv_output || dng_output || lua_state)
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
                        print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                        goto abort;
                    }
                    
                    lua_handle_hdr_data(lua_state, buf.blockType, "_data_read", &block_hdr, sizeof(block_hdr), frame_buffer, frame_size);
                    
                    if(recompress || decompress || ((raw_output || dng_output) && compressed))
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
                                print_msg(MSG_INFO, "    LZMA: "FMT_SIZE" -> "FMT_SIZE"  (%2.2f%%)\n", lzma_in_size, lzma_out_size, ((float)lzma_out_size * 100.0f) / (float)lzma_in_size);
                            }
                        }
                        else
                        {
                            print_msg(MSG_INFO, "    LZMA: Failed (%d)\n", ret);
                            goto abort;
                        }
#else
                        print_msg(MSG_INFO, "    LZMA: not compiled into this release, aborting.\n");
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
                            print_msg(MSG_INFO, "   depth: %d -> %d, size: %d -> %d (%2.2f%%)\n", old_depth, new_depth, frame_size, new_size, ((float)new_depth * 100.0f) / (float)old_depth);
                        }
                        
                        int calced_size = ((video_xRes * video_yRes * old_depth + 7) / 8);
                        if(calced_size > frame_size)
                        {
                            print_msg(MSG_INFO, "Error: old frame size is too small for %dx%d at %d bpp. Input data corrupt. (%d < %d)\n", video_xRes, video_yRes, old_depth, frame_size, calced_size);
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
                    
                    /* when no end was specified, save all frames */
                    uint32_t frame_selected = (!frame_end) || ((block_hdr.frameNumber >= frame_start) && (block_hdr.frameNumber <= frame_end));
                    
                    if(frame_selected)
                    {
                        lua_handle_hdr_data(lua_state, buf.blockType, "_data_write", &block_hdr, sizeof(block_hdr), frame_buffer, frame_size);
                        
                        if(raw_output)
                        {
                            if(!lv_rec_footer.frameSize)
                            {
                                lv_rec_footer.frameSize = frame_size;
                            }

                            lua_handle_hdr_data(lua_state, buf.blockType, "_data_write_raw", &block_hdr, sizeof(block_hdr), frame_buffer, frame_size);
                        
                            fseeko(out_file, (uint64_t)block_hdr.frameNumber * (uint64_t)frame_size, SEEK_SET);
                            fwrite(frame_buffer, frame_size, 1, out_file);
                        }
                        
                        if(dng_output)
                        {
                            void fix_vertical_stripes();
                            extern struct raw_info raw_info;

                            int frame_filename_len = strlen(output_filename) + 32;
                            char *frame_filename = malloc(frame_filename_len);
                            snprintf(frame_filename, frame_filename_len, "%s%06d.dng", output_filename, block_hdr.frameNumber);
                            
                            lua_handle_hdr_data(lua_state, buf.blockType, "_data_write_dng", &block_hdr, sizeof(block_hdr), frame_buffer, frame_size);

                            raw_info = lv_rec_footer.raw_info;
                            raw_info.frame_size = frame_size;
                            raw_info.buffer = frame_buffer;
                            
                            /* override the resolution from raw_info with the one from lv_rec_footer, if they don't match */
                            if (lv_rec_footer.xRes != raw_info.width)
                            {
                                raw_info.width = lv_rec_footer.xRes;
                                raw_info.pitch = raw_info.width * 14/8;
                                raw_info.active_area.x1 = 0;
                                raw_info.active_area.x2 = raw_info.width;
                                raw_info.jpeg.x = 0;
                                raw_info.jpeg.width = raw_info.width;
                            }

                            if (lv_rec_footer.yRes != raw_info.height)
                            {
                                raw_info.height = lv_rec_footer.yRes;
                                raw_info.active_area.y1 = 0;
                                raw_info.active_area.y2 = raw_info.height;
                                raw_info.jpeg.y = 0;
                                raw_info.jpeg.height = raw_info.height;
                            }
        
                            /* call raw2dng code */
                            fix_vertical_stripes();
                            
                            /* this is internal again */
                            chroma_smooth(chroma_smooth_method, &raw_info);
                            
                            /* set MLV metadata into DNG tags */
                            dng_set_framerate_rational(main_header.sourceFpsNom, main_header.sourceFpsDenom);
                            dng_set_shutter(1, (int)(1000000.0f/(float)expo_info.shutterValue));
                            dng_set_aperture(lens_info.aperture, 100);
                            dng_set_camname((char*)idnt_info.cameraName);
                            dng_set_description((char*)info_string);
                            dng_set_lensmodel((char*)lens_info.lensName);
                            dng_set_focal(lens_info.focalLength, 1);
                            dng_set_iso(expo_info.isoValue);
                            
                            dng_set_wbgain(1024, wbal_info.wbgain_r, 1024, wbal_info.wbgain_g, 1024, wbal_info.wbgain_b);
                            
                            /* calculate the time this frame was taken at, i.e., the start time + the current timestamp. this can be off by a second but it's better than nothing */
                            int ms = 0.5 + buf.timestamp / 1000.0;
                            int sec = ms / 1000;
                            ms %= 1000;
                            // FIXME: the struct tm doesn't have tm_gmtoff on Linux so the result might be wrong?
                            struct tm tm;
                            tm.tm_sec = rtci_info.tm_sec + sec;
                            tm.tm_min = rtci_info.tm_min;
                            tm.tm_hour = rtci_info.tm_hour;
                            tm.tm_mday = rtci_info.tm_mday;
                            tm.tm_mon = rtci_info.tm_mon;
                            tm.tm_year = rtci_info.tm_year;
                            tm.tm_wday = rtci_info.tm_wday;
                            tm.tm_yday = rtci_info.tm_yday;
                            tm.tm_isdst = rtci_info.tm_isdst;

                            if(mktime(&tm) != -1)
                            {
                                char datetime_str[20], subsec_str[4];
                                strftime(datetime_str, 20, "%Y:%m:%d %H:%M:%S", &tm);
                                snprintf(subsec_str, sizeof(subsec_str), "%03d", ms);
                                dng_set_datetime(datetime_str, subsec_str);
                            }
                            else
                            {
                                // soemthing went wrong. let's proceed anyway
                                print_msg(MSG_ERROR, "[W] Failed calculating the DateTime from the timestamp\n");
                                dng_set_datetime("", "");
                            }
                            
                            
                            uint64_t serial = 0;
                            char *end;
                            serial = strtoull((char *)idnt_info.cameraSerial, &end, 16);
                            if (serial && !*end)
                            {
                                char serial_str[64];
                                
                                sprintf(serial_str, "%"PRIu64, serial);
                                dng_set_camserial((char*)serial_str);
                            }
                            
                            /* finally save the DNG */
                            save_dng(frame_filename, &raw_info);
                            
                            /* callout for a saved dng file */
                            lua_call_va(lua_state, "dng_saved", "si", frame_filename, block_hdr.frameNumber);
                            
                            free(frame_filename);
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
                                        print_msg(MSG_INFO, "    LZMA: "FMT_SIZE" -> "FMT_SIZE"  (%2.2f%%)\n", lzma_in_size, frame_size, ((float)lzma_out_size * 100.0f) / (float)lzma_in_size);
                                    }
                                }
                                else
                                {
                                    print_msg(MSG_INFO, "    LZMA: Failed (%d)\n", ret);
                                    goto abort;
                                }
                                free(lzma_out);
#else
                                print_msg(MSG_INFO, "    LZMA: not compiled into this release, aborting.\n");
                                goto abort;
#endif
                            }
                            
                            if(frame_size != prev_frame_size)
                            {
                                print_msg(MSG_INFO, "  saving: "FMT_SIZE" -> "FMT_SIZE"  (%2.2f%%)\n", prev_frame_size, frame_size, ((float)frame_size * 100.0f) / (float)prev_frame_size);
                            }
                            
                            lua_handle_hdr_data(lua_state, buf.blockType, "_data_write_mlv", &block_hdr, sizeof(block_hdr), frame_buffer, frame_size);
                            
                            /* delete free space and correct header size if needed */
                            block_hdr.blockSize = sizeof(mlv_vidf_hdr_t) + frame_size;
                            block_hdr.frameSpace = 0;
                            block_hdr.frameNumber -= frame_start;
                            
                            if(fwrite(&block_hdr, sizeof(mlv_vidf_hdr_t), 1, out_file) != 1)
                            {
                                print_msg(MSG_ERROR, "Failed writing into output file\n");
                                goto abort;
                            }
                            if(fwrite(frame_buffer, frame_size, 1, out_file) != 1)
                            {
                                print_msg(MSG_ERROR, "Failed writing into output file\n");
                                goto abort;
                            }
                        }
                    }
                }
                else
                {
                    fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);
                }

                vidf_max_number = MAX(vidf_max_number, block_hdr.frameNumber);
                
                vidf_frames_processed++;
            }
            else if(!memcmp(buf.blockType, "LENS", 4))
            {
                uint32_t hdr_size = MIN(sizeof(mlv_lens_hdr_t), buf.blockSize);

                if(fread(&lens_info, hdr_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + lens_info.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &lens_info, sizeof(lens_info));

                if(verbose)
                {
                    print_msg(MSG_INFO, "     Name:        '%s'\n", lens_info.lensName);
                    print_msg(MSG_INFO, "     Serial:      '%s'\n", lens_info.lensSerial);
                    print_msg(MSG_INFO, "     Focal Len:   %d mm\n", lens_info.focalLength);
                    print_msg(MSG_INFO, "     Focus Dist:  %d mm\n", lens_info.focalDist);
                    print_msg(MSG_INFO, "     Aperture:    f/%.2f\n", (double)lens_info.aperture / 100.0f);
                    print_msg(MSG_INFO, "     IS Mode:     %d\n", lens_info.stabilizerMode);
                    print_msg(MSG_INFO, "     AF Mode:     %d\n", lens_info.autofocusMode);
                    print_msg(MSG_INFO, "     Lens ID:     0x%08X\n", lens_info.lensID);
                    print_msg(MSG_INFO, "     Flags:       0x%08X\n", lens_info.flags);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    lens_info.blockSize = sizeof(mlv_lens_hdr_t);
                    if(fwrite(&lens_info, lens_info.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
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
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                lua_handle_hdr(lua_state, buf.blockType, &block_hdr, sizeof(block_hdr));

                /* get the string length and malloc a buffer for that string */
                int str_length = block_hdr.blockSize - hdr_size;

                if(str_length)
                {
                    char *buf = malloc(str_length + 1);

                    if(fread(buf, str_length, 1, in_file) != 1)
                    {
                        print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                        goto abort;
                    }
                    
                    strncpy(info_string, buf, sizeof(info_string));

                    if(verbose)
                    {
                        buf[str_length] = '\000';
                        print_msg(MSG_INFO, "     String:   '%s'\n", buf);
                    }
                    
                    /* only output this block if there is any data */
                    if(mlv_output && !no_metadata_mode)
                    {
                        /* correct header size if needed */
                        block_hdr.blockSize = sizeof(mlv_info_hdr_t) + str_length;
                        if(fwrite(&block_hdr, sizeof(mlv_info_hdr_t), 1, out_file) != 1)
                        {
                            print_msg(MSG_ERROR, "Failed writing into output file\n");
                            goto abort;
                        }
                        if(fwrite(buf, str_length, 1, out_file) != 1)
                        {
                            print_msg(MSG_ERROR, "Failed writing into output file\n");
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
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &block_hdr, sizeof(block_hdr));

                if(verbose)
                {
                    print_msg(MSG_INFO, "     Roll:    %2.2f\n", (double)block_hdr.roll / 100.0f);
                    print_msg(MSG_INFO, "     Pitch:   %2.2f\n", (double)block_hdr.pitch / 100.0f);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_elvl_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
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
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &block_hdr, sizeof(block_hdr));

                if(verbose)
                {
                    print_msg(MSG_INFO, "     picStyle:   %d\n", block_hdr.picStyleId);
                    print_msg(MSG_INFO, "     contrast:   %d\n", block_hdr.contrast);
                    print_msg(MSG_INFO, "     sharpness:  %d\n", block_hdr.sharpness);
                    print_msg(MSG_INFO, "     saturation: %d\n", block_hdr.saturation);
                    print_msg(MSG_INFO, "     colortone:  %d\n", block_hdr.colortone);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_styl_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "WBAL", 4))
            {
                uint32_t hdr_size = MIN(sizeof(mlv_wbal_hdr_t), buf.blockSize);

                if(fread(&wbal_info, hdr_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + wbal_info.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &wbal_info, sizeof(wbal_info));

                if(verbose)
                {
                    print_msg(MSG_INFO, "     Mode:   %d\n", wbal_info.wb_mode);
                    print_msg(MSG_INFO, "     Kelvin:   %d\n", wbal_info.kelvin);
                    print_msg(MSG_INFO, "     Gain R:   %d\n", wbal_info.wbgain_r);
                    print_msg(MSG_INFO, "     Gain G:   %d\n", wbal_info.wbgain_g);
                    print_msg(MSG_INFO, "     Gain B:   %d\n", wbal_info.wbgain_b);
                    print_msg(MSG_INFO, "     Shift GM:   %d\n", wbal_info.wbs_gm);
                    print_msg(MSG_INFO, "     Shift BA:   %d\n", wbal_info.wbs_ba);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    wbal_info.blockSize = sizeof(mlv_wbal_hdr_t);
                    if(fwrite(&wbal_info, wbal_info.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "IDNT", 4))
            {
                uint32_t hdr_size = MIN(sizeof(mlv_idnt_hdr_t), buf.blockSize);

                if(fread(&idnt_info, hdr_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + idnt_info.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &idnt_info, sizeof(idnt_info));

                if(verbose)
                {
                    print_msg(MSG_INFO, "     Camera Name:   '%s'\n", idnt_info.cameraName);
                    print_msg(MSG_INFO, "     Camera Serial: '%s'\n", idnt_info.cameraSerial);
                    print_msg(MSG_INFO, "     Camera Model:  0x%08X\n", idnt_info.cameraModel);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    idnt_info.blockSize = sizeof(mlv_idnt_hdr_t);
                    if(fwrite(&idnt_info, idnt_info.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "RTCI", 4))
            {
                uint32_t hdr_size = MIN(sizeof(mlv_rtci_hdr_t), buf.blockSize);

                if(fread(&rtci_info, hdr_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + rtci_info.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &rtci_info, sizeof(rtci_info));

                if(verbose)
                {
                    print_msg(MSG_INFO, "     Date:        %02d.%02d.%04d\n", rtci_info.tm_mday, rtci_info.tm_mon + 1, 1900 + rtci_info.tm_year);
                    print_msg(MSG_INFO, "     Time:        %02d:%02d:%02d (GMT+%d)\n", rtci_info.tm_hour, rtci_info.tm_min, rtci_info.tm_sec, rtci_info.tm_gmtoff);
                    print_msg(MSG_INFO, "     Zone:        '%s'\n", rtci_info.tm_zone);
                    print_msg(MSG_INFO, "     Day of week: %d\n", rtci_info.tm_wday);
                    print_msg(MSG_INFO, "     Day of year: %d\n", rtci_info.tm_yday);
                    print_msg(MSG_INFO, "     Daylight s.: %d\n", rtci_info.tm_isdst);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    rtci_info.blockSize = sizeof(mlv_rtci_hdr_t);
                    if(fwrite(&rtci_info, rtci_info.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
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
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &block_hdr, sizeof(block_hdr));

                if(verbose)
                {
                    print_msg(MSG_INFO, "  Button: 0x%02X\n", block_hdr.type);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_mark_hdr_t);
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "EXPO", 4))
            {
                uint32_t hdr_size = MIN(sizeof(mlv_expo_hdr_t), buf.blockSize);

                if(fread(&expo_info, hdr_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + expo_info.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &expo_info, sizeof(expo_info));

                if(verbose)
                {
                    print_msg(MSG_INFO, "     ISO Mode:   %d\n", expo_info.isoMode);
                    print_msg(MSG_INFO, "     ISO:        %d\n", expo_info.isoValue);
                    print_msg(MSG_INFO, "     ISO Analog: %d\n", expo_info.isoAnalog);
                    print_msg(MSG_INFO, "     ISO DGain:  %d/1024 EV\n", expo_info.digitalGain);
                    print_msg(MSG_INFO, "     Shutter:    %" PRIu64 " µs (1/%.2f)\n", expo_info.shutterValue, 1000000.0f/(float)expo_info.shutterValue);
                }
            
                if(mlv_output && !no_metadata_mode)
                {
                    /* correct header size if needed */
                    expo_info.blockSize = sizeof(mlv_expo_hdr_t);
                    if(fwrite(&expo_info, expo_info.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
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
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &block_hdr, sizeof(block_hdr));

                video_xRes = block_hdr.xRes;
                video_yRes = block_hdr.yRes;
                if(verbose)
                {
                    print_msg(MSG_INFO, "    Res:  %dx%d\n", block_hdr.xRes, block_hdr.yRes);
                    print_msg(MSG_INFO, "    raw_info:\n");
                    print_msg(MSG_INFO, "      api_version      0x%08X\n", block_hdr.raw_info.api_version);
                    print_msg(MSG_INFO, "      height           %d\n", block_hdr.raw_info.height);
                    print_msg(MSG_INFO, "      width            %d\n", block_hdr.raw_info.width);
                    print_msg(MSG_INFO, "      pitch            %d\n", block_hdr.raw_info.pitch);
                    print_msg(MSG_INFO, "      frame_size       0x%08X\n", block_hdr.raw_info.frame_size);
                    print_msg(MSG_INFO, "      bits_per_pixel   %d\n", block_hdr.raw_info.bits_per_pixel);
                    print_msg(MSG_INFO, "      black_level      %d\n", block_hdr.raw_info.black_level);
                    print_msg(MSG_INFO, "      white_level      %d\n", block_hdr.raw_info.white_level);
                    print_msg(MSG_INFO, "      active_area.y1   %d\n", block_hdr.raw_info.active_area.y1);
                    print_msg(MSG_INFO, "      active_area.x1   %d\n", block_hdr.raw_info.active_area.x1);
                    print_msg(MSG_INFO, "      active_area.y2   %d\n", block_hdr.raw_info.active_area.y2);
                    print_msg(MSG_INFO, "      active_area.x2   %d\n", block_hdr.raw_info.active_area.x2);
                    print_msg(MSG_INFO, "      exposure_bias    %d, %d\n", block_hdr.raw_info.exposure_bias[0], block_hdr.raw_info.exposure_bias[1]);
                    print_msg(MSG_INFO, "      cfa_pattern      0x%08X\n", block_hdr.raw_info.cfa_pattern);
                    print_msg(MSG_INFO, "      calibration_ill  %d\n", block_hdr.raw_info.calibration_illuminant1);
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
                if(mlv_output)
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_rawi_hdr_t);
                    
                    if(bit_depth)
                    {
                        block_hdr.raw_info.bits_per_pixel = bit_depth;
                    }
                    
                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into output file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(buf.blockType, "WAVI", 4))
            {
                mlv_wavi_hdr_t block_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_wavi_hdr_t), buf.blockSize);

                if(fread(&block_hdr, hdr_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }
                
                /* skip remaining data, if there is any */
                fseeko(in_file, position + block_hdr.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, &block_hdr, sizeof(block_hdr));

                if(verbose)
                {
                    print_msg(MSG_INFO, "    wav_info:\n");
                    print_msg(MSG_INFO, "      format           %d\n", block_hdr.format);
                    print_msg(MSG_INFO, "      channels         %d\n", block_hdr.channels);
                    print_msg(MSG_INFO, "      samplingRate     %d\n", block_hdr.samplingRate);
                    print_msg(MSG_INFO, "      bytesPerSecond   %d\n", block_hdr.bytesPerSecond);
                    print_msg(MSG_INFO, "      blockAlign       %d\n", block_hdr.blockAlign);
                    print_msg(MSG_INFO, "      bitsPerSample    %d\n", block_hdr.bitsPerSample);
                }
                
                memcpy(&wavi_info, &block_hdr, sizeof(mlv_wavi_hdr_t));
            
                if(output_filename && out_file_wav == NULL)
                {
                    size_t name_len = strlen(output_filename) + 5;  // + .wav\0
                    char* wav_file_name = malloc(name_len);
                    /* NOTE, assumes little endian system, fix for big endian */
                    uint32_t tmp_uint32;
                    uint16_t tmp_uint16;

                    strncpy(wav_file_name, output_filename, name_len);
                    strncat(wav_file_name, ".wav", name_len);
                    out_file_wav = fopen(wav_file_name, "wb");
                    free(wav_file_name);
                    if(!out_file_wav)
                    {
                        print_msg(MSG_ERROR, "Failed writing into audio output file\n");
                        goto abort;
                    }
    
                    /* Write header */
                    fwrite("RIFF", 4, 1, out_file_wav);
                    tmp_uint32 = 36; // Two headers combined size, will be patched later
                    fwrite(&tmp_uint32, 4, 1, out_file_wav);
                    fwrite("WAVE", 4, 1, out_file_wav);
                    
                    fwrite("fmt ", 4, 1, out_file_wav);
                    tmp_uint32 = 16; // Header size
                    fwrite(&tmp_uint32, 4, 1, out_file_wav);
                    tmp_uint16 = wavi_info.format; // PCM
                    fwrite(&tmp_uint16, 2, 1, out_file_wav);
                    tmp_uint16 = wavi_info.channels; // Stereo
                    fwrite(&tmp_uint16, 2, 1, out_file_wav);
                    tmp_uint32 = wavi_info.samplingRate; // Sample rate
                    fwrite(&tmp_uint32, 4, 1, out_file_wav);
                    tmp_uint32 = wavi_info.bytesPerSecond; // Byte rate (16-bit data, stereo)
                    fwrite(&tmp_uint32, 4, 1, out_file_wav);
                    tmp_uint16 = wavi_info.blockAlign; // Block align
                    fwrite(&tmp_uint16, 2, 1, out_file_wav);
                    tmp_uint16 = wavi_info.bitsPerSample; // Bits per sample
                    fwrite(&tmp_uint16, 2, 1, out_file_wav);

                    fwrite("data", 4, 1, out_file_wav);
                    tmp_uint32 = 0; // Audio data length, will be patched later
                    fwrite(&tmp_uint32, 4, 1, out_file_wav);
                    
                    wav_file_size = 0;
                    wav_header_size = ftello(out_file_wav);
                }
            }
            else if(!memcmp(buf.blockType, "NULL", 4))
            {
                fseeko(in_file, position + buf.blockSize, SEEK_SET);
            }
            else
            {
                print_msg(MSG_INFO, "Unknown Block: %c%c%c%c, skipping\n", buf.blockType[0], buf.blockType[1], buf.blockType[2], buf.blockType[3]);
                fseeko(in_file, position + buf.blockSize, SEEK_SET);
                
                lua_handle_hdr(lua_state, buf.blockType, "", 0);
            }
        }
        
        /* count any read block, no matter if header or video frame */
        blocks_processed++;
        

        if(block_xref)
        {
            block_xref_pos++;
            if(block_xref_pos >= block_xref->entryCount)
            {
                print_msg(MSG_INFO, "Reached end of all files after %i blocks\n", blocks_processed);
                break;
            }
        }
    }
    while(!feof(in_file));
    
abort:

    print_msg(MSG_INFO, "Processed %d video frames\n", vidf_frames_processed);
    
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
        print_msg(MSG_INFO, "XREF table contains %d entries\n", frame_xref_entries);
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

    if(out_file_wav)
    {
        /* Patch the WAV size fields */
        uint32_t tmp_uint32 = wav_file_size + 36; /* + header size */
        fseek(out_file_wav, 4, SEEK_SET);
        fwrite(&tmp_uint32, 4, 1, out_file_wav);

        tmp_uint32 = wav_file_size; /* data size */
        fseek(out_file_wav, 40, SEEK_SET);
        fwrite(&tmp_uint32, 4, 1, out_file_wav);
        fclose(out_file_wav);
    }

    /* passing NULL to free is absolutely legal, so no check required */
    free(lut_filename);
    free(subtract_filename);
    free(output_filename);
    free(prev_frame_buffer);
    free(frame_arith_buffer);
    free(block_xref);

    print_msg(MSG_INFO, "Done\n"); 
    print_msg(MSG_INFO, "\n"); 

    return ERR_OK;
}
