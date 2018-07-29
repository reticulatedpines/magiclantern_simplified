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
#include <strings.h>
#include <getopt.h>
#include <inttypes.h>
#include <time.h>
#include <assert.h>

#define MODULE_STRINGS_PREFIX mlv_dump_strings
#include "../module_strings_wrapper.h"
#include "module_strings.h"
MODULE_STRINGS()
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

/* helper macro to get a pointer into a data block's payload based on integer offset */
#define BYTE_OFFSET(var,offset) ((void *)((uintmax_t)(var) + (uintmax_t)(offset)))

/* helper to mark function parameters as unused */
#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#define MSG_INFO     0
#define MSG_ERROR    1
#define MSG_PROGRESS 2


/* some compile warning, why? */
char *strdup(const char *s);

#ifdef MLV_USE_LZMA
#include <LzmaLib.h>
#endif

#ifdef MLV_USE_LJ92
#include "lj92.h"
#endif

/* project includes */
#include "../lv_rec/lv_rec.h"
#include "../../src/raw.h"
#include "mlv.h"
#include "dng/dng.h"
#include "wav.h"

enum bug_id
{
    BUG_ID_NONE = 0,
    /* 
        this bug results in wrong block sizes in a VIDF, even with unaligned lenghs. 
        when this fix is enabled and an unknown block is encountered, scan the area 
        for a NULL block which should follow right after the VIDF.
        introduced: 9058cbc13fa4 
        fixed in  : 2da80f3de3d1 
        */
    BUG_ID_BLOCKSIZE_WRONG = 1,
    /* 
        dont know yet where this bug comes from. was reported in http://www.magiclantern.fm/forum/index.php?topic=14703
    */
    BUG_ID_FRAMEDATA_MISALIGN = 2,
    /* 
        the code currently assumes that writing from cached memory, especially cache, would work.
        seems that works most time, but not always.
        no matter where it comes from, we need a workaround.
        https://www.magiclantern.fm/forum/index.php?topic=19761
    */
    BUG_ID_NULL_SIZE_ZERO = 3
};

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


/* based on http://www.lua.org/pil/25.3.html */
int32_t lua_call_va(lua_State *L, const char *func, const char *sig, ...)
{
    va_list vl;

    va_start(vl, sig);
#if defined(USE_LUA)
    int narg, nres;  /* number of arguments and results */
    int verbose = 0;
    
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
    (void)L;
    (void)func;
    (void)sig;

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
#else
    (void)lua_state;
    (void)type;
    (void)suffix;
    (void)hdr;
    (void)hdr_len;
    (void)data;
    (void)data_len;
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

/* platform/target specific fseek/ftell functions go here */
uint64_t file_get_pos(FILE *stream)
{
#if defined(__WIN32)
    return ftello64(stream);
#else
    return ftello(stream);
#endif
}

uint32_t file_set_pos(FILE *stream, uint64_t offset, int whence)
{
#if defined(__WIN32)
    return fseeko64(stream, offset, whence);
#else
    return fseeko(stream, offset, whence);
#endif
}


/* this structure is used to build the mlv_xref_t table */
typedef struct
{
    uint64_t    frameTime;
    uint64_t    frameOffset;
    uint16_t    fileNumber;
    uint16_t    frameType;
} frame_xref_t;

void xref_resize(frame_xref_t **table, int entries, int *allocated)
{
    /* make sure there is no crappy pointer before using */
    if(*allocated == 0)
    {
        *table = NULL;
    }

    /* only resize if the buffer is too small */
    if(entries * sizeof(frame_xref_t) > (uint32_t)(*allocated))
    {
        *allocated += (entries + 1) * sizeof(frame_xref_t);
        *table = realloc(*table, *allocated);
    }
}

void xref_dump(mlv_xref_hdr_t *xref)
{
    mlv_xref_t *xrefs = (mlv_xref_t*)&(((unsigned char *)xref)[sizeof(mlv_xref_hdr_t)]);

    for(uint32_t pos = 0; pos < xref->entryCount; pos++)
    {
        print_msg(MSG_INFO, "Entry %d/%d\n", pos + 1, xref->entryCount);
        print_msg(MSG_INFO, "    File   #%d\n", xrefs[pos].fileNumber);
        print_msg(MSG_INFO, "    Offset 0x%08X\n", xrefs[pos].frameOffset);
        switch (xrefs[pos].frameType)
        {
            case MLV_FRAME_VIDF:
                print_msg(MSG_INFO, "    Type   VIDF\n");
                break;
            case MLV_FRAME_AUDF:
                print_msg(MSG_INFO, "    Type   AUDF\n");
                break;
            default:
                break;
        }
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

int load_frame(char *filename, uint8_t **frame_buffer, uint32_t *frame_buffer_size)
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

        position = file_get_pos(in_file);

        if(fread(&buf, sizeof(mlv_hdr_t), 1, in_file) != 1)
        {
            print_msg(MSG_ERROR, "Failed to read from file '%s'\n", filename);
            ret = 2;
            goto load_frame_finish;
        }

        print_msg(MSG_INFO, "Block: %c%c%c%c\n", buf.blockType[0], buf.blockType[1], buf.blockType[2], buf.blockType[3]);
        print_msg(MSG_INFO, "  Offset: 0x%08" PRIx64 "\n", position);
        print_msg(MSG_INFO, "    Size: %d\n", buf.blockSize);
        
        /* jump back to the beginning of the block just read */
        file_set_pos(in_file, position, SEEK_SET);

        position = file_get_pos(in_file);

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
            file_set_pos(in_file, position + file_hdr.blockSize, SEEK_SET);

            if(file_hdr.videoClass & MLV_VIDEO_CLASS_FLAG_LZMA)
            {
                print_msg(MSG_ERROR, "Compressed formats not supported for frame extraction\n");
                ret = 5;
                goto load_frame_finish;
            }
            if(file_hdr.videoClass & MLV_VIDEO_CLASS_FLAG_LJ92)
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
        
            /* loading the first frame. report frame size and allocate memory for that frame */
            *frame_buffer_size = frame_size;
            *frame_buffer = malloc(frame_size);

            file_set_pos(in_file, block_hdr.frameSpace, SEEK_CUR);
            if(fread(*frame_buffer, frame_size, 1, in_file) != 1)
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
            file_set_pos(in_file, position + buf.blockSize, SEEK_SET);
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
    int max_name_len = strlen(base_filename) + 16;
    char *filename = malloc(max_name_len);
    FILE *in_file = NULL;

    strncpy(filename, base_filename, max_name_len);
    strcpy(&filename[strlen(filename) - 3], "IDX");

    in_file = fopen(filename, "rb");

    if(!in_file)
    {
        free(filename);
        return NULL;
    }

    print_msg(MSG_INFO, "File %s opened (XREF)\n", filename);

    do
    {
        mlv_hdr_t buf;
        uint64_t position = 0;

        position = file_get_pos(in_file);

        if(fread(&buf, sizeof(mlv_hdr_t), 1, in_file) != 1)
        {
            break;
        }

        /* jump back to the beginning of the block just read */
        file_set_pos(in_file, position, SEEK_SET);

        position = file_get_pos(in_file);

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
            file_set_pos(in_file, position + buf.blockSize, SEEK_SET);
        }

        /* we are at the same position as before, so abort */
        if(position == file_get_pos(in_file))
        {
            print_msg(MSG_ERROR, "File '%s' has invalid blocks\n", filename);
            break;
        }
    }
    while(!feof(in_file));

    fclose(in_file);

    free(filename);
    return block_hdr;
}

void save_index(char *base_filename, mlv_file_hdr_t *ref_file_hdr, int fileCount, frame_xref_t *index, int entries)
{
    int max_name_len = strlen(base_filename) + 16;
    char *filename = malloc(max_name_len);
    FILE *out_file = NULL;

    strncpy(filename, base_filename, max_name_len);

    strcpy(&filename[strlen(filename) - 3], "IDX");

    out_file = fopen(filename, "wb+");

    if(!out_file)
    {
        free(filename);
        print_msg(MSG_ERROR, "Failed writing into .IDX file\n");
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

    if(fwrite(&file_hdr, sizeof(mlv_file_hdr_t), 1, out_file) != 1)
    {
        free(filename);
        print_msg(MSG_ERROR, "Failed writing into .IDX file\n");
        fclose(out_file);
        return;
    }

    /* now write XREF block */
    mlv_xref_hdr_t hdr;

    memset(&hdr, 0x00, sizeof(mlv_xref_hdr_t));
    memcpy(hdr.blockType, "XREF", 4);
    hdr.blockSize = sizeof(mlv_xref_hdr_t) + entries * sizeof(mlv_xref_t);
    hdr.entryCount = entries;

    if(fwrite(&hdr, sizeof(mlv_xref_hdr_t), 1, out_file) != 1)
    {
        free(filename);
        print_msg(MSG_ERROR, "Failed writing into .IDX file\n");
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
        field.frameType = index[entry].frameType;

        if(fwrite(&field, sizeof(mlv_xref_t), 1, out_file) != 1)
        {
            free(filename);
            print_msg(MSG_ERROR, "Failed writing into .IDX file\n");
            fclose(out_file);
            return;
        }
    }

    free(filename);
    fclose(out_file);
}


FILE **load_all_chunks(char *base_filename, int *entries)
{
    int seq_number = 0;
    int max_name_len = strlen(base_filename) + 16;
    char *filename = malloc(max_name_len);

    strncpy(filename, base_filename, max_name_len - 1);
    FILE **files = malloc(sizeof(FILE*));

    files[0] = fopen(filename, "rb");
    if(!files[0])
    {
        free(filename);
        free(files);
        return NULL;
    }

    print_msg(MSG_INFO, "File %s opened\n", filename);

    /* get extension and check if it is a .MLV */
    char *dot = strrchr(filename, '.');
    if(dot)
    {
        dot++;
        if(strcasecmp(dot, "mlv"))
        {
            seq_number = 100;
        }
    }
    
    (*entries)++;
    while(seq_number < 99)
    {
        FILE **realloc_files = realloc(files, (*entries + 1) * sizeof(FILE*));

        if(!realloc_files)
        {
            free(filename);
            free(files);
            return NULL;
        }

        files = realloc_files;

        /* check for the next file M00, M01 etc */
        char seq_name[8];

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
            print_msg(MSG_INFO, "File %s not existing.\n", filename);
            break;
        }
    }

    free(filename);
    return files;
}

void show_usage(char *executable)
{
    print_msg(MSG_INFO, "Usage: %s [-o output_file] [-rscd] [-l compression_level(0-9)] <inputfile>\n", executable);
    print_msg(MSG_INFO, "Parameters:\n");
    print_msg(MSG_INFO, "  -o output_file      write video data into a MLV file\n");
    print_msg(MSG_INFO, "  -v                  verbose output\n");
    print_msg(MSG_INFO, "  --version           print version information\n");
    print_msg(MSG_INFO, "  --batch             format output message suitable for batch processing\n");
    print_msg(MSG_INFO, "  --relaxed           do not exit on every error, skip blocks that are erroneous\n");
    print_msg(MSG_INFO, "  --no-audio          for DNG output WAV not saved, for MLV output WAVI/AUDF blocks are not included in destination MLV\n");
    
    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- DNG output --\n");
    print_msg(MSG_INFO, "  --dng               output frames into separate .dng files. set prefix with -o\n");
    print_msg(MSG_INFO, "  --no-cs             no chroma smoothing (default)\n");
    print_msg(MSG_INFO, "  --cs2x2             2x2 chroma smoothing\n");
    print_msg(MSG_INFO, "  --cs3x3             3x3 chroma smoothing\n");
    print_msg(MSG_INFO, "  --cs5x5             5x5 chroma smoothing\n");
    print_msg(MSG_INFO, "  --no-fixfp          do not fix focus pixels\n");
    print_msg(MSG_INFO, "  --no-fixcp          do not fix bad pixels\n");
    print_msg(MSG_INFO, "  --fixcp2            use aggressive method for revealing more bad pixels\n");
    print_msg(MSG_INFO, "  --no-stripes        do not fix vertical stripes in highlights\n");
    print_msg(MSG_INFO, "  --force-stripes     compute stripe correction for every frame\n");
    print_msg(MSG_INFO, "  --is-dualiso        use dual iso compatible horizontal interpolation of focus and bad pixels\n");
    print_msg(MSG_INFO, "  --is-croprec        generate focus map for crop_rec mode\n");
    print_msg(MSG_INFO, "  --save-bpm          save bad pixels to .BPM file\n");
    print_msg(MSG_INFO, "  --fixpn             fix pattern noise\n");
    print_msg(MSG_INFO, "  --deflicker=value   per-frame exposure compensation. value is target median in raw units ex: 3072 (default)\n");
    print_msg(MSG_INFO, "  --no-bitpack        write DNG files with unpacked to 16 bit raw data\n");
    print_msg(MSG_INFO, "  --show-progress     show DNG file creation progress. ignored when -v or --batch is specified\n");
    print_msg(MSG_INFO, "                      also works when compressing MLV to MLV and shows compression ratio for each frame\n");
    print_msg(MSG_INFO, "  --fpi <method>      focus pixel interpolation method: 0 (mlvfs), 1 (raw2dng), default is 0\n");
    print_msg(MSG_INFO, "  --bpi <method>      bad pixel interpolation method: 0 (mlvfs), 1 (raw2dng), default is 0\n");

    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- RAW output --\n");
    print_msg(MSG_INFO, "  -r                  output into a legacy raw file for e.g. raw2dng\n");

    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- MLV output --\n");
    print_msg(MSG_INFO, "  -b bits             convert image data to given bit depth per channel (1-16)\n");
    print_msg(MSG_INFO, "  -z bits             zero the lowest bits, so we have only specified number of bits containing data (1-16) (improves compression rate)\n");
    print_msg(MSG_INFO, "  -f frames           frames to save. e.g. '12' saves frames 0 to 12, '12-40' saves frames 12 to 40\n");
    print_msg(MSG_INFO, "  -A fpsx1000         Alter the video file's FPS metadata\n");
    print_msg(MSG_INFO, "  -x                  build xref file (indexing)\n");

    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- MLV autopsy --\n");
    print_msg(MSG_INFO, "  --skip-block <block#>        skip given block number, as if it wasn't present\n");
    print_msg(MSG_INFO, "  --skip-type <type>           skip given block type (e.g. VIDF, AUDF, etc), as if they weren't present\n");
    print_msg(MSG_INFO, "  --extract <block#>           extract the block at given position into autopsy file\n");
    print_msg(MSG_INFO, "  --extract-type <type>        extract the block type (e.g. VERS, LENS, etc) into autopsy file\n");
    print_msg(MSG_INFO, "  --replace <block#>           replace block with data from given autopsy file; requires --autopsy-file\n");
    print_msg(MSG_INFO, "  --payload-only               extract/replace affect not the whole block, but only payload\n");
    print_msg(MSG_INFO, "  --header-only                extract/replace affect not the whole block, but only header\n");
    print_msg(MSG_INFO, "  --autopsy-file <file>        extract/replace from this file\n");
    print_msg(MSG_INFO, "  --hex                        extract prints the selected data as hexdump on screen\n");
    print_msg(MSG_INFO, "  --ascii                      extract prints the selected data as ASCII on screen (only suitable for VERS and DEBG)\n");
    print_msg(MSG_INFO, "  --visualize                  visualize block types, most likely you want to use --skip-xref along with it\n");
    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- MLV manipulation --\n");
    print_msg(MSG_INFO, "  --skip-xref                  skip loading .IDX (XREF) file, read block in the MLV file's order instead of presorted\n");
    print_msg(MSG_INFO, "  -I <mlv_file>                inject data from given MLV file right after MLVI header\n");
    print_msg(MSG_INFO, "  -X type                      extract only block type int output file\n");
    
    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- Image manipulation --\n");
    print_msg(MSG_INFO, "  -a                  average all frames in <inputfile> and output a single-frame MLV from it\n");
    print_msg(MSG_INFO, "  --avg-vertical      [DARKFRAME ONLY] average the resulting frame in vertical direction, so we will extract vertical banding\n");
    print_msg(MSG_INFO, "  --avg-horizontal    [DARKFRAME ONLY] average the resulting frame in horizontal direction, so we will extract horizontal banding\n");
    print_msg(MSG_INFO, "  -s mlv_file         subtract the reference frame in given file from every single frame during processing\n");
    print_msg(MSG_INFO, "  -t mlv_file         use the reference frame in given file as flat field (gain correction)\n");

    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, "-- Processing --\n");
    print_msg(MSG_INFO, "  -e                  delta-encode frames to improve compression, but lose random access capabilities\n");

    /* yet unclear which format to choose, so keep that as reminder */
    //print_msg(MSG_INFO, " -u lut_file         look-up table with 4 * xRes * yRes 16-bit words that is applied before bit depth conversion\n");

#if defined(MLV_USE_LZMA) || defined(MLV_USE_LJ92)
    print_msg(MSG_INFO, "  -c                  compress video frames using LJ92. if input is lossless, then decompress and recompress again.\n");
    print_msg(MSG_INFO, "  -d                  decompress compressed video and audio frames using LZMA or LJ92\n");
#else
    print_msg(MSG_INFO, "  -c, -d              NOT AVAILABLE: compression support was not compiled into this release\n");
#endif
    print_msg(MSG_INFO, "  -p                  pass through original raw data without processing, it works for lossless or uncompressed raw\n");
    print_msg(MSG_INFO, "\n");

    print_msg(MSG_INFO, "-- bugfixes --\n");
    print_msg(MSG_INFO, "  --black-fix=value   set black level to <value> (fix green/magenta cast). if no value given, it will be set to 2048.\n");
    print_msg(MSG_INFO, "  --white-fix=value   set white level to <value>. if no value given, it will be set to 15000.\n");
    print_msg(MSG_INFO, "  --fix-bug=id        fix some special bugs. *only* to be used if given instruction by developers.\n");
    print_msg(MSG_INFO, "\n");
}

void print_sampling_info(int bin, int skip, char * what)
{
    if (bin + skip == 1) {
        print_msg(MSG_INFO, "read every %s", what);
    } else if (skip == 0) {
        print_msg(MSG_INFO, "bin %d %s%s", bin, what, bin == 1 ? "" : "s");
    } else {
        print_msg(MSG_INFO, "%s %d %s%s", bin == 1 ? "read" : "bin", bin, what, bin == 1 ? "" : "s");
        if (skip) {
            print_msg(MSG_INFO, ", skip %d", skip);
        }
    }
}

void print_capture_info(mlv_rawc_hdr_t * rawc)
{
    print_msg(
        MSG_INFO, "    raw_capture_info:\n"
    );
    print_msg(
        MSG_INFO, "      sensor res      %dx%d\n",
        rawc->raw_capture_info.sensor_res_x,
        rawc->raw_capture_info.sensor_res_y
    );
    print_msg(
        MSG_INFO, "      sensor crop     %d.%02d (%s)\n",
        rawc->raw_capture_info.sensor_crop / 100,
        rawc->raw_capture_info.sensor_crop % 100,
        rawc->raw_capture_info.sensor_crop == 100 ? "Full frame" : 
        rawc->raw_capture_info.sensor_crop == 162 ? "APS-C" : "35mm equiv"
    );
    
    int sampling_x = rawc->raw_capture_info.binning_x + rawc->raw_capture_info.skipping_x;
    int sampling_y = rawc->raw_capture_info.binning_y + rawc->raw_capture_info.skipping_y;
    
    print_msg(
        MSG_INFO, "      sampling        %dx%d (",
        sampling_y, sampling_x
    );
    print_sampling_info(
        rawc->raw_capture_info.binning_y,
        rawc->raw_capture_info.skipping_y,
        "line"
    );
    print_msg(MSG_INFO, ", ");
    print_sampling_info(
        rawc->raw_capture_info.binning_x,
        rawc->raw_capture_info.skipping_x,
        "column"
    );
    print_msg(MSG_INFO, ")\n");

    if (rawc->raw_capture_info.offset_x != -32768 &&
        rawc->raw_capture_info.offset_y != -32768)
    {
        print_msg(
            MSG_INFO, "      offset          %d,%d\n",
            rawc->raw_capture_info.offset_x,
            rawc->raw_capture_info.offset_y
        );
    }
}

int get_header_size(void *type)
{
#define HEADER_SIZE(h,s) do {if(!memcmp(type, h, 4)) { return sizeof(s); } } while(0)

    HEADER_SIZE("MLVI", mlv_file_hdr_t);
    HEADER_SIZE("VIDF", mlv_vidf_hdr_t);
    HEADER_SIZE("AUDF", mlv_vidf_hdr_t);
    HEADER_SIZE("RAWI", mlv_rawi_hdr_t);
    HEADER_SIZE("RAWC", mlv_rawc_hdr_t);
    HEADER_SIZE("WAVI", mlv_wavi_hdr_t);
    HEADER_SIZE("EXPO", mlv_expo_hdr_t);
    HEADER_SIZE("LENS", mlv_lens_hdr_t);
    HEADER_SIZE("RTCI", mlv_rtci_hdr_t);
    HEADER_SIZE("IDNT", mlv_idnt_hdr_t);
    HEADER_SIZE("XREF", mlv_xref_hdr_t);
    HEADER_SIZE("INFO", mlv_info_hdr_t);
    HEADER_SIZE("DISO", mlv_diso_hdr_t);
    HEADER_SIZE("MARK", mlv_mark_hdr_t);
    HEADER_SIZE("STYL", mlv_styl_hdr_t);
    HEADER_SIZE("ELVL", mlv_elvl_hdr_t);
    HEADER_SIZE("WBAL", mlv_wbal_hdr_t);
    HEADER_SIZE("DEBG", mlv_debg_hdr_t);
    HEADER_SIZE("VERS", mlv_vers_hdr_t);
    
    return 0;

#undef HEADER_SIZE
}


/* from ptpcam */
static void print_safe(char *buf, int size)
{
  int i;
  for (i=0; i<size; i++)
  {
    if ( buf[i] < ' ' || buf[i] > '~' )
    {
      print_msg(MSG_INFO, ".");
    } else {
      print_msg(MSG_INFO, "%c",buf[i]);
    }
  }
}

static void hexdump(char *buf, unsigned int size, unsigned int offset)
{
  unsigned int start_offset = offset;
  unsigned int i;
  char s[16];

  if ( offset % 16 != 0 )
  {
      print_msg(MSG_INFO, "0x%08X (+0x%04X)  ",offset, offset-start_offset);
      for (i=0; i<(offset%16); i++)
      {
        print_msg(MSG_INFO, "   ");
      }
      if ( offset % 16 > 8 )
      {
        print_msg(MSG_INFO, " ");
      }
      memset(s,' ',offset%16);
  }
  for (i=0; ; i++, offset++)
  {
    if ( offset % 16 == 0 )
    {
      if ( i > 0 )
      {
        print_msg(MSG_INFO, " |");
        print_safe(s,16);
        print_msg(MSG_INFO, "|\n");
      }
      print_msg(MSG_INFO, "0x%08X (+0x%04X)",offset, offset-start_offset);
      if (i < size)
      {
        print_msg(MSG_INFO, " ");
      }
    }
    if ( offset % 8 == 0 )
    {
      print_msg(MSG_INFO, " ");
    }
    if ( i == size )
    {
      break;
    }
    print_msg(MSG_INFO, "%02x ",(unsigned char) buf[i]);
    s[offset%16] = buf[i];
  }
  if ( offset % 16 != 0 )
  {
      for (i=0; i<16-(offset%16); i++)
      {
        print_msg(MSG_INFO, "   ");
      }
      if ( offset % 16 < 8 )
      {
        print_msg(MSG_INFO, " ");
      }
      memset(s+(offset%16),' ',16-(offset%16));
      print_msg(MSG_INFO, " |");
      print_safe(s,16);
      print_msg(MSG_INFO, "|");
  }
  print_msg(MSG_INFO, "\n");
}

/* rescale black and white levels if bit depth is changed (-b) and/or fix levels if --black/white-fix specified */
static void fix_black_white_level(int32_t * black_level, int32_t * white_level, int32_t * bitdepth, int32_t new_bitdepth, int black_fix, int white_fix, int verbose)
{
    int32_t old_black = *black_level;
    int32_t old_white = *white_level;

    if(new_bitdepth)
    {
        int delta = *bitdepth - new_bitdepth;
        
        /* scale down black and white level */
        if(delta)
        {
            *bitdepth = new_bitdepth;
        
            if(delta > 0)
            {
                *black_level >>= delta;
                *white_level >>= delta;
            }
            else
            {
                *black_level <<= ABS(delta);
                *white_level <<= ABS(delta);
            }

            if(verbose)
            {
                if(!black_fix)
                    print_msg(MSG_INFO, "   black: %d -> %d\n", old_black, *black_level);
                if(!white_fix)
                    print_msg(MSG_INFO, "   white: %d -> %d\n", old_white, *white_level);
            }
        }
    }
    
    if(black_fix)
    {
        *black_level = black_fix;
        if(verbose)
        {
            print_msg(MSG_INFO, "   black: %d -> %d (forced)\n", old_black, *black_level);
        }
    }
    
    if(white_fix)
    {
        *white_level = white_fix;
        if(verbose)
        {
            print_msg(MSG_INFO, "   white: %d -> %d (forced)\n", old_white, *white_level);
        }
    }
}


int main (int argc, char *argv[])
{
    char *input_filename = NULL;
    char *output_filename = NULL;
    char *subtract_filename = NULL;
    char *flatfield_filename = NULL;
    char *lut_filename = NULL;
    char *extract_block = NULL;
    char *inject_filename = NULL;
    int blocks_processed = 0;

    int extract_frames = 0;
    uint32_t frame_start = 0;
    uint32_t frame_end = UINT32_MAX;
    uint32_t audf_frames_processed = 0;
    uint32_t vidf_frames_processed = 0;
    uint32_t vidf_max_number = 0;

    int version = 0;
    int delta_encode_mode = 0;
    int xref_mode = 0;
    int average_mode = 0;
    int average_vert = 0;
    int average_hor = 0;
    int subtract_mode = 0;
    int flatfield_mode = 0;
    int average_samples = 0;
    int relaxed = 0;
    int visualize = 0;
    int skip_xref = 0;

    int mlv_output = 0;
    int raw_output = 0;
    int bit_depth = 0;
    int bit_zap = 0;
    int compress_output = 0;
    int decompress_input = 0;
    int verbose = 0;
    int alter_fps = 0;
    int pass_through = 0;
    char opt = ' ';

    int video_xRes = 0;
    int video_yRes = 0;

    lua_State *lua_state = NULL;

    /* long options */
    int chroma_smooth_method = 0;
    int black_fix = 0;
    int white_fix = 0;
    int dng_output = 0;
    int dump_xrefs = 0;
    int fix_focus_pixels = 1;
    int fix_cold_pixels = 1;
    int fix_vert_stripes = 1;
    int is_dual_iso = 0;
    int save_bpm_file = 0;
    int fix_pattern_noise = 0;
    int deflicker_target = 0;
    int show_progress = 0;
    int pack_dng_bits = 1;
    int no_audio = 0;
    int fpi_method = 0; // default is 'mlvfs'
    int bpi_method = 0; // default is 'mlvfs'
    int crop_rec = 0;
    
    /* helper structs for DNG exporting */
    struct frame_info frame_info = { 0 };
    struct dng_data dng_data = { 0, 0, 0, 0, NULL, NULL, NULL, NULL };
    
    enum bug_id fix_bug = BUG_ID_NONE;
    
    /* MLV autopsy */
    enum autopsy_content_type
    {
        AUTOPSY_BLOCK = 0,
        AUTOPSY_HEADER = 1,
        AUTOPSY_PAYLOAD = 2
    };
    enum autopsy_mode_type
    {
        AUTOPSY_OFF = 0,
        AUTOPSY_EXTRACT = 1,
        AUTOPSY_EXTRACT_TYPE = 2,
        AUTOPSY_REPLACE = 3,
        AUTOPSY_SKIP_BLOCK = 4,
        AUTOPSY_SKIP_TYPE = 5
    };
    enum autopsy_dump_type
    {
        AUTOPSY_DUMP_FILE = 0,
        AUTOPSY_DUMP_HEX = 1,
        AUTOPSY_DUMP_ASCII = 2
    };
    
    /* return/status codes for block handler functions */
    typedef enum 
    {
        PROCESS_MISSING,
        PROCESS_OK,
        PROCESS_SKIP,
        PROCESS_ERROR
    } process_result_e;
    
    int autopsy_mode = AUTOPSY_OFF;
    int autopsy_content = AUTOPSY_BLOCK;
    int autopsy_block = 0;
    int autopsy_dump = AUTOPSY_DUMP_FILE;
    char *autopsy_block_type = "    ";
    char *autopsy_file = "autopsy.bin";
    
    struct option long_options[] = {
        {"version",  no_argument, &version,  1 },
        {"lua",    required_argument, NULL,  'L' },
        {"black-fix",  optional_argument, NULL,  'B' },
        {"white-fix",  optional_argument, NULL,  'W' },
        {"fix-bug",  required_argument, NULL,  'F' },
        {"batch",  no_argument, &batch_mode,  1 },
        {"dump-xrefs",   no_argument, &dump_xrefs,  1 },
        {"dng",    no_argument, &dng_output,  1 },
        {"no-cs",  no_argument, &chroma_smooth_method,  0 },
        {"cs2x2",  no_argument, &chroma_smooth_method,  2 },
        {"cs3x3",  no_argument, &chroma_smooth_method,  3 },
        {"cs5x5",  no_argument, &chroma_smooth_method,  5 },
        {"no-fixfp",  no_argument, &fix_focus_pixels,  0 },
        {"no-fixcp",  no_argument, &fix_cold_pixels,  0 },
        {"fixcp2",    no_argument, &fix_cold_pixels,  2 },
        {"no-stripes",  no_argument, &fix_vert_stripes,  0 },
        {"avg-vertical",  no_argument, &average_vert,  1 },
        {"avg-horizontal",  no_argument, &average_hor,  1 },
        {"is-dualiso",    no_argument, &is_dual_iso,  1 },
        {"is-croprec",    no_argument, &crop_rec,  1 },
        {"save-bpm",    no_argument, &save_bpm_file,  1 },
        {"force-stripes",  no_argument, &fix_vert_stripes,  2 },
        {"fixpn",  no_argument, &fix_pattern_noise,  1 },
        {"deflicker",  optional_argument, NULL,  'D' },
        {"show-progress",  no_argument, &show_progress,  1 },
        {"no-bitpack",  no_argument, &pack_dng_bits,  0 },
        {"no-audio",  no_argument, &no_audio,  1 },
        {"fpi",     required_argument, NULL,  'i' },
        {"bpi",     required_argument, NULL,  'j' },
        
        /* MLV autopsy */
        {"relaxed",       no_argument, &relaxed,  1 },
        {"visualize",     no_argument, &visualize,  1 },
        {"skip-xref",     no_argument, &skip_xref,  1 },
        {"hex",           no_argument, &autopsy_dump,  AUTOPSY_DUMP_HEX },
        {"ascii",         no_argument, &autopsy_dump,  AUTOPSY_DUMP_ASCII },
        {"skip-type",     required_argument, NULL,  'T' },
        {"skip-block",    required_argument, NULL,  'U' },
        {"extract-type",  required_argument, NULL,  'S' },
        {"extract",       required_argument, NULL,  'Y' },
        {"replace",       required_argument, NULL,  'Z' },
        {"autopsy-file",  required_argument, NULL,  'V' },
        {"header-only",   no_argument, &autopsy_content,  (int)AUTOPSY_HEADER },
        {"payload-only",  no_argument, &autopsy_content,  (int)AUTOPSY_PAYLOAD },
        
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
    while ((opt = getopt_long(argc, argv, "A:F:B:W:L:S:T:V:X:Y:Z:I:D:t:xz:eas:uvrcdpo:l:b:f:i:j:", long_options, &index)) != -1)
    {
        switch (opt)
        {
            case 'Y':
                autopsy_mode = AUTOPSY_EXTRACT;
                autopsy_block = atoi(optarg);
                break;
                
            case 'Z':
                autopsy_mode = AUTOPSY_REPLACE;
                autopsy_block = atoi(optarg);
                break;
                
            case 'U':
                autopsy_mode = AUTOPSY_SKIP_BLOCK;
                autopsy_block = atoi(optarg);
                break;
                
            case 'S':
                autopsy_mode = AUTOPSY_EXTRACT_TYPE;
                autopsy_block_type = strdup(optarg);
                
                if(strlen(autopsy_block_type) != 4)
                {
                    print_msg(MSG_ERROR, "Error: Block types must be 4 characters\n");
                    return ERR_PARAM;
                }
                break;
                
            case 'T':
                autopsy_mode = AUTOPSY_SKIP_TYPE;
                autopsy_block_type = strdup(optarg);
                
                if(strlen(autopsy_block_type) != 4)
                {
                    print_msg(MSG_ERROR, "Error: Block types must be 4 characters\n");
                    return ERR_PARAM;
                }
                break;
                
            case 'V':
                autopsy_file = strdup(optarg);
                break;
              
            case 'F':
                if(!optarg)
                {
                    print_msg(MSG_ERROR, "Error: Missing bug ID. possible ones:\n");
                    print_msg(MSG_ERROR, "    #3 - fix invalid NULL block size\n");
                    return ERR_PARAM;
                }
                else
                {
                    fix_bug = MIN(16384, MAX(1, atoi(optarg)));
                    print_msg(MSG_INFO, "FIX BUG #%d [active]\n", fix_bug);
                }
                break;
              
            case 'W':
                if(!optarg)
                {
                    white_fix = 15000;
                }
                else
                {
                    white_fix = MIN(16384, MAX(1, atoi(optarg)));
                }
                break;
                
            case 'B':
                if(!optarg)
                {
                    black_fix = 2048;
                }
                else
                {
                    black_fix = MIN(16384, MAX(1, atoi(optarg)));
                }
                break;
                
            case 'D':
                if(!optarg)
                {
                    deflicker_target = 3072;
                }
                else
                {
                    deflicker_target = MIN(16384, MAX(1, atoi(optarg)));
                }
                break;
            
            case 'A':
                if(!optarg)
                {
                    print_msg(MSG_ERROR, "Error: Missing parameter FPSx1000\n");
                    return ERR_PARAM;
                }
                else
                {
                    alter_fps = MAX(1, atoi(optarg));
                }
                break;
                
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

            case 'e':
                delta_encode_mode = 1;
                break;

            case 'a':
                average_mode = 1;
                decompress_input = 1;
                break;

            case 's':
                if(!optarg)
                {
                    print_msg(MSG_ERROR, "Error: Missing subtract frame filename\n");
                    return ERR_PARAM;
                }
                subtract_filename = strdup(optarg);
                subtract_mode = 1;
                decompress_input = 1;
                break;

            case 't':
                if(!optarg)
                {
                    print_msg(MSG_ERROR, "Error: Missing flat-field frame filename\n");
                    return ERR_PARAM;
                }
                flatfield_filename = strdup(optarg);
                flatfield_mode = 1;
                decompress_input = 1;
                break;

            case 'X':
                if(!optarg || strlen(optarg) != 4)
                {
                    print_msg(MSG_ERROR, "Error: Missing block type. e.g. MLVI or RAWI\n");
                    return ERR_PARAM;
                }
                extract_block = strdup(optarg);
                break;

            case 'I':
                if(!optarg)
                {
                    print_msg(MSG_ERROR, "Error: Missing filename of data to inject\n");
                    return ERR_PARAM;
                }
                inject_filename = strdup(optarg);
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
#if defined(MLV_USE_LJ92)
                compress_output = (!pass_through) ? 1 : 0;
#else
                print_msg(MSG_ERROR, "Error: Compression support was not compiled into this release\n");
                return ERR_PARAM;
#endif
                break;

            case 'd':
#if defined(MLV_USE_LZMA) || defined(MLV_USE_LJ92)
                decompress_input = 1;
#else
                print_msg(MSG_ERROR, "Error: Compression support was not compiled into this release\n");
                return ERR_PARAM;
#endif
                break;

            case 'p':
                pass_through = (!compress_output) ? 1 : 0;
                break;

            case 'o':
                if(!optarg)
                {
                    print_msg(MSG_ERROR, "Error: Missing output filename\n");
                    return ERR_PARAM;
                }
                output_filename = strdup(optarg);
                break;

            case 'f':
                {
                    extract_frames = 1;

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

            case 'i':
                fpi_method = MIN(1, MAX(0, atoi(optarg)));
                break;

            case 'j':
                bpi_method = MIN(1, MAX(0, atoi(optarg)));
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

    print_msg(MSG_INFO, "\n");
    print_msg(MSG_INFO, " MLV Dumper\n");
    print_msg(MSG_INFO, "-----------------\n");
    print_msg(MSG_INFO, "\n");

    if(version)
    {
        const char *last_update = module_get_string(mlv_dump_strings, "Last update");
        const char *build_date = module_get_string(mlv_dump_strings, "Build date");
        
        print_msg(MSG_INFO, "Last update:  %s", last_update);
        print_msg(MSG_INFO, "Build date:   %s", build_date);
        print_msg(MSG_INFO, "\n");
        print_msg(MSG_INFO, "\n");
        return 0;
    }
    
    if(optind >= argc)
    {
        print_msg(MSG_ERROR, "Error: Missing input filename\n");
        show_usage(argv[0]);
        return ERR_PARAM;
    }

    
    /* get first file */
    input_filename = argv[optind];

    print_msg(MSG_INFO, "Mode of operation:\n");
    print_msg(MSG_INFO, "   - Input MLV file: '%s'\n", input_filename);

    if(verbose)
    {
        print_msg(MSG_INFO, "   - Verbose messages\n");
        show_progress = 0; // ignore "--show-progress", incompatible with verbose mode
    }

    if(batch_mode)
    {
        print_msg(MSG_INFO, "   - Batch processing suitable output\n");
        show_progress = 0; // ignore "--show-progress", incompatible with batch mode
    }
    
    if(black_fix)
    {
        print_msg(MSG_INFO, "   - Setting black level to %d\n", black_fix);
    }
    
    if(white_fix)
    {
        print_msg(MSG_INFO, "   - Setting white level to %d\n", white_fix);
    }

    if(alter_fps)
    {
        print_msg(MSG_INFO, "   - Altering FPS metadata for %d/1000 fps\n", alter_fps);
    }
    
    if(dng_output)
    {
        /* correct handling of bit depth conversion for DNG output */
        if(compress_output) 
        {
            print_msg(MSG_INFO, "   - Compress frames written into DNG (slow)\n");
            if(bit_depth)
            {
                /* ignore "-b" switch */
                print_msg(MSG_INFO, "   - WARNING: Ignoring bit depth conversion\n");
                bit_depth = 0;
            }
        }
        else if(pass_through)
        {
            print_msg(MSG_INFO, "   - Writing original (compressed/uncompressed) payload into DNG\n");
            print_msg(MSG_INFO, "   - WARNING: These DNGs will not undergo any preprocessing like stripe fix etc\n");
            if(bit_depth)
            {
                /* ignore "-b" switch */
                print_msg(MSG_INFO, "   - WARNING: Ignoring bit depth conversion\n");
                bit_depth = 0;
            }
        }
        else
        {
            /* decompress input before processing in case it's compressed.
               if "-b" switch used do bit depth convertion, no depth forcing/ignoring is done */
            decompress_input = 1;
        }
        
        /* special case - splitting into frames doesnt require a specific output file */
        if(!output_filename)
        {
            int len = strlen(input_filename) + 16;
            output_filename = malloc(len);

            strcpy(output_filename, input_filename);

            char *ext_dot = strrchr(output_filename, '.');
            if(ext_dot)
            {
                *ext_dot = '\000';
            }

            strcat(output_filename, "_");
            print_msg(MSG_INFO, "   - Using output path '%s' for DNGs\n", output_filename);
        }
    }

    /* display and set/unset variables according to parameters to have a consistent state */
    if(output_filename)
    {
        if(dng_output)
        {
            print_msg(MSG_INFO, "   - Convert to DNG frames\n");

            delta_encode_mode = 0;
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
                if(average_vert)
                {
                    print_msg(MSG_INFO, "   - Also average the images in vertical direction to extract vertical banding\n");
                }
                if(average_hor)
                {
                    print_msg(MSG_INFO, "   - Also average the images in horizontal direction to extract horizontal banding\n");
                }
            }
            if(extract_block)
            {
                print_msg(MSG_INFO, "   - But only write '%s' blocks\n", extract_block);
            }
            if(inject_filename)
            {
                print_msg(MSG_INFO, "   - Inject data from '%s'\n", inject_filename);
            }
        }

        if(subtract_mode)
        {
            print_msg(MSG_INFO, "   - Subtract reference frame '%s'\n", subtract_filename);
        }
        if(flatfield_mode)
        {
            print_msg(MSG_INFO, "   - Flat-field reference frame '%s'\n", flatfield_filename);
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
    mlv_diso_hdr_t diso_info;
    mlv_lens_hdr_t lens_info;
    mlv_expo_hdr_t expo_info;
    mlv_idnt_hdr_t idnt_info;
    mlv_wbal_hdr_t wbal_info;
    mlv_wavi_hdr_t wavi_info;
    mlv_rtci_hdr_t rtci_info;
    mlv_rawc_hdr_t rawc_info;
    mlv_vidf_hdr_t last_vidf;

    /* initialize stuff */
    memset(&lv_rec_footer, 0x00, sizeof(lv_rec_file_footer_t));
    memset(&diso_info, 0x00, sizeof(mlv_diso_hdr_t));
    memset(&lens_info, 0x00, sizeof(mlv_lens_hdr_t));
    memset(&expo_info, 0x00, sizeof(mlv_expo_hdr_t));
    memset(&idnt_info, 0x00, sizeof(mlv_idnt_hdr_t));
    memset(&wbal_info, 0x00, sizeof(mlv_wbal_hdr_t));
    memset(&wavi_info, 0x00, sizeof(mlv_wavi_hdr_t));
    memset(&rtci_info, 0x00, sizeof(mlv_rtci_hdr_t));
    memset(&rawc_info, 0x00, sizeof(mlv_rawc_hdr_t));
    memset(&main_header, 0x00, sizeof(mlv_file_hdr_t));

    char info_string[1024] = "";

    /* this table contains the XREF chunk read from idx file, if existing */
    mlv_xref_hdr_t *block_xref = NULL;
    mlv_xref_t *xrefs = NULL;
    uint32_t block_xref_pos = 0;

    uint32_t frame_buffer_size = 1*1024*1024;
    uint32_t subtract_frame_buffer_size = 0;
    uint32_t flatfield_frame_buffer_size = 0;

    uint32_t *frame_arith_buffer = NULL;
    uint8_t *frame_sub_buffer = NULL;
    uint8_t *frame_flat_buffer = NULL;
    uint8_t *frame_buffer = NULL;
    uint8_t *prev_frame_buffer = NULL;

    FILE *out_file = NULL;
    FILE *out_file_wav = NULL;
    FILE **in_files = NULL;
    FILE *in_file = NULL;

    int in_file_count = 0;
    int in_file_num = 0;

    uint32_t wav_data_size = 0; /* WAV format supports only 32-bit size */
    uint32_t wav_header_size = 0;

    /* this is for our generated XREF table */
    frame_xref_t *frame_xref_table = NULL;
    int frame_xref_allocated = 0;
    int frame_xref_entries = 0;

    int total_vidf_count = 0;
    int total_audf_count = 0;

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

    if(!xref_mode && !skip_xref)
    {
        block_xref = load_index(input_filename);

        if(block_xref)
        {
            if(block_xref->entryCount == 0)
            {
                print_msg(MSG_INFO, "Empty XREF table, will be ignored\n");
                free(block_xref);
                block_xref= NULL;
            }
            else
            {
                print_msg(MSG_INFO, "XREF table contains %d entries\n", block_xref->entryCount);
                xrefs = (mlv_xref_t *)(block_xref + 1);

                if(dump_xrefs)
                {
                    xref_dump(block_xref);
                }
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

    /* this block will load an image from a MLV file, so use its reported frame size for future use */
    if(subtract_mode)
    {
        print_msg(MSG_INFO, "Loading subtract (dark) frame '%s'\n", subtract_filename);
        int ret = load_frame(subtract_filename, &frame_sub_buffer, &subtract_frame_buffer_size);

        if(ret)
        {
            print_msg(MSG_ERROR, "Failed to load subtract frame (%d)\n", ret);
            return ERR_FILE;
        }
    }

    if(flatfield_mode)
    {
        print_msg(MSG_INFO, "Loading flat-field frame '%s'\n", flatfield_filename);
        int ret = load_frame(flatfield_filename, &frame_flat_buffer, &flatfield_frame_buffer_size);

        if(ret)
        {
            print_msg(MSG_ERROR, "Failed to load flat-field frame (%d)\n", ret);
            return ERR_FILE;
        }
    }

    
    if(output_filename || lua_state)
    {
        frame_buffer = malloc(frame_buffer_size);
        if(!frame_buffer)
        {
            print_msg(MSG_ERROR, "Failed to alloc mem\n");
            return ERR_MALLOC;
        }
        memset(frame_buffer, 0x00, frame_buffer_size);

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
    uint32_t mlv_block_size = 8192*1024;
    mlv_hdr_t *mlv_block = malloc(mlv_block_size);
    
    do
    {
        int handled_write = 0;
        uint64_t position = 0;

read_headers:

        print_msg(MSG_PROGRESS, "B:%d/%d V:%d/%d A:%d/%d\n", blocks_processed, block_xref?block_xref->entryCount:0, vidf_frames_processed, total_vidf_count, audf_frames_processed, total_audf_count);

        if(block_xref)
        {
            /* get the file and position of the next block */
            in_file_num = xrefs[block_xref_pos].fileNumber;
            position = xrefs[block_xref_pos].frameOffset;

            /* select file and seek to the right position */
            if(in_file_num >= in_file_count)
            {
                print_msg(MSG_ERROR, "Index points to non-existent file #%d (delete the .IDX file or start with -x)\n", in_file_num);
                return ERR_FILE;
            }
            in_file = in_files[in_file_num];
            file_set_pos(in_file, position, SEEK_SET);
        }

        position = file_get_pos(in_file);

        if(fread(mlv_block, sizeof(mlv_hdr_t), 1, in_file) != 1)
        {
            print_msg(MSG_INFO, "\n");
            
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

        /* unexpected block header size? */
        if(mlv_block->blockSize < sizeof(mlv_hdr_t) || mlv_block->blockSize > 50 * 1024 * 1024)
        {
            if(fix_bug == BUG_ID_NULL_SIZE_ZERO && !memcmp(mlv_block->blockType, "NULL", 4))
            {
                int padded_size = (position + sizeof(mlv_hdr_t) + 511) & ~511;
                mlv_block->blockSize = padded_size - position;
            }
            else
            {
                print_msg(MSG_ERROR, "Invalid block size at position 0x%08" PRIx64 "\n", position);
                goto abort;
            }
        }
        
        /* will the buffer fit its size? */
        if(mlv_block->blockSize > mlv_block_size)
        {
            mlv_block_size = mlv_block->blockSize;
            mlv_block = realloc(mlv_block, mlv_block_size);
            
            if(!mlv_block)
            {
                print_msg(MSG_ERROR, "Invalid block size of 0x%08X at position 0x%08" PRIx64 "\n", mlv_block_size, position);
                goto abort;
            }
        }

        /* jump back to the beginning of the block just read and read it all */
        file_set_pos(in_file, position, SEEK_SET);
        
        if(fread(mlv_block, mlv_block->blockSize, 1, in_file) != 1)
        {
            print_msg(MSG_ERROR, "Invalid block size of 0x%08X at position 0x%08" PRIx64 ", file ended prematurely\n", mlv_block->blockSize, position);
            goto abort;
        }
        
        lua_handle_hdr(lua_state, mlv_block->blockType, &mlv_block, mlv_block->blockSize);
        
        /* show all block types in a more convenient style, but needs little housekeeping code */
        if(visualize)
        {
            static uint8_t last_type[4];
            
            /* housekeeping here */
            if(!memcmp(mlv_block->blockType, "VIDF", 4))
            {
                vidf_frames_processed++;
            }
            if(!memcmp(mlv_block->blockType, "AUDF", 4))
            {
                audf_frames_processed++;
            }
            if(!memcmp(mlv_block->blockType, "MLVI", 4))
            {
                mlv_file_hdr_t file_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_file_hdr_t), mlv_block->blockSize);

                /* read the whole header block, but limit size to either our local type size or the written block size */
                if(fread(&file_hdr, hdr_size, 1, in_file) != 1)
                {
                    print_msg(MSG_ERROR, "File ends in the middle of a block\n");
                    goto abort;
                }

                /* is this the first file? */
                if(main_header.fileGuid == 0)
                {
                    /* correct header size if needed */
                    file_hdr.blockSize = sizeof(mlv_file_hdr_t);

                    memcpy(&main_header, &file_hdr, sizeof(mlv_file_hdr_t));
                }
            }
            
            /* repetetive frames are printed with a plus */
            if(!memcmp(mlv_block->blockType, last_type, 4))
            {
                print_msg(MSG_INFO, "+");
            }
            else
            {
                /* one line per video frame, if not repeated */
                if(!memcmp(mlv_block->blockType, "VIDF", 4))
                {
                    print_msg(MSG_INFO, "\n");
                }
                
                print_msg(MSG_INFO, "[%c%c%c%c]", mlv_block->blockType[0], mlv_block->blockType[1], mlv_block->blockType[2], mlv_block->blockType[3]);
                
                /* also a new line after header */
                if(!memcmp(mlv_block->blockType, "MLVI", 4))
                {
                    print_msg(MSG_INFO, "\n");
                }
                
                /* remember last type */
                memcpy(last_type, mlv_block->blockType, 4);
            }
            
            goto skip_block;
        }
        
        if(autopsy_mode)
        {
            if((blocks_processed == autopsy_block) || (autopsy_mode == AUTOPSY_SKIP_TYPE) || (autopsy_mode == AUTOPSY_EXTRACT_TYPE))
            {
                switch(autopsy_mode)
                {
                    case AUTOPSY_SKIP_BLOCK:
                    {
                        goto skip_block;
                    }

                    case AUTOPSY_SKIP_TYPE:
                    {
                        if(!memcmp(autopsy_block_type, mlv_block->blockType, 4))
                        {
                            goto skip_block;
                        }
                    }

                    case AUTOPSY_EXTRACT_TYPE:
                    case AUTOPSY_EXTRACT:
                    {
                        /* by default, dump all of the block's data */
                        uint32_t start = 0;
                        uint32_t length = mlv_block->blockSize;
                        
                        /* for extracting specific types, we have to check the type as we are called for every block */
                        if(autopsy_mode == AUTOPSY_EXTRACT_TYPE)
                        {
                            /* not the block we want to extract? go on */
                            if(memcmp(autopsy_block_type, mlv_block->blockType, 4))
                            {
                                break;
                            }
                        }
                    
                        FILE *autopsy_handle = NULL;
                        
                        /* only operate on file, if not dumping hex */
                        if(!autopsy_dump)
                        {
                            autopsy_handle = fopen(autopsy_file, "wb+");
                            
                            if(!autopsy_handle)
                            {
                                print_msg(MSG_ERROR, "Failed opening autopsy file\n");
                                goto abort;
                            }
                        }
                        
                        /* allocate a null-terminatable buffer */
                        uint8_t *autopsy_buf = malloc(length + 1);
                        if(!autopsy_buf)
                        {
                            print_msg(MSG_ERROR, "Failed to allocate buffer for data to extract\n");
                            goto abort;
                        }
                        autopsy_buf[length] = 0;
                        
                        if(fread(autopsy_buf, length, 1, in_file) != 1)
                        {
                            print_msg(MSG_ERROR, "Failed to read data from input file\n");
                            goto abort;
                        }
                        
                        /* rewind to block start, as if noone played with it */
                        file_set_pos(in_file, position, SEEK_SET);
                        
                        switch(autopsy_content)
                        {
                            case AUTOPSY_HEADER:
                            {
                                /* header only, so jsut reduce length */
                                length = get_header_size(autopsy_buf);
                                
                                if(!length)
                                {
                                    print_msg(MSG_ERROR, "Error: Unknown block type '%c%c%c%c', cannot determine its header size.\n", autopsy_buf[0], autopsy_buf[1], autopsy_buf[2], autopsy_buf[3]);
                                    goto abort;
                                }
                                break;
                            }
                            
                            case AUTOPSY_PAYLOAD:
                            {
                                /* payload only, so skip header */
                                int header_length = get_header_size(autopsy_buf);
                                
                                if(!header_length)
                                {
                                    print_msg(MSG_ERROR, "Error: Unknown block type '%c%c%c%c', cannot determine its header size.\n", autopsy_buf[0], autopsy_buf[1], autopsy_buf[2], autopsy_buf[3]);
                                    goto abort;
                                }
                                
                                length -= header_length;
                                start = header_length;
                                
                                if(!length)
                                {
                                    print_msg(MSG_ERROR, "Error: No payload available for this block type '%c%c%c%c'.\n", autopsy_buf[0], autopsy_buf[1], autopsy_buf[2], autopsy_buf[3]);
                                    goto abort;
                                }
                                break;
                            }
                        }
                        
                        /* make sure not to read beyond source buffer */
                        if(start + length > mlv_block->blockSize)
                        {
                            print_msg(MSG_ERROR, "Error: Block type '%c%c%c%c' has invalid size.\n", autopsy_buf[0], autopsy_buf[1], autopsy_buf[2], autopsy_buf[3]);
                            goto abort;
                        }
                        
                        switch(autopsy_dump)
                        {
                            case AUTOPSY_DUMP_FILE:
                            {
                                /* write data int autopsy file */
                                if(fwrite(&autopsy_buf[start], length, 1, autopsy_handle) != 1)
                                {
                                    print_msg(MSG_ERROR, "Failed writing into autopsy file\n");
                                    goto abort;
                                }
                                
                                fclose(autopsy_handle);
                                break;
                            }
                            
                            case AUTOPSY_DUMP_HEX:
                            {
                                print_msg(MSG_INFO, "--- Hex display ---\n");
                                print_msg(MSG_INFO, "   Block: %c%c%c%c\n", mlv_block->blockType[0], mlv_block->blockType[1], mlv_block->blockType[2], mlv_block->blockType[3]);
                                print_msg(MSG_INFO, "  Offset: 0x%08" PRIx64 "\n", position);
                                print_msg(MSG_INFO, "  Number: %d\n", blocks_processed);
                                print_msg(MSG_INFO, "    Size: %d\n", mlv_block->blockSize);

                                hexdump((char *)&autopsy_buf[start], length, start);
                                print_msg(MSG_INFO, "--------------------\n");
                                print_msg(MSG_INFO, "\n");
                                break;
                            }
                            
                            case AUTOPSY_DUMP_ASCII:
                            {
                                print_msg(MSG_INFO, "--- ASCII display ---\n");
                                print_msg(MSG_INFO, "   Block: %c%c%c%c\n", mlv_block->blockType[0], mlv_block->blockType[1], mlv_block->blockType[2], mlv_block->blockType[3]);
                                print_msg(MSG_INFO, "  Offset: 0x%08" PRIx64 "\n", position);
                                print_msg(MSG_INFO, "  Number: %d\n", blocks_processed);
                                print_msg(MSG_INFO, "    Size: %d\n", mlv_block->blockSize);
                                print_msg(MSG_INFO, " Content: \"%s\"\n", &autopsy_buf[start]);
                                print_msg(MSG_INFO, "---------------------\n");
                                print_msg(MSG_INFO, "\n");
                                break;
                            }
                        }
                        
                        free(autopsy_buf);
                        
                        /* when extracting block types, keep rolling, there might be more */
                        if(autopsy_mode != AUTOPSY_EXTRACT_TYPE)
                        {
                            goto abort;
                        }
                        break;
                    }

                    case AUTOPSY_REPLACE:
                    {
                        if(!output_filename)
                        {
                            print_msg(MSG_ERROR, "Need some output file, else manipulation would make no sense\n");
                            goto abort;
                        }
                        
                        FILE *autopsy_handle = fopen(autopsy_file, "rb");
                        
                        if(!autopsy_handle)
                        {
                            print_msg(MSG_ERROR, "Failed opening autopsy file\n");
                            goto abort;
                        }
                        
                        /* first get content length of autopsy file */
                        file_set_pos(autopsy_handle, 0, SEEK_END);
                        uint32_t autopsy_size = file_get_pos(autopsy_handle);
                        file_set_pos(autopsy_handle, 0, SEEK_SET);
                        
                        switch(autopsy_content)
                        {
                            case AUTOPSY_HEADER:
                            {
                                /* to replace header, read original block and replace data */
                                mlv_hdr_t *autopsy_block = malloc(mlv_block->blockSize);
                                mlv_hdr_t *autopsy_header = malloc(autopsy_size);
                                
                                if(!autopsy_block || !autopsy_header)
                                {
                                    print_msg(MSG_ERROR, "Failed to allocate buffer for data to extract\n");
                                    goto abort;
                                }
                                
                                /* read original block from input file */
                                if(fread(autopsy_block, mlv_block->blockSize, 1, in_file) != 1)
                                {
                                    print_msg(MSG_ERROR, "Failed to read data from input file\n");
                                    goto abort;
                                }
                        
                                /* read new header from autopsy file */
                                if(fread(autopsy_header, autopsy_size, 1, autopsy_handle) != 1)
                                {
                                    print_msg(MSG_ERROR, "Failed to read data from autopsy file\n");
                                    goto abort;
                                }
                                
                                /* patch autopsy header to match block size */
                                int old_header_size = get_header_size(autopsy_block);
                                
                                if(!old_header_size)
                                {
                                    print_msg(MSG_ERROR, "Error: Unknown block type '%c%c%c%c', cannot determine its header size.\n", autopsy_block->blockType[0], autopsy_block->blockType[1], autopsy_block->blockType[2], autopsy_block->blockType[3]);
                                    goto abort;
                                }
                                
                                /* calculate new block size from payload length */
                                int payload_size = autopsy_block->blockSize - old_header_size;
                                autopsy_header->blockSize = autopsy_size + payload_size;
                                
                                /* write new patched header */
                                if(fwrite(autopsy_header, autopsy_size, 1, out_file) != 1)
                                {
                                    print_msg(MSG_ERROR, "Failed writing into .MLV file\n");
                                    goto abort;
                                }
                                
                                /* write original payload */
                                if(fwrite(&((uint8_t*)autopsy_block)[old_header_size], payload_size, 1, out_file) != 1)
                                {
                                    print_msg(MSG_ERROR, "Failed writing into .MLV file\n");
                                    goto abort;
                                }
                                
                                free(autopsy_block);
                                free(autopsy_header);
                                break;
                            }
                                
                            case AUTOPSY_PAYLOAD:
                            {
                                /* read original header and use payload from autopsy file */
                                int header_size = get_header_size(mlv_block);
                                
                                if(!header_size)
                                {
                                    print_msg(MSG_ERROR, "Error: Unknown block type '%c%c%c%c', cannot determine its header size.\n", mlv_block->blockType[0], mlv_block->blockType[1], mlv_block->blockType[2], mlv_block->blockType[3]);
                                    goto abort;
                                }
                                
                                mlv_hdr_t *autopsy_header = malloc(header_size);
                                mlv_hdr_t *autopsy_payload = malloc(autopsy_size);
                                
                                if(!autopsy_header || !autopsy_payload)
                                {
                                    print_msg(MSG_ERROR, "Failed to allocate buffer for data to extract\n");
                                    goto abort;
                                }
                                
                                /* read original header from input file */
                                if(fread(autopsy_header, header_size, 1, in_file) != 1)
                                {
                                    print_msg(MSG_ERROR, "Failed to read data from input file\n");
                                    goto abort;
                                }
                        
                                /* calculate new block size from payload length */
                                autopsy_header->blockSize = header_size + autopsy_size;
                                
                                /* write new patched header */
                                if(fwrite(autopsy_header, header_size, 1, out_file) != 1)
                                {
                                    print_msg(MSG_ERROR, "Failed writing into .MLV file\n");
                                    goto abort;
                                }
                                
                                /* read new payload from autopsy file */
                                if(fread(autopsy_payload, autopsy_size, 1, autopsy_handle) != 1)
                                {
                                    print_msg(MSG_ERROR, "Failed to read data from autopsy file\n");
                                    goto abort;
                                }
                                
                                /* write replaced payload */
                                if(fwrite(autopsy_payload, autopsy_size, 1, out_file) != 1)
                                {
                                    print_msg(MSG_ERROR, "Failed writing into .MLV file\n");
                                    goto abort;
                                }
                                
                                free(autopsy_header);
                                free(autopsy_payload);
                                break;
                            }

                            case AUTOPSY_DUMP_HEX:
                            {
                                print_msg(MSG_INFO, "--- Hex display ---\n");
                                print_msg(MSG_INFO, "   Block: %c%c%c%c\n", buf.blockType[0], buf.blockType[1], buf.blockType[2], buf.blockType[3]);
                                print_msg(MSG_INFO, "  Offset: 0x%08" PRIx64 "\n", position);
                                print_msg(MSG_INFO, "  Number: %d\n", blocks_processed);
                                print_msg(MSG_INFO, "    Size: %d\n", buf.blockSize);

                                hexdump((char *)&autopsy_buf[start], length, start);
                                print_msg(MSG_INFO, "--------------------\n");
                                print_msg(MSG_INFO, "\n");
                                break;
                            }                            

                            case AUTOPSY_DUMP_ASCII:
                            {
                                print_msg(MSG_INFO, "--- ASCII display ---\n");
                                print_msg(MSG_INFO, "   Block: %c%c%c%c\n", buf.blockType[0], buf.blockType[1], buf.blockType[2], buf.blockType[3]);
                                print_msg(MSG_INFO, "  Offset: 0x%08" PRIx64 "\n", position);
                                print_msg(MSG_INFO, "  Number: %d\n", blocks_processed);
                                print_msg(MSG_INFO, "    Size: %d\n", buf.blockSize);
                                print_msg(MSG_INFO, " Content: \"%s\"\n", &autopsy_buf[start]);
                                print_msg(MSG_INFO, "---------------------\n");
                                print_msg(MSG_INFO, "\n");
                                break;
                            }
                        }
                        
                        free(autopsy_buf);
                        
                        /* when extracting block types, keep rolling, there might be more */
                        if(autopsy_mode != AUTOPSY_EXTRACT_TYPE)
                        {
                            goto abort;
                        }
                        break;
                    }

        /* file header */
        if(!memcmp(mlv_block->blockType, "MLVI", 4))
        {
            mlv_file_hdr_t file_hdr = *(mlv_file_hdr_t *)mlv_block;
            
            handled_write = 1;

            /* disable '--no-audio' switch if MLV has no audio */
            if(!file_hdr.audioClass) no_audio = 0;

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
                print_msg(MSG_INFO, "    Class Video : 0x%08X\n", file_hdr.videoClass);
                print_msg(MSG_INFO, "    Class Audio : 0x%08X\n", file_hdr.audioClass);
            }
            
            if(alter_fps)
            {
                file_hdr.sourceFpsNom = alter_fps;
                file_hdr.sourceFpsDenom = 1000;
            }

            /* in xref mode, use every block and get its timestamp etc */
            if(xref_mode)
            {
                xref_resize(&frame_xref_table, frame_xref_entries + 1, &frame_xref_allocated);

                /* add xref data */
                frame_xref_table[frame_xref_entries].frameTime = 0;
                frame_xref_table[frame_xref_entries].frameOffset = position;
                frame_xref_table[frame_xref_entries].fileNumber = in_file_num;
                frame_xref_table[frame_xref_entries].frameType = MLV_FRAME_UNSPECIFIED;

                frame_xref_entries++;
            }

            /* is this the first file? */
            if(main_header.fileGuid == 0)
            {
                /* correct header size if needed */
                file_hdr.blockSize = sizeof(mlv_file_hdr_t);

                memcpy(&main_header, &file_hdr, sizeof(mlv_file_hdr_t));

                total_vidf_count = main_header.videoFrameCount;
                total_audf_count = main_header.audioFrameCount;

                if(mlv_output)
                {
                    if(average_mode)
                    {
                        file_hdr.videoFrameCount = 1;
                    }

                    /* set the output compression flag */
                    if(compress_output)
                    {
                        file_hdr.videoClass |= MLV_VIDEO_CLASS_FLAG_LJ92;
                        file_hdr.videoClass &= ~MLV_VIDEO_CLASS_FLAG_LZMA;
                    }
                    
                    if(decompress_input)
                    {
                        file_hdr.videoClass &= ~MLV_VIDEO_CLASS_FLAG_LJ92;
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

                    if(!extract_block || !strncasecmp(extract_block, (char*)file_hdr.fileMagic, 4))
                    {
                        if(fwrite(&file_hdr, file_hdr.blockSize, 1, out_file) != 1)
                        {
                            print_msg(MSG_ERROR, "Failed writing into .MLV file\n");
                            goto abort;
                        }
                    }
                    
                    if(inject_filename)
                    {
                        FILE *inject_file = fopen(inject_filename, "rb");
                        
                        if(!inject_file)
                        {
                            print_msg(MSG_ERROR, "Failed opening .MLV file to inject\n");
                            goto abort;
                        }
                        
                        file_set_pos(inject_file, 0, SEEK_END);
                        uint32_t size = file_get_pos(inject_file);
                        file_set_pos(inject_file, 0, SEEK_SET);
                        
                        uint8_t *buf = malloc(size);
                        if(!buf)
                        {
                            print_msg(MSG_ERROR, "Failed to allocate buffer for data to inject\n");
                            goto abort;
                        }
                        
                        if(fread(buf, size, 1, inject_file) != 1)
                        {
                            print_msg(MSG_ERROR, "Failed to read data from inject file\n");
                            goto abort;
                        }

                        if(fwrite(buf, size, 1, out_file) != 1)
                        {
                            print_msg(MSG_ERROR, "Failed writing into .MLV file\n");
                            goto abort;
                        }
                        
                        free(buf);
                        
                        fclose(inject_file);
                    }
                }
            }
            else
            {
                /* no, its another chunk */
                if(main_header.fileGuid != file_hdr.fileGuid)
                {
                    print_msg(MSG_ERROR, "Error: GUID within the file chunks mismatch!\n");
                    //break;
                }

                total_vidf_count += file_hdr.videoFrameCount;
                total_audf_count += file_hdr.audioFrameCount;
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
            if(xref_mode && memcmp(mlv_block->blockType, "NULL", 4) && memcmp(mlv_block->blockType, "BKUP", 4))
            {
                xref_resize(&frame_xref_table, frame_xref_entries + 1, &frame_xref_allocated);

                /* add xref data */
                frame_xref_table[frame_xref_entries].frameTime = mlv_block->timestamp;
                frame_xref_table[frame_xref_entries].frameOffset = position;
                frame_xref_table[frame_xref_entries].fileNumber = in_file_num;
                frame_xref_table[frame_xref_entries].frameType =
                    !memcmp(mlv_block->blockType, "VIDF", 4) ? MLV_FRAME_VIDF :
                    !memcmp(mlv_block->blockType, "AUDF", 4) ? MLV_FRAME_AUDF :
                    MLV_FRAME_UNSPECIFIED;

                frame_xref_entries++;
            }

            if(main_header.blockSize == 0)
            {
                print_msg(MSG_ERROR, "Missing file header\n");
                goto abort;
            }

            if(verbose)
            {
                print_msg(MSG_INFO, "Block: %c%c%c%c\n", mlv_block->blockType[0], mlv_block->blockType[1], mlv_block->blockType[2], mlv_block->blockType[3]);
                print_msg(MSG_INFO, "  Offset: 0x%08" PRIx64 "\n", position);
                print_msg(MSG_INFO, "  Number: %d\n", blocks_processed);
                print_msg(MSG_INFO, "    Size: %d\n", mlv_block->blockSize);

                /* NULL blocks don't have timestamps */
                if(memcmp(mlv_block->blockType, "NULL", 4)|| memcmp(mlv_block->blockType, "BKUP", 4))
                {
                    print_msg(MSG_INFO, "    Time: %f ms\n", (double)mlv_block->timestamp / 1000.0f);
                }
            }

            if(!memcmp(mlv_block->blockType, "AUDF", 4) && !no_audio)
            {
                mlv_audf_hdr_t block_hdr = *(mlv_audf_hdr_t *)mlv_block;

                lua_handle_hdr(lua_state, mlv_block->blockType, &block_hdr, sizeof(block_hdr));
                if(verbose)
                {
                    print_msg(MSG_INFO, "   Frame: #%04d\n", block_hdr.frameNumber);
                    print_msg(MSG_INFO, "   Space: %d\n", block_hdr.frameSpace);
                }
                
                if(block_hdr.frameSpace > block_hdr.blockSize - sizeof(mlv_vidf_hdr_t))
                {
                    print_msg(MSG_ERROR, "AUDF: Frame space is larger than block size. Skipping\n");
                    return PROCESS_SKIP;
                }

                int frame_size = block_hdr.blockSize - sizeof(mlv_audf_hdr_t) - block_hdr.frameSpace;
                void *payload = BYTE_OFFSET(mlv_block, sizeof(mlv_audf_hdr_t) + block_hdr.frameSpace);
                
                /* only write WAV if the WAVI header created a file */
                if(out_file_wav)
                {
                    if(!wavi_info.timestamp)
                    {
                        print_msg(MSG_ERROR, "AUDF: Received AUDF without WAVI, the .wav file might be corrupt\n");
                    }
                    
                    /* assume block size is uniform, this allows random access */
                    file_set_pos(out_file_wav, wav_header_size + frame_size * block_hdr.frameNumber, SEEK_SET);
                    
                    if(fwrite(payload, frame_size, 1, out_file_wav) != 1)
                    {
                        print_msg(MSG_ERROR, "AUDF: Failed writing into .WAV file\n");
                        return PROCESS_ERROR;
                    }
                    
                    wav_file_size += frame_size;
                }
                
                audf_frames_processed++;
            }
            else if(!memcmp(mlv_block->blockType, "VIDF", 4))
            {
                mlv_vidf_hdr_t block_hdr = *(mlv_vidf_hdr_t *)mlv_block;
                
                /* this code handles writing on its own */
                handled_write = 1;

                /* store last VIDF for reference */
                last_vidf = block_hdr;

                if(verbose)
                {
                    print_msg(MSG_INFO, "   Frame: #%04d\n", block_hdr.frameNumber);
                    print_msg(MSG_INFO, "    Crop: %dx%d\n", block_hdr.cropPosX, block_hdr.cropPosY);
                    print_msg(MSG_INFO, "     Pan: %dx%d\n", block_hdr.panPosX, block_hdr.panPosY);
                    print_msg(MSG_INFO, "   Space: %d\n", block_hdr.frameSpace);
                }
                
                if(alter_fps)
                {
                    block_hdr.timestamp = (((uint64_t)block_hdr.frameNumber * 10000000ULL) / alter_fps) * 1000;
                }
                
                uint32_t skip_block = 0;
                
                if(block_hdr.frameSpace > block_hdr.blockSize - sizeof(mlv_vidf_hdr_t))
                {
                    print_msg(MSG_ERROR, "VIDF: Frame space is larger than block size. Skipping\n");
                    skip_block = 1;
                }

                /* 
                  conditions when to process this block:
                    a) if we should output a RAW file
                    b) if we should output a MLV file
                    c) if we should output DNG files
                    d) if LUA is enabled
                    e) but not if this block should be skipped (due to inconsistent header data)
                */
                if((raw_output || mlv_output || dng_output || lua_state) && !skip_block)
                {
                    /* if already compressed, we have to decompress it first */
                    int compressed_lzma = main_header.videoClass & MLV_VIDEO_CLASS_FLAG_LZMA;
                    int compressed_lj92 = main_header.videoClass & MLV_VIDEO_CLASS_FLAG_LJ92;
                    int compressed = compressed_lzma || compressed_lj92;
                    int recompress = compressed && compress_output;
                    int decompress = compressed && decompress_input;
                    
                    /*
                      run decompression routine when
                        a) we shall re-compress the output (option -c for compressed input)
                        b) we shall de-compress the output (option -d)
                        c) we have compressed input and should write DNG or RAW
                    */
                    int run_decompressor = recompress || decompress || ((raw_output || dng_output) && compressed);
                    
                    /*
                      write this block when following conditions are true
                        a) we are writing a MLV file
                        b) this is not average mode (where video data will accumulate and be written as last)
                        c) this block should get extracted in case of extraction mode
                    */
                    int write_block = mlv_output && !average_mode && (!extract_block || !strncasecmp(extract_block, (char*)block_hdr.blockType, 4));             

                    /*
                      compress_output can be 0 or 1. run compressor if not zero.
                    */
                    int run_compressor = compress_output != 0;
                    
                    /*
                      special case:
                        when specified "-p" on commandline, pass through unmodified (compressed/uncompressed) data into DNG files.
                        this will be a lot faster, but requires the user to fix striping and stuff on its own.
                    */
                    if(dng_output && pass_through)
                    {
                        run_decompressor = 0;
                        run_compressor = 0;
                        fix_vert_stripes = 0;
                        fix_cold_pixels = 0;
                        assert(!chroma_smooth_method);
                        assert(!subtract_mode);
                        assert(!flatfield_mode);
                        assert(!average_mode);
                        assert(!bit_zap);
                        assert(!delta_encode_mode);
                        assert(!raw_output);
                    }
                    
                    /*
                      the frame_size is the size of the raw video frame.
                      for uncompressed files, the VIDF contains this amount of bytes along with some padding.
                      compressed files can contain arbitrary payload sizes.
                      the decompressed content however must match the frame_size.
                       */
                    int frame_size = ((video_xRes * video_yRes * lv_rec_footer.raw_info.bits_per_pixel + 7) / 8);

                    /* cache frame size as there are modes where the stored size does not match the frame's size (e.g. compressed frames) */
                    int read_size = frame_size;
                    
                    /* when the block is compressed, read the whole content, not just "frame_size" */
                    if(compressed)
                    {
                        /* just read everything right behind the frameSpace */
                        read_size = block_hdr.blockSize - sizeof(mlv_vidf_hdr_t) - block_hdr.frameSpace;
                    }
                    
                    /* check if there is enough memory for that frame, compressed or uncompressed or with unexpected VIDF size */
                    uint32_t new_buffer_size = MAX((uint32_t)frame_size, (uint32_t)read_size);
                    if(frame_buffer_size < new_buffer_size)
                    {
                        /* no, set new size */
                        frame_buffer_size = new_buffer_size;
                        
                        /* realloc buffers */
                        frame_buffer = realloc(frame_buffer, frame_buffer_size);
                        
                        if(!frame_buffer)
                        {
                            print_msg(MSG_ERROR, "VIDF: Failed to allocate %d byte\n", frame_buffer_size);
                            goto abort;
                        }
                    }
                    
                    /* copy data from read block */
                    void *payload = BYTE_OFFSET(mlv_block, sizeof(mlv_vidf_hdr_t) + block_hdr.frameSpace);
                    memcpy(frame_buffer, payload, read_size);

                    lua_handle_hdr_data(lua_state, mlv_block->blockType, "_data_read", &block_hdr, sizeof(block_hdr), frame_buffer, frame_buffer_size);

                    if(run_decompressor)
                    {
                        if(compressed_lj92)
                        {
#ifdef MLV_USE_LJ92
                            lj92 handle;
                            int lj92_width = 0;
                            int lj92_height = 0;
                            int lj92_bitdepth = 0;
                            int lj92_components = 0;

                            int ret = lj92_open(&handle, (uint8_t *)frame_buffer, read_size, &lj92_width, &lj92_height, &lj92_bitdepth, &lj92_components);

                            /* this is the raw data size with 16 bit words. it's just temporary */
                            size_t out_size = lj92_width * lj92_height * sizeof(uint16_t) * lj92_components;
                            
                            if(ret == LJ92_ERROR_NONE)
                            {
                                if(verbose)
                                {
                                    print_msg(MSG_INFO, "    LJ92: Decompressing\n");
                                    print_msg(MSG_INFO, "    LJ92: %dx%dx%d %d bpp (%d bytes buffer)\n", lj92_width, lj92_height, lj92_components, lj92_bitdepth, out_size);
                                }
                            }
                            else
                            {
                                print_msg(MSG_ERROR, "    LJ92: Open failed (%d)\n", ret);
                                if(relaxed)
                                {
                                    goto skip_block;
                                }
                                goto abort;
                            }
                            
                            /* do a proper size check before we continue */
                            int lj92_frame_size = ((lj92_width * lj92_height * lj92_components * lv_rec_footer.raw_info.bits_per_pixel + 7) / 8);
                            
                            if(lj92_frame_size != frame_size)
                            {
                                print_msg(MSG_ERROR, "    LJ92: decompressed image size (%d) does not match size retrieved from RAWI (%d)\n", lj92_frame_size, frame_size);
                                goto abort;
                            }
                            
                            /* we need a temporary buffer so we don't overwrite source data */
                            uint16_t *decompressed = malloc(out_size);
                            
                            ret = lj92_decode(handle, decompressed, lj92_width * lj92_height * lj92_components, 0, NULL, 0);

                            if(ret != LJ92_ERROR_NONE)
                            {
                                print_msg(MSG_ERROR, "    LJ92: Decompress failed (%d)\n", ret);
                                if(relaxed)
                                {
                                    goto skip_block;
                                }
                                goto abort;
                            }
                            
                            if(verbose)
                            {
                                print_msg(MSG_INFO, "    LJ92: "FMT_SIZE" -> "FMT_SIZE"  (%2.2f%% ratio)\n", frame_buffer_size, frame_size, ((float)frame_buffer_size * 100.0f) / (float)frame_size);
                            }
                            
                            /* repack the 16 bit words containing values with max 14 bit */
                            int orig_pitch = video_xRes * lv_rec_footer.raw_info.bits_per_pixel / 8;

                            for(int y = 0; y < video_yRes; y++)
                            {
                                uint16_t *src_line = &decompressed[y * video_xRes];
                                void *dst_line = &frame_buffer[y * orig_pitch];

                                for(int x = 0; x < video_xRes; x++)
                                {
                                    bitinsert(dst_line, x, lv_rec_footer.raw_info.bits_per_pixel, src_line[x]);
                                }
                            }
                            
                            free(decompressed);
#else
                            print_msg(MSG_INFO, "    LJ92: not compiled into this release, aborting.\n");
                            goto abort;
#endif
                        }
                        else if(compressed_lzma)
                        {
#ifdef MLV_USE_LZMA
                            size_t lzma_out_size = *(uint32_t *)frame_buffer;
                            size_t lzma_in_size = frame_buffer_size - LZMA_PROPS_SIZE - 4;
                            size_t lzma_props_size = LZMA_PROPS_SIZE;
                            unsigned char *lzma_out = malloc(lzma_out_size);

                            int ret = LzmaUncompress(
                                lzma_out, &lzma_out_size,
                                (unsigned char *)&frame_buffer[4 + LZMA_PROPS_SIZE], &lzma_in_size,
                                (unsigned char *)&frame_buffer[4], lzma_props_size
                                );
                                
                            if(lzma_out_size != frame_size)
                            {
                                print_msg(MSG_ERROR, "    LZMA: decompressed image size (%d) does not match size retrieved from RAWI (%d)\n", lzma_out_size, frame_size);
                                goto abort;
                            }
                            
                            if(ret == SZ_OK)
                            {
                                memcpy(frame_buffer, lzma_out, lzma_out_size);
                                
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
                    }

                    int old_depth = lv_rec_footer.raw_info.bits_per_pixel;
                    int new_depth = bit_depth;

                    /* this value changes in this context */
                    int current_depth = old_depth;

                    /* in subtract mode, subtract reference frame. do that before averaging */
                    if(subtract_mode)
                    {
                        if((int)subtract_frame_buffer_size != frame_size)
                        {
                            print_msg(MSG_ERROR, "Error: Frame sizes of footage and subtract frame differ (%d, %d)", frame_size, subtract_frame_buffer_size);
                            break;
                        }
                        
                        int pitch = video_xRes * current_depth / 8;

                        for(int y = 0; y < video_yRes; y++)
                        {
                            uint16_t *src_line = (uint16_t *)&frame_buffer[y * pitch];
                            uint16_t *sub_line = (uint16_t *)&frame_sub_buffer[y * pitch];

                            for(int x = 0; x < video_xRes; x++)
                            {
                                int32_t value = bitextract(src_line, x, current_depth);
                                int32_t sub_value = bitextract(sub_line, x, current_depth);

                                value -= sub_value;
                                value += lv_rec_footer.raw_info.black_level; /* should we really add it here? or better subtract it from averaged frame? */
                                value = COERCE(value, 0, (1<<current_depth)-1);

                                bitinsert(src_line, x, current_depth, value);
                            }
                        }
                    }

                    /* in flat-field mode, divide each image by the normalized reference frame */
                    if(flatfield_mode)
                    {
                        if((int)flatfield_frame_buffer_size != frame_size)
                        {
                            print_msg(MSG_ERROR, "Error: Frame sizes of footage and flat-field frame differ (%d, %d)", frame_size, flatfield_frame_buffer_size);
                            break;
                        }
                        
                        int pitch = video_xRes * current_depth / 8;

                        /* normalize flat frame on each Bayer channel (median) */
                        /* and adjust all medians using green's 5th percentile to prevent whites from clipping */
                        static int32_t med[2][2] = {{0,0},{0,0}};
                        static int32_t pr5[2][2] = {{0,0},{0,0}};
                        static int32_t adj_num = 0;
                        static int32_t adj_den = 0;

                        int black = lv_rec_footer.raw_info.black_level;
                        
                        if (!med[0][0])
                        {
                            /* normalize using frame center only
                             * (also works on lenses with heavy vignetting) */
                            
                            int* hist[2][2];
                            int total[2][2] = {{0,0},{0,0}};
                            
                            hist[0][0] = calloc(1 << current_depth, sizeof(int));
                            hist[0][1] = calloc(1 << current_depth, sizeof(int));
                            hist[1][0] = calloc(1 << current_depth, sizeof(int));
                            hist[1][1] = calloc(1 << current_depth, sizeof(int));
                            
                            for(int y = video_yRes/4; y < video_yRes*3/4; y++)
                            {
                                uint16_t *flat_line = (uint16_t *)&frame_flat_buffer[y * pitch];
                                for(int x = video_xRes/4; x < video_xRes*3/4; x++)
                                {
                                    uint32_t value = bitextract(flat_line, x, current_depth);
                                    hist[y%2][x%2][value]++;
                                    total[y%2][x%2]++;
                                }
                            }
                            
                            for (int dy = 0; dy < 2; dy++)
                            {
                                for (int dx = 0; dx < 2; dx++)
                                {
                                    int acc = 0;
                                    for (int i = 0; i < (1 << current_depth); i++)
                                    {
                                        acc += hist[dy][dx][i];
                                        
                                        if (acc < total[dy][dx]/20)
                                        {
                                            /* 5th percentile */
                                            pr5[dy][dx] = i - black;
                                        }
                                        
                                        if (acc < total[dy][dx]/2)
                                        {
                                            /* median */
                                            med[dy][dx] = i - black;
                                        }
                                    }
                                }
                            }
                            
                            free(hist[0][0]);
                            free(hist[0][1]);
                            free(hist[1][0]);
                            free(hist[1][1]);
                            
                            adj_num = (pr5[0][1] + pr5[1][0]) / 2;
                            adj_den = (med[0][1] + med[1][0]) / 2;

                            print_msg(MSG_INFO, "Flat-field median: [%d %d; %d %d], adjusted by %d/%d\n", 
                                med[0][0], med[0][1],
                                med[1][0], med[1][1],
                                adj_num, adj_den
                            );
                        }
                        
                        for(int y = 0; y < video_yRes; y++)
                        {
                            uint16_t *src_line = (uint16_t *)&frame_buffer[y * pitch];
                            uint16_t *flat_line = (uint16_t *)&frame_flat_buffer[y * pitch];

                            for(int x = 0; x < video_xRes; x++)
                            {
                                int32_t value = bitextract(src_line, x, current_depth);
                                int32_t flat_value = bitextract(flat_line, x, current_depth);
                                
                                if (flat_value - black <= 0)
                                {
                                    int left  = bitextract(flat_line, MAX(x-1,0), current_depth);
                                    int right = bitextract(flat_line, MIN(x+1,video_xRes-1), current_depth);
                                    flat_value = MAX(left, right);
                                }

                                if (flat_value - black > 0)
                                {
                                    value -= black;
                                    value = (int64_t) value * med[y%2][x%2] * adj_num / adj_den / (flat_value - black);
                                    value += black;
                                    value = COERCE(value, 0, (1<<current_depth)-1);
                                }

                                bitinsert(src_line, x, current_depth, value);
                            }
                        }
                    }
                    static int countzz = 1;
                    /* in average mode, sum up all pixel values of a pixel position */
                    if(average_mode)
                    {
                        int pitch = video_xRes * current_depth / 8;

                        for(int y = 0; y < video_yRes; y++)
                        {
                            uint16_t *src_line = (uint16_t *)&frame_buffer[y * pitch];

                            for(int x = 0; x < video_xRes; x++)
                            {
                                uint16_t value = bitextract(src_line, x, current_depth);

                                frame_arith_buffer[y * video_xRes + x] += value;

                                if(countzz)
                                {
                                    printf("#%u avg[0] = %u, unpacked[0] = %u\n", average_samples, frame_arith_buffer[0], value);
                                    countzz = 0;
                                }
                            }
                        }
                        countzz = 1;
                        average_samples++;
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

                                /* normalize the old value to 16 bits, minimizing the roundoff error */
                                /* assume the bit depth reduction simply discarded the lower bits */
                                /* => we have a bias of 0.5 LSB that can be corrected here. */
                                value <<= (16-old_depth);
                                value += (1 << (15-old_depth));

                                /* convert the old value to destination depth */
                                value >>= (16-new_depth);

                                bitinsert(dst_line, x, new_depth, value);
                            }
                        }

                        /* update uncompressed frame and buffer size */
                        frame_size = new_size;
                        frame_buffer_size = new_size;
                        current_depth = new_depth;

                        frame_buffer = realloc(frame_buffer, frame_buffer_size);
                        assert(frame_buffer);
                        memcpy(frame_buffer, new_buffer, frame_buffer_size);
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
                    uint32_t frame_selected = (!extract_frames) || ((block_hdr.frameNumber >= frame_start) && (block_hdr.frameNumber <= frame_end));

                    if(frame_selected)
                    {
                        lua_handle_hdr_data(lua_state, mlv_block->blockType, "_data_write", &block_hdr, sizeof(block_hdr), frame_buffer, frame_buffer_size);

                        if(raw_output)
                        {
                            if(!lv_rec_footer.frameSize)
                            {
                                lv_rec_footer.frameSize = frame_size;
                            }

                            lua_handle_hdr_data(lua_state, mlv_block->blockType, "_data_write_raw", &block_hdr, sizeof(block_hdr), frame_buffer, frame_size);

                            file_set_pos(out_file, (uint64_t)block_hdr.frameNumber * (uint64_t)frame_size, SEEK_SET);
                            if(fwrite(frame_buffer, frame_size, 1, out_file) != 1)
                            {
                                print_msg(MSG_ERROR, "VIDF: Failed writing into .RAW file\n");
                                goto abort;
                            }
                        }
                        
                        /* before compressing do stuff necessary for DNG output */
                        if(dng_output)
                        {
                            struct raw_info raw_info;

                            int frame_filename_len = strlen(output_filename) + 32;
                            char *frame_filename = malloc(frame_filename_len);
                            snprintf(frame_filename, frame_filename_len, "%s%06d.dng", output_filename, block_hdr.frameNumber);

                            lua_handle_hdr_data(lua_state, mlv_block->blockType, "_data_write_dng", &block_hdr, sizeof(block_hdr), frame_buffer, frame_size);

                            raw_info.api_version = lv_rec_footer.raw_info.api_version;
                            raw_info.height = lv_rec_footer.raw_info.height;
                            raw_info.width = lv_rec_footer.raw_info.width;
                            raw_info.pitch = lv_rec_footer.raw_info.pitch;
                            raw_info.bits_per_pixel = lv_rec_footer.raw_info.bits_per_pixel;
                            raw_info.black_level = lv_rec_footer.raw_info.black_level;
                            raw_info.white_level = lv_rec_footer.raw_info.white_level;
                            raw_info.jpeg.x = lv_rec_footer.raw_info.jpeg.x;
                            raw_info.jpeg.y = lv_rec_footer.raw_info.jpeg.y;
                            raw_info.jpeg.width = lv_rec_footer.raw_info.jpeg.width;
                            raw_info.jpeg.height = lv_rec_footer.raw_info.jpeg.height;
                            raw_info.exposure_bias[0] = lv_rec_footer.raw_info.exposure_bias[0];
                            raw_info.exposure_bias[1] = lv_rec_footer.raw_info.exposure_bias[1];
                            raw_info.cfa_pattern = lv_rec_footer.raw_info.cfa_pattern;
                            raw_info.calibration_illuminant1 = lv_rec_footer.raw_info.calibration_illuminant1;
                            memcpy(raw_info.color_matrix1, lv_rec_footer.raw_info.color_matrix1, sizeof(raw_info.color_matrix1));
                            memcpy(raw_info.dng_active_area, lv_rec_footer.raw_info.dng_active_area, sizeof(raw_info.dng_active_area));
                            raw_info.dynamic_range = lv_rec_footer.raw_info.dynamic_range;
                            raw_info.frame_size = frame_size;
                            raw_info.buffer = frame_buffer;
                            
                            /* patch raw info if black and/or white fix specified or bit depth changed */
                            fix_black_white_level(&raw_info.black_level, &raw_info.white_level, &raw_info.bits_per_pixel, bit_depth, black_fix, white_fix, verbose);
                            
                            /* raw state: 
                               0 - uncompressed/decompressed
                               1 - compressed/recompressed by "-c"
                               2 - original uncompressed
                               3 - original lossless
                            */
                            enum raw_state raw_state = (pass_through << 1) | (compressed & pass_through) | compress_output;

                            /************ Initialize frame_info struct ************/
                            frame_info.mlv_filename         = input_filename;
                            frame_info.fps_override         = alter_fps;
                            frame_info.deflicker_target     = deflicker_target;
                            frame_info.vertical_stripes     = fix_vert_stripes;
                            frame_info.focus_pixels         = fix_focus_pixels;
                            frame_info.bad_pixels           = fix_cold_pixels;
                            frame_info.dual_iso             = is_dual_iso;
                            frame_info.save_bpm             = save_bpm_file;
                            frame_info.chroma_smooth        = chroma_smooth_method;
                            frame_info.pattern_noise        = fix_pattern_noise;
                            frame_info.show_progress        = show_progress;
                            frame_info.raw_state            = raw_state;
                            frame_info.pack_bits            = pack_dng_bits;
                            frame_info.fpi_method           = fpi_method;
                            frame_info.bpi_method           = bpi_method;
                            frame_info.crop_rec             = crop_rec;

                            frame_info.file_hdr             = main_header;
                            frame_info.vidf_hdr             = last_vidf;
                            frame_info.rtci_hdr             = rtci_info;
                            frame_info.idnt_hdr             = idnt_info;
                            frame_info.expo_hdr             = expo_info;
                            frame_info.lens_hdr             = lens_info;
                            frame_info.wbal_hdr             = wbal_info;
                            frame_info.rawc_hdr             = rawc_info;
                            frame_info.info_str             = info_string;
                            frame_info.rawi_hdr.xRes        = lv_rec_footer.xRes;
                            frame_info.rawi_hdr.yRes        = lv_rec_footer.yRes;
                            frame_info.rawi_hdr.raw_info    = raw_info;
                            /******************************************************/

                            /* init and process 'dng_data' raw buffers or use original compressed/uncompressed ones */
                            switch(raw_state)
                            {
                                /* if raw input lossless/uncompressed should stay 
                                   uncompressed or is going to be compressed/recompressed */
                                case UNCOMPRESSED_RAW:
                                case COMPRESSED_RAW:
                                    dng_init_data(&frame_info, &dng_data);
                                    dng_process_data(&frame_info, &dng_data);
                                    break;
                                /* if passing through original uncompressed/lossless raw */
                                case UNCOMPRESSED_ORIG:
                                case COMPRESSED_ORIG:
                                    dng_data.image_buf = (uint16_t *)frame_buffer;
                                    dng_data.image_size = frame_buffer_size;
                                    break;
                            }
                        }

                        if(run_compressor)
                        {
#ifdef MLV_USE_LJ92
                            uint8_t *compressed = NULL;
                            int compressed_size = 0;
                            
                            int compress_buffer_size = 0;
                            uint16_t *compress_buffer = NULL;

                            /* if DNG output then point ready to compress buffer pointer to dng_data.image_buf and set correct size */
                            if(dng_output)
                            {
                                compress_buffer_size = dng_data.image_size;
                                compress_buffer = dng_data.image_buf;
                            }
                            else // other case(s), MLV output, etc
                            {
                                compress_buffer_size = video_xRes * video_yRes * sizeof(uint16_t);
                                compress_buffer = malloc(compress_buffer_size);
                                
                                /* repack the 16 bit words containing values with max 14 bit */
                                int orig_pitch = video_xRes * lv_rec_footer.raw_info.bits_per_pixel / 8;

                                for(int y = 0; y < video_yRes; y++)
                                {
                                    void *src_line = &frame_buffer[y * orig_pitch];
                                    uint16_t *dst_line = &compress_buffer[y * video_xRes];

                                    for(int x = 0; x < video_xRes; x++)
                                    {
                                        dst_line[x] = bitextract(src_line, x, lv_rec_footer.raw_info.bits_per_pixel);
                                    }
                                }
                            }
                            
                            /* trying to compress with the same properties as the camera compresses, but..
                               a small problem here, the compress part of the lj92 lib currently used seems not to allow multiple components.
                               i.e. a line of RGRGRGRG... would be one component and the next line of GBGBGBGB the second.
                               unfortunately the image would have to get compressed a bit less efficient as a single-component image.
                               
                               a1ex had a good workaround that implies setting xres*2 and yres/2 which would help the compressor.
                               
                               canon compression:
                                 1872x624x2 14 bpp
                                 2348480 -> 4088448  (57.44% ratio)
                                 
                               single component compression: 
                                 1872x1248x1 14 bpp
                                 3515085 -> 4088448  (85.98% ratio)

                               single component "x2" compression:
                                 3744x624x1 14 bpp 
                                 2353223 -> 4088448  (57.56% ratio)

                               so this method gets quite close to canon's in-camera compression.
                               while the resolution outputs differ, the real image data is the same.
                               
                               downside: we cannot double check if the lj92-reported resolution matches the RAWI information.
                            */
                            int lj92_components = 1;
                            int lj92_width = video_xRes * 2;
                            int lj92_height = video_yRes / 2;
                            int lj92_bitdepth = old_depth;
                            
                            if(verbose)
                            {
                                print_msg(MSG_INFO, "    LJ92: Compressing\n");
                                print_msg(MSG_INFO, "    LJ92: %dx%dx%d %d bpp (%d bytes buffer)\n", lj92_width, lj92_height, lj92_components, lj92_bitdepth, compress_buffer_size);
                            }
                            
                            int ret = lj92_encode(compress_buffer, lj92_width, lj92_height, lj92_bitdepth, 2, lj92_width * lj92_height, 0, NULL, 0, &compressed, &compressed_size);

                            if(ret == LJ92_ERROR_NONE)
                            {
                                if(verbose)
                                {
                                    print_msg(MSG_INFO, "    LJ92: "FMT_SIZE" -> "FMT_SIZE"  (%2.2f%% ratio)\n", frame_size, compressed_size, ((float)compressed_size * 100.0f) / (float)frame_size);
                                }
                                
                                /* set new compressed size and copy buffers */
                                frame_buffer = realloc(frame_buffer, compressed_size);
                                assert(frame_buffer);
                                memcpy(frame_buffer, compressed, compressed_size);
                                frame_buffer_size = compressed_size;

                                /* if DNG output then point dng_data.image_buf to already compressed buffer and set correct size */
                                if(dng_output)
                                {
                                    dng_data.image_buf = (uint16_t *)frame_buffer;
                                    dng_data.image_size = frame_buffer_size;
                                }
                                else // other case(s), MLV output, etc
                                {
                                    free(compress_buffer);
                                }
                            }
                            else
                            {
                                print_msg(MSG_ERROR, "    LJ92: Failed (%d)\n", ret);
                                goto abort;
                            }
                            
                            free(compressed);
#else
                            print_msg(MSG_INFO, "    no compression type compiled into this release, aborting.\n");
                            if(relaxed)
                            {
                                goto skip_block;
                            }
                            goto abort;
#endif
                            if(frame_buffer_size != (uint32_t)frame_size && !verbose && show_progress)
                            {
                                static int first_time = 1;
                                if(first_time)
                                {
                                    print_msg(MSG_INFO, "\nWriting LJ92 compressed frames...\n");
                                    first_time = 0;
                                }
                                print_msg(MSG_INFO, "  saving: "FMT_SIZE" -> "FMT_SIZE"  (%2.2f%% ratio)\n", frame_size, frame_buffer_size, ((float)frame_buffer_size * 100.0f) / (float)frame_size);
                            }
                        }

                        /* save DNG frame */
                        if(dng_output)
                        {
                            int frame_filename_len = strlen(output_filename) + 32;
                            char *frame_filename = malloc(frame_filename_len);
                            snprintf(frame_filename, frame_filename_len, "%s%06d.dng", output_filename, block_hdr.frameNumber);
                            frame_info.dng_filename = frame_filename;

                            lua_handle_hdr_data(lua_state, mlv_block->blockType, "_data_write_dng", &block_hdr, sizeof(block_hdr), frame_buffer, frame_buffer_size);

                            /* set MLV metadata into DNG tags */
                            dng_set_framerate_rational(main_header.sourceFpsNom, main_header.sourceFpsDenom);
                            dng_set_shutter(expo_info.shutterValue, 1000000);
                            dng_set_aperture(lens_info.aperture, 100);
                            dng_set_camname((char*)unique_camname);
                            dng_set_description((char*)info_string);
                            dng_set_lensmodel((char*)lens_info.lensName);
                            dng_set_focal(lens_info.focalLength, 1);
                            dng_set_iso(expo_info.isoValue);

                            //dng_set_wbgain(1024, wbal_info.wbgain_r, 1024, wbal_info.wbgain_g, 1024, wbal_info.wbgain_b);

                            /* calculate the time this frame was taken at, i.e., the start time + the current timestamp. this can be off by a second but it's better than nothing */
                            int ms = 0.5 + mlv_block->timestamp / 1000.0;
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
                                print_msg(MSG_ERROR, "VIDF: Failed writing into .DNG file\n");
                                if(relaxed)
                                {
                                    goto skip_block;
                                }
                                goto abort;
                            }

                            /* callout for a saved dng file */
                            lua_call_va(lua_state, "dng_saved", "si", frame_filename, block_hdr.frameNumber);

                            free(frame_filename);
                        }

                        if(write_block)
                        {
                            lua_handle_hdr_data(lua_state, mlv_block->blockType, "_data_write_mlv", &block_hdr, sizeof(block_hdr), frame_buffer, frame_buffer_size);

                            /* delete free space and correct header size if needed */
                            block_hdr.blockSize = sizeof(mlv_vidf_hdr_t) + frame_buffer_size;
                            block_hdr.frameSpace = 0;
                            block_hdr.frameNumber -= frame_start;

                            if(fwrite(&block_hdr, sizeof(mlv_vidf_hdr_t), 1, out_file) != 1)
                            {
                                print_msg(MSG_ERROR, "VIDF: Failed writing into .MLV file\n");
                                goto abort;
                            }
                            if(fwrite(frame_buffer, frame_buffer_size, 1, out_file) != 1)
                            {
                                print_msg(MSG_ERROR, "VIDF: Failed writing into .MLV file\n");
                                goto abort;
                            }
                        }
                    }
                }
                
                vidf_max_number = MAX(vidf_max_number, block_hdr.frameNumber);

                vidf_frames_processed++;
            }
            else if(!memcmp(mlv_block->blockType, "LENS", 4))
            {
                lens_info = *(mlv_lens_hdr_t *)mlv_block;

                if(verbose)
                {
                    uint64_t serial = 0;
                    char serial_str[64];
                    char *end;
                    
                    strcpy(serial_str, "no valid S/N");
                    serial = strtoull((char *)lens_info.lensSerial, &end, 16);
                    if (serial && !*end)
                    {
                        sprintf(serial_str, "%"PRIu64, serial);
                    }
                    
                    print_msg(MSG_INFO, "     Name:        '%s'\n", lens_info.lensName);
                    print_msg(MSG_INFO, "     Serial:      '%s' (%s)\n", lens_info.lensSerial, serial_str);
                    print_msg(MSG_INFO, "     Focal Len:   %d mm\n", lens_info.focalLength);
                    print_msg(MSG_INFO, "     Focus Dist:  %d mm\n", lens_info.focalDist);
                    print_msg(MSG_INFO, "     Aperture:    f/%.2f\n", (double)lens_info.aperture / 100.0f);
                    print_msg(MSG_INFO, "     IS Mode:     %d\n", lens_info.stabilizerMode);
                    print_msg(MSG_INFO, "     AF Mode:     %d\n", lens_info.autofocusMode);
                    print_msg(MSG_INFO, "     Lens ID:     0x%08X\n", lens_info.lensID);
                    print_msg(MSG_INFO, "     Flags:       0x%08X\n", lens_info.flags);
                }
            }
            else if(!memcmp(mlv_block->blockType, "INFO", 4))
            {
                mlv_info_hdr_t block_hdr = *(mlv_info_hdr_t *)mlv_block;

                /* get the string length and malloc a buffer for that string */
                int str_length = MIN(block_hdr.blockSize - sizeof(block_hdr), sizeof(info_string) - 1);

                if(str_length)
                {
                    void *payload = BYTE_OFFSET(mlv_block, sizeof(mlv_info_hdr_t));

                    strncpy(info_string, payload, MIN((size_t)str_length, sizeof(info_string)));
                    info_string[str_length] = '\000';

                    if(verbose)
                    {
                        print_msg(MSG_INFO, "     String:   '%s'\n", info_string);
                    }
                }
            }
            else if(!memcmp(mlv_block->blockType, "DEBG", 4))
            {
                mlv_debg_hdr_t block_hdr = *(mlv_debg_hdr_t *)mlv_block;

                /* get the string length and malloc a buffer for that string */
                int str_length = MIN(block_hdr.length, block_hdr.blockSize - sizeof(block_hdr));

                if(str_length)
                {
                    void *payload = BYTE_OFFSET(mlv_block, sizeof(mlv_debg_hdr_t));
                    char *buf = malloc(str_length + 1);
            
                    strncpy(buf, payload, str_length);
                    buf[str_length] = '\000';
            
                    if(verbose)
                    {
                        print_msg(MSG_INFO, "     String:   '%s'\n", buf);
                    }
                    
                    char *log_filename = malloc(strlen(input_filename) + 6);
                    snprintf(log_filename, strlen(input_filename) + 6, "%s.log", input_filename);
                    
                    FILE *log_file = fopen(log_filename, "ab+");
                    fwrite(buf, block_hdr.length, 1, log_file);
                    fclose(log_file);
                    free(log_filename);
            
                    free(buf);
                }
            }
            else if(!memcmp(mlv_block->blockType, "VERS", 4))
            {
                mlv_vers_hdr_t block_hdr = *(mlv_vers_hdr_t *)mlv_block;

                /* get the string length and malloc a buffer for that string */
                int str_length = MIN(block_hdr.length, block_hdr.blockSize - sizeof(block_hdr));

                if(str_length)
                {
                    void *payload = BYTE_OFFSET(mlv_block, sizeof(mlv_vers_hdr_t));
                    char *buf = malloc(str_length + 1);
            
                    strncpy(buf, payload, str_length);
                    buf[str_length] = '\000';
                    
                    if(verbose)
                    {
                        print_msg(MSG_INFO, "  String: '%s'\n", buf);
                    }
                    free(buf);
                }
            }
            else if(!memcmp(mlv_block->blockType, "ELVL", 4))
            {
                mlv_elvl_hdr_t block_hdr = *(mlv_elvl_hdr_t *)mlv_block;

                if(verbose)
                {
                    print_msg(MSG_INFO, "     Roll:    %2.2f\n", (double)block_hdr.roll / 100.0f);
                    print_msg(MSG_INFO, "     Pitch:   %2.2f\n", (double)block_hdr.pitch / 100.0f);
                }
            }
            else if(!memcmp(mlv_block->blockType, "STYL", 4))
            {
                mlv_styl_hdr_t block_hdr = *(mlv_styl_hdr_t *)mlv_block;

                if(verbose)
                {
                    print_msg(MSG_INFO, "     picStyle:   %d\n", block_hdr.picStyleId);
                    print_msg(MSG_INFO, "     contrast:   %d\n", block_hdr.contrast);
                    print_msg(MSG_INFO, "     sharpness:  %d\n", block_hdr.sharpness);
                    print_msg(MSG_INFO, "     saturation: %d\n", block_hdr.saturation);
                    print_msg(MSG_INFO, "     colortone:  %d\n", block_hdr.colortone);
                }
            }
            else if(!memcmp(mlv_block->blockType, "WBAL", 4))
            {
                wbal_info = *(mlv_wbal_hdr_t *)mlv_block;

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
            }
            else if(!memcmp(mlv_block->blockType, "IDNT", 4))
            {
                idnt_info = *(mlv_idnt_hdr_t *)mlv_block;

                uint64_t serial = 0;
                char serial_str[64];
                char *end;
                
                strcpy(serial_str, "no valid S/N");
                serial = strtoull((char *)idnt_info.cameraSerial, &end, 16);
                if (serial && !*end)
                {
                    sprintf(serial_str, "%"PRIu64, serial);
                }
                
                if(verbose)
                {
                    print_msg(MSG_INFO, "     Camera Name:   '%s'\n", idnt_info.cameraName);
                    print_msg(MSG_INFO, "     Camera Serial: '%s' (%s)\n", idnt_info.cameraSerial, serial_str);
                    print_msg(MSG_INFO, "     Camera Model:  0x%08X\n", idnt_info.cameraModel);
                }
            }
            else if(!memcmp(mlv_block->blockType, "RTCI", 4))
            {
                rtci_info = *(mlv_rtci_hdr_t *)mlv_block;

                if(verbose)
                {
                    print_msg(MSG_INFO, "     Date:        %02d.%02d.%04d\n", rtci_info.tm_mday, rtci_info.tm_mon + 1, 1900 + rtci_info.tm_year);
                    print_msg(MSG_INFO, "     Time:        %02d:%02d:%02d (GMT+%d)\n", rtci_info.tm_hour, rtci_info.tm_min, rtci_info.tm_sec, rtci_info.tm_gmtoff);
                    print_msg(MSG_INFO, "     Zone:        '%s'\n", rtci_info.tm_zone);
                    print_msg(MSG_INFO, "     Day of week: %d\n", rtci_info.tm_wday);
                    print_msg(MSG_INFO, "     Day of year: %d\n", rtci_info.tm_yday);
                    print_msg(MSG_INFO, "     Daylight s.: %d\n", rtci_info.tm_isdst);
                }
            }
            else if(!memcmp(mlv_block->blockType, "MARK", 4))
            {
                mlv_mark_hdr_t block_hdr = *(mlv_mark_hdr_t *)mlv_block;

                if(verbose)
                {
                    print_msg(MSG_INFO, "  Button: 0x%02X\n", block_hdr.type);
                }
            }
            else if(!memcmp(mlv_block->blockType, "EXPO", 4))
            {
                expo_info = *(mlv_expo_hdr_t *)mlv_block;

                if(verbose)
                {
                    print_msg(MSG_INFO, "     ISO Mode:   %d\n", expo_info.isoMode);
                    print_msg(MSG_INFO, "     ISO:        %d\n", expo_info.isoValue);
                    print_msg(MSG_INFO, "     ISO Analog: %d\n", expo_info.isoAnalog);
                    print_msg(MSG_INFO, "     ISO DGain:  %d/1024 EV\n", expo_info.digitalGain);
                    print_msg(MSG_INFO, "     Shutter:    %" PRIu64 " microseconds (1/%.2f)\n", expo_info.shutterValue, 1000000.0f/(float)expo_info.shutterValue);
                }
            }
            else if(!memcmp(mlv_block->blockType, "RAWI", 4))
            {
                mlv_rawi_hdr_t block_hdr = *(mlv_rawi_hdr_t *)mlv_block;
                
                /* 
                    commented out b/c some postprocessing tools depend on original MLV raw_info.height
                    to work properly and rounded/unreal values printed out by mlv_dump are misleading
                */
                /* well, it appears to happen that MLVs with odd sizes were written, restrict that */
                //block_hdr.raw_info.height &= ~1;
                //block_hdr.yRes &= ~1;
                
                handled_write = 1;

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
                strncpy((char*)lv_rec_footer.magic, "RAWM", 4);
                lv_rec_footer.xRes = block_hdr.xRes;
                lv_rec_footer.yRes = block_hdr.yRes;
                lv_rec_footer.raw_info = block_hdr.raw_info;

                int frame_size = MAX(bit_depth, block_hdr.raw_info.bits_per_pixel) * block_hdr.raw_info.height * block_hdr.raw_info.width / 8;
                
                /* resolution change, old data will be thrown away */
                if(frame_arith_buffer)
                {
                    print_msg(MSG_INFO, "Got a new RAWI, throwing away average buffers etc.\n");
                    free(frame_arith_buffer);
                    frame_arith_buffer = NULL;
                }
                
                if(prev_frame_buffer)
                {
                    print_msg(MSG_INFO, "Got a new RAWI, throwing away previous frame buffers etc.\n");
                    free(prev_frame_buffer);
                    prev_frame_buffer = NULL;
                }
                
                frame_arith_buffer = malloc(frame_size * sizeof(uint32_t));
                if(!frame_arith_buffer)
                {
                    print_msg(MSG_ERROR, "Failed to allocate %d byte for frame_arith_buffer\n", frame_size * sizeof(uint32_t));
                    goto abort;
                }
                
                prev_frame_buffer = malloc(frame_size);
                if(!prev_frame_buffer)
                {
                    print_msg(MSG_ERROR, "Failed to allocate %d byte for prev_frame_buffer\n", frame_size);
                    goto abort;
                }
                
                memset(frame_arith_buffer, 0x00, frame_size * sizeof(uint32_t));
                memset(prev_frame_buffer, 0x00, frame_size);

                /* always output RAWI blocks, its not just metadata, but important frame format data */
                if(mlv_output && (!extract_block || !strncasecmp(extract_block, (char*)&block_hdr.blockType, 4)))
                {
                    /* correct header size if needed */
                    block_hdr.blockSize = sizeof(mlv_rawi_hdr_t);
                    block_hdr.raw_info.bits_per_pixel = lv_rec_footer.raw_info.bits_per_pixel;

                    /* patch raw info if black and/or white fix specified or bit depth changed */
                    fix_black_white_level(&block_hdr.raw_info.black_level, &block_hdr.raw_info.white_level, &block_hdr.raw_info.bits_per_pixel, bit_depth, black_fix, white_fix, verbose);

                    if(fwrite(&block_hdr, block_hdr.blockSize, 1, out_file) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into .MLV file\n");
                        goto abort;
                    }
                }
            }
            else if(!memcmp(mlv_block->blockType, "RAWC", 4))
            {
                mlv_rawc_hdr_t block_hdr = *(mlv_rawc_hdr_t *)mlv_block;

                if(verbose)
                {
                    print_capture_info(&block_hdr);
                }
            }            
            else if(!memcmp(mlv_block->blockType, "WAVI", 4) && !no_audio)
            {
                mlv_wavi_hdr_t block_hdr = *(mlv_wavi_hdr_t *)mlv_block;

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

                if(output_filename && out_file_wav == NULL && !extract_block)
                {
                    size_t name_len = strlen(output_filename) + 5;  // + .wav\0
                    char* wav_file_name = malloc(name_len);
                    strncpy(wav_file_name, output_filename, name_len);
                    char *uline = strrchr(wav_file_name, '_');
                    if(uline)
                    {
                        *uline = '\000';
                    }
                    strcat(wav_file_name, ".wav");
                    out_file_wav = fopen(wav_file_name, "wb");
                    free(wav_file_name);
                    
                    if(!out_file_wav)
                    {
                        print_msg(MSG_ERROR, "Failed writing into audio output file\n");
                        goto abort;
                    }

                    /************************* Initialize wav_header struct *************************/
                    struct wav_header wav_hdr =
                    {
                        .RIFF = "RIFF",
                        .file_size = 0x504D4554, // for now it's "TEMP", should be (uint32_t)(wav_data_size + wav_header_size - 8), gonna be patched later
                        .WAVE = "WAVE",
                        .bext_id = "bext",
                        .bext_size = sizeof(struct wav_bext),
                        .bext.time_reference = 0, // (uint64_t)(rtci_info->tm_hour * 3600 + rtci_info->tm_min * 60 + rtci_info->tm_sec) * (uint64_t)wavi_info->samplingRate,
                        .iXML_id = "iXML",
                        .iXML_size = 1024,
                        .fmt = "fmt\x20",
                        .subchunk1_size = 16,
                        .audio_format = 1,
                        .num_channels = wavi_info.channels,
                        .sample_rate = wavi_info.samplingRate,
                        .byte_rate = wavi_info.bytesPerSecond,
                        .block_align = 4,
                        .bits_per_sample = wavi_info.bitsPerSample,
                        .data = "data",
                        .subchunk2_size = 0x504D4554, // for now it's "TEMP", should be (uint32_t)(wav_data_size), gonna be patched later
                    };

                    char temp[64];
                    snprintf(temp, sizeof(temp), "%s", idnt_info.cameraName);
                    memcpy(wav_hdr.bext.originator, temp, 32);
                    snprintf(temp, sizeof(temp), "JPCAN%04d%.8s%02d%02d%02d%09d", idnt_info.cameraModel, idnt_info.cameraSerial , rtci_info.tm_hour, rtci_info.tm_min, rtci_info.tm_sec, rand());
                    memcpy(wav_hdr.bext.originator_reference, temp, 32);
                    snprintf(temp, sizeof(temp), "%04d:%02d:%02d", 1900 + rtci_info.tm_year, rtci_info.tm_mon, rtci_info.tm_mday);
                    memcpy(wav_hdr.bext.origination_date, temp, 10);
                    snprintf(temp, sizeof(temp), "%02d:%02d:%02d", rtci_info.tm_hour, rtci_info.tm_min, rtci_info.tm_sec);
                    memcpy(wav_hdr.bext.origination_time, temp, 8);
                    
                    char * project = "Magic Lantern";
                    char * notes = "";
                    char * keywords = "";
                    int tape = 1, scene = 1, shot = 1, take = 1;
                    int fps_denom = main_header.sourceFpsDenom;
                    int fps_nom = main_header.sourceFpsNom;
                    snprintf(wav_hdr.iXML, wav_hdr.iXML_size, iXML, project, notes, keywords, tape, scene, shot, take, fps_nom, fps_denom, fps_nom, fps_denom, fps_nom, fps_denom);
                    /********************************************************************************/

                    /* write WAV header */
                    if((fwrite(&wav_hdr, sizeof(struct wav_header), 1, out_file_wav)) != 1)
                    {
                        print_msg(MSG_ERROR, "Failed writing into .WAV file\n");
                        goto abort;
                    }

                    /* init WAV data size, will be grow later block by block (AUDF) */
                    wav_data_size = 0;
                    wav_header_size = file_get_pos(out_file_wav); /* same as sizeof(struct wav_header) */
                }
            }
            else if(!memcmp(mlv_block->blockType, "DISO", 4))
            {
                diso_info = *(mlv_diso_hdr_t *)mlv_block;

                if(verbose)
                {
                    print_msg(MSG_INFO, "     Mode:        %d\n", diso_info.dualMode);
                    print_msg(MSG_INFO, "     ISO Value:   %d\n", diso_info.isoValue);
                }
            }
            else if(!memcmp(mlv_block->blockType, "NULL", 4))
            {
                /* those are just placeholders. ignore them. */
                if(extract_frames || no_audio) goto skip_block;
            }
            else if(!memcmp(mlv_block->blockType, "BKUP", 4))
            {
                /* once they were used to backup headers during frame processing in mlv_rec and could have appeared in a file. no need anymore. */
                if(extract_frames || no_audio) goto skip_block;
            }
            else
            {
                if(no_audio)
                {
                    if(!show_progress)
                    { 
                        print_msg(MSG_INFO, "Skipping '%c%c%c%c' block\n", mlv_block->blockType[0], mlv_block->blockType[1], mlv_block->blockType[2], mlv_block->blockType[3]);
                    }
                    goto skip_block;
                }
                else
                {
                    print_msg(MSG_INFO, "Unknown Block: %c%c%c%c, skipping\n", mlv_block->blockType[0], mlv_block->blockType[1], mlv_block->blockType[2], mlv_block->blockType[3]);
                }

                lua_handle_hdr(lua_state, mlv_block->blockType, "", 0);
            }
        }

       
        /* write to output file if requested */
        if(
            mlv_output &&              /* only write blocks in mlv output mode */
            !handled_write &&
            (!extract_block || !strncasecmp(extract_block, (char *)mlv_block->blockType, 4)) /* when block extraction was requested, only write those */
            )
        {
            if(fwrite(mlv_block, mlv_block->blockSize, 1, out_file) != 1)
            { 
                print_msg(MSG_ERROR, "Failed writing into .MLV file\n");
                goto abort;
            }
        }

skip_block:
        file_set_pos(in_file, position + mlv_block->blockSize, SEEK_SET);
            
        /* count any read block, no matter if header or video frame */
        blocks_processed++;


        if(block_xref)
        {
            block_xref_pos++;
            if(block_xref_pos >= block_xref->entryCount)
            {
                print_msg(MSG_INFO, "\n");
                print_msg(MSG_INFO, "Reached end of all files after %i blocks\n", blocks_processed);
                break;
            }
        }
    }
    while(!feof(in_file));

abort:

    /* free block buffer */
    if(mlv_block)
    {
        free(mlv_block);
        mlv_block = NULL;
    }

    {
        float fps = main_header.sourceFpsNom / (float)main_header.sourceFpsDenom;

        print_msg(MSG_INFO, "Processed %d video frames at %2.2f FPS (%2.2f s)\n", vidf_frames_processed, fps, vidf_frames_processed / fps);
    }
    
    /* in average mode, finalize average calculation and output the resulting average */
    if(average_mode)
    {
        if(!out_file)
        {
            print_msg(MSG_ERROR, "Averaged image, but no out file specified\n");
        }
        else if(!average_samples)
        {
            print_msg(MSG_ERROR, "Number of averaged frames is zero. Cannot continue.\n");
        }
        else
        {
            int old_depth = lv_rec_footer.raw_info.bits_per_pixel;
            int new_depth = bit_depth ? bit_depth : old_depth;
            int new_pitch = video_xRes * new_depth / 8;
            
            print_msg(MSG_INFO, "Writing averaged frame with %dbpp\n", new_depth);
            
            /* average the pixels in vertical direction, so we will extract vertical banding noise */
            if(average_vert)
            {
                for(int x = 0; x < video_xRes; x++)
                {
                    uint64_t column = 0;
                    
                    for(int y = 0; y < video_yRes; y++)
                    {
                        column += frame_arith_buffer[y * video_xRes + x];
                    }
                    column /= video_yRes;
                    for(int y = 0; y < video_yRes; y++)
                    {
                        frame_arith_buffer[y * video_xRes + x] = column;
                    }
                }
            }
            if(average_hor)
            {
                for(int y = 0; y < video_yRes; y++)
                {
                    uint64_t line = 0;
                    
                    for(int x = 0; x < video_xRes; x++)
                    {
                        line += frame_arith_buffer[y * video_xRes + x];
                    }
                    line /= video_yRes;
                    for(int x = 0; x < video_xRes; x++)
                    {

                        frame_arith_buffer[y * video_xRes + x] = line;
                    }
                }
            }
            
            for(int y = 0; y < video_yRes; y++)
            {
                uint16_t *dst_line = (uint16_t *)&frame_buffer[y * new_pitch];
                for(int x = 0; x < video_xRes; x++)
                {
                    uint32_t value = frame_arith_buffer[y * video_xRes + x];
                    
                    /* complete the averaging, minimizing the roundoff error */
                    value = (value + average_samples/2) / average_samples;
                    static int countz = 1;
                    if(countz)
                    {
                        printf("avg[0] = %u samples = %u\n", value, average_samples);
                        countz = 0;
                    }
                    
                    /* scale value when bit depth changed according to depth conversion in VIDF block */
                    if(old_depth != new_depth)
                    {
                        value <<= (16-old_depth);
                        value += (1 << (15-old_depth));
                        value >>= (16-new_depth);
                    }

                    bitinsert(dst_line, x, new_depth, value);
                }
            }
            

            int frame_size = ((video_xRes * video_yRes * new_depth + 7) / 8);

            mlv_vidf_hdr_t hdr;

            memset(&hdr, 0x00, sizeof(mlv_vidf_hdr_t));
            memcpy(hdr.blockType, "VIDF", 4);
            hdr.blockSize = sizeof(mlv_vidf_hdr_t) + frame_size;
            hdr.frameNumber = 0;
            hdr.timestamp = last_vidf.timestamp;

            if(fwrite(&hdr, sizeof(mlv_vidf_hdr_t), 1, out_file) != 1)
            {
                print_msg(MSG_ERROR, "Failed writing average frame header into .MLV file\n");
            }
            if(fwrite(frame_buffer, frame_size, 1, out_file) != 1)
            {
                print_msg(MSG_ERROR, "Failed writing average frame data into .MLV file\n");
            }
        }
    }

    if(raw_output)
    {
        lv_rec_footer.frameCount = vidf_max_number + 1;
        lv_rec_footer.raw_info.bits_per_pixel = 14;

        file_set_pos(out_file, 0, SEEK_END);
        if(fwrite(&lv_rec_footer, sizeof(lv_rec_file_footer_t), 1, out_file) != 1)
        {
            print_msg(MSG_ERROR, "Failed writing into .RAW file\n");
        }
    }

    if(xref_mode && !autopsy_mode && !visualize)
    {
        print_msg(MSG_INFO, "XREF table contains %d entries\n", frame_xref_entries);
        xref_sort(frame_xref_table, frame_xref_entries);
        save_index(input_filename, &main_header, in_file_count, frame_xref_table, frame_xref_entries);
    }

    /* fix frame count */
    if(mlv_output && !extract_block)
    {
        /* get extension and set fileNum in header to zero if its a .MLV */
        char *dot = strrchr(output_filename, '.');
        if(dot)
        {
            dot++;
            if(!strcasecmp(dot, "mlv"))
            {
                main_header.fileNum = 0;
                main_header.fileCount = 1;
            }
        }
        
        main_header.videoFrameCount = vidf_frames_processed;
        main_header.audioFrameCount = audf_frames_processed;

        if(compress_output)
        {
            main_header.videoClass |= MLV_VIDEO_CLASS_FLAG_LJ92;
            main_header.videoClass &= ~MLV_VIDEO_CLASS_FLAG_LZMA;
        }
        if(decompress_input)
        {
            main_header.videoClass &= ~MLV_VIDEO_CLASS_FLAG_LJ92;
            main_header.videoClass &= ~MLV_VIDEO_CLASS_FLAG_LZMA;
        }

        if(delta_encode_mode)
        {
            main_header.videoClass |= MLV_VIDEO_CLASS_FLAG_DELTA;
        }
        else
        {
            main_header.videoClass &= ~MLV_VIDEO_CLASS_FLAG_DELTA;
        }
        
        if(no_audio)
        {
            main_header.audioClass = 0x0;
        }

        fseek(out_file, 0L, SEEK_SET);
        
        if(fwrite(&main_header, main_header.blockSize, 1, out_file) != 1)
        {
            print_msg(MSG_ERROR, "Failed to rewrite header in .MLV file\n");
        }
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
        uint32_t tmp_uint32 = wav_data_size + wav_header_size - 8; /* minus 8 = RIFF + (file size field 4 bytes) */
        file_set_pos(out_file_wav, 4, SEEK_SET);
        if(fwrite(&tmp_uint32, 4, 1, out_file_wav) != 1)
        {
            print_msg(MSG_ERROR, "Failed writing into .WAV file\n");
        }

        tmp_uint32 = wav_data_size; /* data size */
        file_set_pos(out_file_wav, 1686, SEEK_SET);
        if(fwrite(&tmp_uint32, 4, 1, out_file_wav) != 1)
        {
            print_msg(MSG_ERROR, "Failed writing into .WAV file\n");
        }
        fclose(out_file_wav);
    }

    if(dng_output)
    {
        dng_free_data(&dng_data);
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

