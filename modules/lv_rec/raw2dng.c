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

#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "string.h"
#include "strings.h"
#include "math.h"
#include "lv_rec.h"
#include <raw.h>
#include <chdk-dng.h>
#include "qsort.h"  /* much faster than standard C qsort */
#include "../dual_iso/optmed.h"
#include "../dual_iso/wirth.h"
#include "../mlv_rec/mlv.h"


/* useful to clean pink dots, may also help with color aliasing, but it's best turned off if you don't have these problems */
//~ #define CHROMA_SMOOTH

lv_rec_file_footer_t lv_rec_footer;
struct raw_info raw_info;

#define FAIL(fmt,...) { fprintf(stderr, "Error: "); fprintf(stderr, fmt, ## __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }
#define CHECK(ok, fmt,...) { if (!(ok)) FAIL(fmt, ## __VA_ARGS__); }

void fix_vertical_stripes();
void find_and_fix_cold_pixels(int force_analysis);
void chroma_smooth();

#define EV_RESOLUTION 32768

#ifndef MLV2DNG
/* MLV headers */
mlv_hdr_t mlv_hdr;
mlv_file_hdr_t file_hdr;
mlv_rawi_hdr_t rawi_hdr;
mlv_idnt_hdr_t idnt_hdr;
mlv_expo_hdr_t expo_hdr;
mlv_lens_hdr_t lens_hdr;
mlv_rtci_hdr_t rtci_hdr;
mlv_wbal_hdr_t wbal_hdr;
mlv_vidf_hdr_t vidf_hdr;

FILE **load_all_chunks(char *base_filename, int *entries);
void set_out_file_name(char *outname, char *inname);
int parse_sidecar(char *scname);
void init_mlv_structs();
void set_mlvi_block();
void set_rawi_block();
void set_idnt_block();
void set_expo_block();
void set_lens_block();
void set_wbal_block();
void set_rtci_block(char *inname);
void set_vidf_static_block();
uint64_t mlv_generate_guid();
uint64_t mlv_prng_lfsr(uint64_t value);
uint32_t file_set_pos(FILE *stream, uint64_t offset, int whence);

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf(
            "\n"
            "usage:\n"
            "\n"
            "%s file.raw [prefix|--mlv [sidecar]]\n"
            "\n"
            "  prefix    will create prefix000000.dng, prefix0000001.dng and so on.\n"
            "   --mlv    will output MLV with unprocessed raw data and the same name as input.\n"
            " sidecar    if needed specify (prerecorded or any) MLV file to override meaningless\n"
            "            metadata values in IDNT, EXPO, LENS and WBAL blocks\n"
            "\n",
            argv[0]
        );
        return 1;
    }
    
    char* prefix = "";
    char* sidecar_name = "";
    int mlvout = 0, sidecar_ok = 0;
    uint64_t frame_dur_us;
    
    FILE *out_file = NULL;
    FILE **in_files = NULL;
    FILE *in_file = NULL;
    char *in_file_name = argv[1];
    int in_file_count = 0;
    int in_file_num = 0;
    

    in_files = load_all_chunks(in_file_name, &in_file_count);
    if(!in_files || !in_file_count)
    {
        /* Print this out on RAW chunk opening errors */
        printf(" - Exiting program\n");
        exit(1);
    }
    else
    {
        in_file_num = in_file_count;
        in_file = in_files[in_file_num-1];
    }

    if (sizeof(lv_rec_file_footer_t) != 192) FAIL("sizeof(lv_rec_file_footer_t) = %d, should be 192", sizeof(lv_rec_file_footer_t));
    file_set_pos(in_file, -192, SEEK_END);

    memset(&lv_rec_footer, 0x00, sizeof(lv_rec_file_footer_t));
    int r = fread(&lv_rec_footer, 1, sizeof(lv_rec_file_footer_t), in_file);
    CHECK(r == sizeof(lv_rec_file_footer_t), "footer");
    raw_info = lv_rec_footer.raw_info;
    file_set_pos(in_file, 0, SEEK_SET);

    if (strncmp((char*)lv_rec_footer.magic, "RAWM", 4))
        FAIL("This ain't a lv_rec RAW file\n");
    
    if (raw_info.api_version != 1)
        FAIL("API version mismatch: %d\n", raw_info.api_version);
    
    /* override params here (e.g. when the footer is from some other file) */
    //~ lv_rec_footer.xRes=2048;
    //~ lv_rec_footer.yRes=1024;
    //~ lv_rec_footer.frameSize = lv_rec_footer.xRes * lv_rec_footer.yRes * 14/8;
    //~ lv_rec_footer.raw_info.white_level = 16383;
    
    /* print out some lv_rec_footer metadata */
    printf("\nResolution  : %d x %d\n", lv_rec_footer.xRes, lv_rec_footer.yRes);
    printf("Frames      : %d\n", lv_rec_footer.frameCount);
    printf("Frame size  : %d bytes\n", lv_rec_footer.frameSize);
    printf("FPS         : %d.%03d\n", lv_rec_footer.sourceFpsx1000/1000, lv_rec_footer.sourceFpsx1000%1000);
    printf("Black level : %d\n", lv_rec_footer.raw_info.black_level);
    printf("White level : %d\n\n", lv_rec_footer.raw_info.white_level);

    char *raw = malloc(lv_rec_footer.frameSize);
    CHECK(raw, "malloc");
    
    if ((argc > 2) && (strcmp(argv[2], "--mlv") == 0))
    {
        mlvout = 1;
        
        /* Zero all structs */
        init_mlv_structs();

        /* Read sidecar MLV */
        sidecar_name = argc > 3 ? argv[3] : "";
        if(strlen(sidecar_name))
        {
            sidecar_ok = parse_sidecar(sidecar_name);
        }
        
        /* Open MLV file for output */
        char out_file_name[strlen(argv[1])];
        set_out_file_name(out_file_name, argv[1]);
        out_file = fopen(out_file_name, "wb");
        CHECK(out_file, "could not open %s", out_file_name);

        /* Write main file header (MLVI block) */
        set_mlvi_block();
        if(fwrite(&file_hdr, sizeof(mlv_file_hdr_t), 1, out_file) != 1)
        {
            printf("Failed writing MLVI block into .MLV file\n");
            goto abort;
        }

        /* Write RAWI block */
        set_rawi_block();
        if(fwrite(&rawi_hdr, sizeof(mlv_rawi_hdr_t), 1, out_file) != 1)
        {
            printf("Failed writing RAWI block into .MLV file\n");
            goto abort;
        }
        
        /* Write IDNT block */
        if(!sidecar_ok) set_idnt_block();
        if(fwrite(&idnt_hdr, sizeof(mlv_idnt_hdr_t), 1, out_file) != 1)
        {
            printf("Failed writing IDNT block into .MLV file\n");
            goto abort;
        }

        /* Write EXPO block */
        if(!sidecar_ok) set_expo_block();
        if(fwrite(&expo_hdr, sizeof(mlv_expo_hdr_t), 1, out_file) != 1)
        {
            printf("Failed writing EXPO block into .MLV file\n");
            goto abort;
        }

        /* Write LENS block */
        if(!sidecar_ok) set_lens_block();
        if(fwrite(&lens_hdr, sizeof(mlv_lens_hdr_t), 1, out_file) != 1)
        {
            printf("Failed writing LENS block into .MLV file\n");
            goto abort;
        }

        /* Write WBAL block */
        if(!sidecar_ok) set_wbal_block();
        if(fwrite(&wbal_hdr, sizeof(mlv_wbal_hdr_t), 1, out_file) != 1)
        {
            printf("Failed writing WBAL block into .MLV file\n");
            goto abort;
        }

        /* Write RTCI block */
        set_rtci_block(argv[1]);
        if(fwrite(&rtci_hdr, sizeof(mlv_rtci_hdr_t), 1, out_file) != 1)
        {
            printf("Failed writing RTCI block into .MLV file\n");
            goto abort;
        }
    
        /* Set VIDF header static data */
        set_vidf_static_block();
        // Calculate frame duration in microseconds        
        frame_dur_us = (unsigned long long) (1 / ((double)file_hdr.sourceFpsNom / (double)file_hdr.sourceFpsDenom) * 1000000);
    }
    else
    {
        mlvout = 0;
        prefix = argc > 2 ? argv[2] : "";
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

        raw_info.frame_size = lv_rec_footer.frameSize;
        set_idnt_block(); // get the camera name to fill appropriate DNG tag
    }

    int framenumber;
    in_file_num = 0;
    in_file = in_files[in_file_num];
    for (framenumber = 0; framenumber < lv_rec_footer.frameCount; framenumber++)
    {
        printf("\rProcessing frame %d of %d ", framenumber+1, lv_rec_footer.frameCount);
        fflush(stdout);
        
        unsigned int r = fread(raw, 1, lv_rec_footer.frameSize, in_file);
        if(r != lv_rec_footer.frameSize && in_file_num < in_file_count)
        {
            in_file_num++;
            in_file = in_files[in_file_num];
            
            if(r != 0)
            {
                raw += r;
                unsigned int h = fread(raw, 1, (lv_rec_footer.frameSize - r), in_file);
                raw -= r;
                printf("\n\nFrame %d is splitted between neigbour file chunks in a sequence\nReconstructing frame -> %u bytes + %u bytes = %u bytes\n\n", framenumber + 1, r, h, r + h);
            }
        }
        else if(r != lv_rec_footer.frameSize)
        {
            printf("\nError: last file is corrupted.");
            goto abort;
        }
        
        if (!mlvout)
        {
            printf("writing DNG...");
            raw_info.buffer = raw;
            
            /* uncomment if the raw file is recovered from a DNG with dd */
            //~ reverse_bytes_order(raw, lv_rec_footer.frameSize);
            
            char fn[100];
            snprintf(fn, sizeof(fn), "%s%06d.dng", prefix, framenumber);

            fix_vertical_stripes();
            find_and_fix_cold_pixels(0);

            #ifdef CHROMA_SMOOTH
            chroma_smooth();
            #endif

            dng_set_camname((char*)idnt_hdr.cameraName);
            dng_set_framerate(lv_rec_footer.sourceFpsx1000);
            save_dng(fn, &raw_info);
        }
        else
        {
            // VIDF header dynamic data
            vidf_hdr.timestamp += frame_dur_us;
            vidf_hdr.frameNumber = framenumber;
            if(fwrite(&vidf_hdr, sizeof(mlv_vidf_hdr_t), 1, out_file) != 1)
            {
                printf("Failed writing number %d VIDF block header into .MLV file\n", framenumber+1);
                goto abort;
            }
            if(fwrite(raw, lv_rec_footer.frameSize, 1, out_file) != 1)
            {
                printf("Failed writing number %d VIDF block data into .MLV file\n", framenumber+1);
                goto abort;
            }

            printf("writing MLV...");
        }
    }

abort:

    /* Close all opened input files and free list of input files */
    for(in_file_num = 0; in_file_num < in_file_count; in_file_num++)
    {
        fclose(in_files[in_file_num]);
    }
    free(in_files);
    
    free(raw);

    if (!mlvout)
    {
        printf(" Done.\n");
        printf("\nTo convert to jpg, you can try: \n");
        printf("    ufraw-batch --out-type=jpg %s*.dng\n", prefix);
        printf("\nTo get a mjpeg video: \n");
        printf("    ffmpeg -i %s%%6d.jpg -vcodec mjpeg -qscale 1 video.avi\n\n", prefix);
    }
    else
    {
        fclose(out_file);
        printf(" Done.\n");
    }
    
    return 0;
}


void set_out_file_name(char *outname, char *inname)
{
    int namelen = strlen(inname);
    strcpy(outname, inname);
    outname[namelen-4] = '.';
    outname[namelen-3] = 'M';
    outname[namelen-2] = 'L';
    outname[namelen-1] = 'V';
}

int parse_sidecar(char *scname)
{
    FILE* sidecar = fopen(scname, "rb");
    CHECK(sidecar, "could not open %s", scname);
    printf("Using sidecar file '%s'\n", scname);
    
    if(fread(&mlv_hdr, sizeof(mlv_hdr_t), 1, sidecar) != 1)
    {
        printf(" Error: could not read from %s", scname);
        exit(1);   
    }
    if(memcmp(mlv_hdr.blockType, "MLVI", 4) != 0 || mlv_hdr.blockSize != sizeof(mlv_file_hdr_t))
    {
        printf("Error: %s is not a valid MLV", scname);
        exit(1);
    }
    
    /* For safety analyze 30 blocks and search for IDNT, EXPO, LENS and WBAL blocknames, then get 
       values from the first matched of each block and leave not matched ones unchanged if any. 
       If at least one info block matched return 1 otherwise 0 */
    int i = 0, nof = 0, idntf = 0, expof = 0, lensf = 0, wbalf = 0;
    long int mlv_hdr_t_size = sizeof(mlv_hdr_t);
    file_set_pos(sidecar, mlv_hdr.blockSize - mlv_hdr_t_size, SEEK_CUR);
    for (i = 0; i < 30; ++i)
    {
        if(fread(&mlv_hdr, sizeof(mlv_hdr_t), 1, sidecar) != 1)
        {
            printf(" Error: could not read from %s", scname);
            exit(1);
        }
        
        if(!memcmp(mlv_hdr.blockType, "IDNT", 4))
        {
            if(!idntf)
            {
                file_set_pos(sidecar, -mlv_hdr_t_size, SEEK_CUR);
                if(fread(&idnt_hdr, mlv_hdr.blockSize, 1, sidecar) != 1)
                {
                    printf(" Error: could not read from %s", scname);
                    exit(1);
                }
                idnt_hdr.timestamp = 1.300000 * 1000; // override timestamp
                printf(" IDNT");
                idntf = 1;
            }
            else file_set_pos(sidecar, mlv_hdr.blockSize - mlv_hdr_t_size, SEEK_CUR);
            nof++;
        }
        else if(!memcmp(mlv_hdr.blockType, "EXPO", 4))
        {
            if(!expof)
            {
                file_set_pos(sidecar, -mlv_hdr_t_size, SEEK_CUR);
                if(fread(&expo_hdr, mlv_hdr.blockSize, 1, sidecar) != 1)
                {
                    printf(" Error: could not read from %s", scname);
                    exit(1);
                }
                expo_hdr.timestamp = 1.500000 * 1000; // override timestamp
                printf(" EXPO");
                expof = 1;
            }
            else file_set_pos(sidecar, mlv_hdr.blockSize - mlv_hdr_t_size, SEEK_CUR);
            nof++;
        }
        else if(!memcmp(mlv_hdr.blockType, "LENS", 4))
        {
            if(!lensf)
            {
                file_set_pos(sidecar, -mlv_hdr_t_size, SEEK_CUR);
                if(fread(&lens_hdr, mlv_hdr.blockSize, 1, sidecar) != 1)
                {
                    printf(" Error: could not read from %s", scname);
                    exit(1);
                }
                lens_hdr.timestamp = 1.700000 * 1000; // override timestamp
                printf(" LENS");
                lensf = 1;
            }
            else file_set_pos(sidecar, mlv_hdr.blockSize - mlv_hdr_t_size, SEEK_CUR);
            nof++;
        }
        else if(!memcmp(mlv_hdr.blockType, "WBAL", 4))
        {
            if(!wbalf)
            {
                file_set_pos(sidecar, -mlv_hdr_t_size, SEEK_CUR);
                if(fread(&wbal_hdr, mlv_hdr.blockSize, 1, sidecar) != 1)
                {
                    printf(" Error: could not read from %s", scname);
                    exit(1);
                }
                wbal_hdr.timestamp = 1.900000 * 1000; // override timestamp
                printf(" WBAL");
                wbalf = 1;
            }
            else file_set_pos(sidecar, mlv_hdr.blockSize - mlv_hdr_t_size, SEEK_CUR);
            nof++;
        }
        else
        {
            file_set_pos(sidecar, mlv_hdr.blockSize - mlv_hdr_t_size, SEEK_CUR);
            if(nof == 0)
            {
                printf("Found");
                nof++;
            }
        }
        //printf("\n%c%c%c%c", mlv_hdr.blockType[0], mlv_hdr.blockType[1], mlv_hdr.blockType[2], mlv_hdr.blockType[3]);
    }
    fclose(sidecar);
    if(nof == 1)
    {
        printf(" no required block(s)\n\n");
        return 0;
    }
    else
    {
        printf(" block(s)\n\n");
        return 1;
    }
}

void init_mlv_structs()
{
    memset(&mlv_hdr, 0x00, sizeof(mlv_hdr_t));
    memset(&file_hdr, 0x00, sizeof(mlv_file_hdr_t));
    memset(&rawi_hdr, 0x00, sizeof(mlv_rawi_hdr_t));
    memset(&idnt_hdr, 0x00, sizeof(mlv_idnt_hdr_t));
    memset(&expo_hdr, 0x00, sizeof(mlv_expo_hdr_t));
    memset(&lens_hdr, 0x00, sizeof(mlv_lens_hdr_t));
    memset(&rtci_hdr, 0x00, sizeof(mlv_rtci_hdr_t));
    memset(&wbal_hdr, 0x00, sizeof(mlv_wbal_hdr_t));
    memset(&vidf_hdr, 0x00, sizeof(mlv_vidf_hdr_t));
}

void set_mlvi_block()
{
    memcpy(file_hdr.fileMagic, "MLVI", 4);
    file_hdr.blockSize = sizeof(mlv_file_hdr_t);
    memcpy(file_hdr.versionString, "v2.0", 8);
    file_hdr.fileGuid = mlv_generate_guid();
    file_hdr.fileNum = 0;
    file_hdr.fileCount = 1;
    file_hdr.fileFlags = 1;
    file_hdr.videoClass = 1;
    file_hdr.audioClass = 0;
    file_hdr.videoFrameCount = lv_rec_footer.frameCount;
    file_hdr.audioFrameCount = 0;
    file_hdr.sourceFpsNom = lv_rec_footer.sourceFpsx1000;
    file_hdr.sourceFpsDenom = 1000;
}

void set_rawi_block()
{
    memcpy(rawi_hdr.blockType, "RAWI", 4);
    rawi_hdr.blockSize = sizeof(mlv_rawi_hdr_t);
    rawi_hdr.timestamp = 1.100000 * 1000;
    rawi_hdr.xRes = lv_rec_footer.xRes;
    rawi_hdr.yRes = lv_rec_footer.yRes;
    rawi_hdr.raw_info = lv_rec_footer.raw_info;
}

void set_idnt_block()
{
    
    memcpy(idnt_hdr.blockType, "IDNT", 4);
    idnt_hdr.blockSize = sizeof(mlv_idnt_hdr_t);
    idnt_hdr.timestamp = 1.300000 * 1000;

    switch(raw_info.color_matrix1[0])
    {
        case 6722:
            memcpy(idnt_hdr.cameraName, "Canon EOS 5D Mark III", 32);
            idnt_hdr.cameraModel = 0x80000285;
            break;
        case 4716:
            memcpy(idnt_hdr.cameraName, "Canon EOS 5D Mark II", 32);
            idnt_hdr.cameraModel = 0x80000218;
            break;
        case 7034:
            memcpy(idnt_hdr.cameraName, "Canon EOS 6D", 32);
            idnt_hdr.cameraModel = 0x80000302;
            break;
        case 6844:
            memcpy(idnt_hdr.cameraName, "Canon EOS 7D", 32);
            idnt_hdr.cameraModel = 0x80000250;
            break;
        case 4920:
            memcpy(idnt_hdr.cameraName, "Canon EOS 50D", 32);
            idnt_hdr.cameraModel = 0x80000261;
            break;
        case 6719:
            memcpy(idnt_hdr.cameraName, "Canon EOS 60D", 32);
            idnt_hdr.cameraModel = 0x80000287;
            break;
        case 6602:
            memcpy(idnt_hdr.cameraName, "Canon EOS 700D", 32);
            idnt_hdr.cameraModel = 0x80000326;
            break;
        case 6461:
            memcpy(idnt_hdr.cameraName, "Canon EOS 600D", 32);
            idnt_hdr.cameraModel = 0x80000286;
            break;
        case 4763:
            memcpy(idnt_hdr.cameraName, "Canon EOS 500D", 32);
            idnt_hdr.cameraModel = 0x80000252;
            break;
        case 6444:
            memcpy(idnt_hdr.cameraName, "Canon EOS 1100D", 32);
            idnt_hdr.cameraModel = 0x80000288;
            break;
        default:
            memcpy(idnt_hdr.cameraName, "Canon EOS 5DX Mark Free", 32);
            idnt_hdr.cameraModel = 0x8000F4EE;
    }
    
    memcpy(idnt_hdr.cameraSerial, "E055DF4EE", 32);
}

void set_expo_block()
{
    memcpy(expo_hdr.blockType, "EXPO", 4);
    expo_hdr.blockSize = sizeof(mlv_expo_hdr_t);
    expo_hdr.timestamp = 1.500000 * 1000;
    expo_hdr.isoMode = 0;
    expo_hdr.isoValue = 81; // highest dynamic range achieved (5d2) :)
    expo_hdr.isoAnalog = 81;
    expo_hdr.digitalGain = 0;
    expo_hdr.shutterValue = (unsigned long long) (((1000 / (double)(lv_rec_footer.sourceFpsx1000 * 2))) * 1000000); /* 180 degree shutter angle */
}

void set_lens_block()
{
    memcpy(lens_hdr.blockType, "LENS", 4);
    lens_hdr.blockSize = sizeof(mlv_lens_hdr_t);
    lens_hdr.timestamp = 1.700000 * 1000;
    lens_hdr.focalLength = 50;
    lens_hdr.focalDist = 0;
    lens_hdr.aperture = 1 * 100;
    lens_hdr.stabilizerMode = 0;
    lens_hdr.autofocusMode = 0;
    lens_hdr.flags = 0x00000000;
    lens_hdr.lensID = 0x00005010;
    memcpy(lens_hdr.lensName, "Some Great 50mm f/1", 32);
    memcpy(lens_hdr.lensSerial, "", 32);
}

void set_wbal_block()
{
    memcpy(wbal_hdr.blockType, "WBAL", 4);
    wbal_hdr.blockSize = sizeof(mlv_wbal_hdr_t);
    wbal_hdr.timestamp = 1.900000 * 1000;
    wbal_hdr.wb_mode = 9;
    wbal_hdr.kelvin = 6500;
    wbal_hdr.wbgain_r = 0;
    wbal_hdr.wbgain_g = 0;
    wbal_hdr.wbgain_b = 0;
    wbal_hdr.wbs_gm = 0;
    wbal_hdr.wbs_ba = 0;
}

void set_rtci_block(char *inname)
{
    struct stat attr;
    struct tm *fmtime;
    stat(inname, &attr);
    fmtime = localtime(&attr.st_mtime);
    
    memcpy(rtci_hdr.blockType, "RTCI", 4);
    rtci_hdr.blockSize = sizeof(mlv_rtci_hdr_t);
    rtci_hdr.timestamp = 1.110000 * 1000;
    rtci_hdr.tm_sec = fmtime->tm_sec;
    rtci_hdr.tm_min = fmtime->tm_min;
    rtci_hdr.tm_hour = fmtime->tm_hour;
    rtci_hdr.tm_mday = fmtime->tm_mday;
    rtci_hdr.tm_year = fmtime->tm_year;
    rtci_hdr.tm_wday = fmtime->tm_wday;
    rtci_hdr.tm_yday = fmtime->tm_yday;
    rtci_hdr.tm_isdst = 0;
    rtci_hdr.tm_gmtoff = 0;
    memcpy(rtci_hdr.tm_zone, "", 8);
}

void set_vidf_static_block()
{
    /* max res X */
    /* make sure we don't get dead pixels from rounding */
    int left_margin = (raw_info.active_area.x1 + 7) / 8 * 8;
    int right_margin = (raw_info.active_area.x2) / 8 * 8;
    int max = (right_margin - left_margin);
    /* horizontal resolution *MUST* be mod 32 in order to use the fastest EDMAC flags (16 byte transfer) */
    max &= ~31;
    int max_res_x = max;
    /* max res Y */
    int max_res_y = raw_info.jpeg.height & ~1;

    int skip_x = raw_info.active_area.x1 + (max_res_x - lv_rec_footer.xRes) / 2;
    int skip_y = raw_info.active_area.y1 + (max_res_y - lv_rec_footer.yRes) / 2;

    memcpy(vidf_hdr.blockType, "VIDF", 4);
    vidf_hdr.blockSize = sizeof(mlv_vidf_hdr_t) + lv_rec_footer.frameSize;
    vidf_hdr.timestamp = 1000000;
    vidf_hdr.frameNumber = 0;
    vidf_hdr.cropPosX = (skip_x + 7) & ~7;
    vidf_hdr.cropPosY = skip_y & ~1;
    vidf_hdr.panPosX = skip_x;
    vidf_hdr.panPosY = skip_y;
    vidf_hdr.frameSpace = 0;
}

FILE **load_all_chunks(char *base_filename, int *entries)
{
    int seq_number = 0;
    int max_name_len = strlen(base_filename) + 16;
    char *filename = malloc(max_name_len);
    
    strncpy(filename, base_filename, max_name_len - 1);

    /* get extension and check if it is a .RAW */
    char *dot = strrchr(filename, '.');
    if(dot)
    {
        dot++;
        if(strcasecmp(dot, "raw"))
        {
            printf("Not a RAW extension %s", filename);
            free(filename);
            return NULL;
        }
    }
    else
    {
        printf("Incorrect file name %s", filename);
        free(filename);
        return NULL;
    }

    FILE **files = malloc(sizeof(FILE*));

    files[0] = fopen(filename, "rb");
    if(!files[0])
    {
        printf("Failed to open file %s", filename);
        free(filename);
        free(files);
        return NULL;
    }

    printf("\nFound file %s\n", filename);
    
    (*entries)++;
    while(seq_number < 99)
    {
        FILE **realloc_files = realloc(files, (*entries + 1) * sizeof(FILE*));

        if(!realloc_files)
        {
            printf("Error: can not realloc memory");
            free(filename);
            free(files);
            return NULL;
        }

        files = realloc_files;

        /* check for the next file R00, R01 etc */
        char seq_name[8];

        sprintf(seq_name, "%02d", seq_number);
        seq_number++;

        strcpy(&filename[strlen(filename) - 2], seq_name);

        /* try to open */
        files[*entries] = fopen(filename, "rb");
        if(files[*entries])
        {
            printf("Found file %s\n", filename);
            (*entries)++;
        }
        else
        {
            printf("--- End of sequence ---\n");
            break;
        }
    }

    free(filename);
    return files;
}

uint64_t mlv_prng_lfsr(uint64_t value)
{
    uint64_t lfsr = value;
    int max_clocks = 512;

    int clocks;
    for(clocks = 0; clocks < max_clocks; clocks++)
    {
        /* maximum length LFSR according to http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf */
        int bit = ((lfsr >> 63) ^ (lfsr >> 62) ^ (lfsr >> 60) ^ (lfsr >> 59)) & 1;
        lfsr = (lfsr << 1) | bit;
    }

    return lfsr;
}

uint64_t mlv_generate_guid()
{
    time_t rawtime; // use HPET instead? namdvilad upriani iqneboda.
    struct tm *now;
    
    time (&rawtime);
    uint64_t guid = (unsigned long long) rawtime * 1000000;
    now = localtime(&rawtime);
    
    /* now run through prng once to shuffle bits */
    guid = mlv_prng_lfsr(guid);

    /* then seed shuffled bits with rtc time */
    guid ^= now->tm_sec;
    guid ^= now->tm_min << 7;
    guid ^= now->tm_hour << 12;
    guid ^= now->tm_yday << 17;
    guid ^= now->tm_year << 26;
    guid ^= (((unsigned long long)rawtime + 7) * 1000000) << 37;

    /* now run through final prng pass */
    return mlv_prng_lfsr(guid);
}

uint32_t file_set_pos(FILE *stream, uint64_t offset, int whence)
{
#if defined(__WIN32)
    return fseeko64(stream, offset, whence);
#else
    return fseeko(stream, offset, whence);
#endif
}
#endif

int raw_get_pixel(int x, int y) {
    struct raw_pixblock * p = (void*)raw_info.buffer + y * raw_info.pitch + (x/8)*14;
    switch (x%8) {
        case 0: return p->a;
        case 1: return p->b_lo | (p->b_hi << 12);
        case 2: return p->c_lo | (p->c_hi << 10);
        case 3: return p->d_lo | (p->d_hi << 8);
        case 4: return p->e_lo | (p->e_hi << 6);
        case 5: return p->f_lo | (p->f_hi << 4);
        case 6: return p->g_lo | (p->g_hi << 2);
        case 7: return p->h;
    }
    return p->a;
}

void raw_set_pixel(int x, int y, int value)
{
    struct raw_pixblock * p = (void*)raw_info.buffer + y * raw_info.pitch + (x/8)*14;
    switch (x%8) {
        case 0: p->a = value; break;
        case 1: p->b_lo = value; p->b_hi = value >> 12; break;
        case 2: p->c_lo = value; p->c_hi = value >> 10; break;
        case 3: p->d_lo = value; p->d_hi = value >> 8; break;
        case 4: p->e_lo = value; p->e_hi = value >> 6; break;
        case 5: p->f_lo = value; p->f_hi = value >> 4; break;
        case 6: p->g_lo = value; p->g_hi = value >> 2; break;
        case 7: p->h = value; break;
    }
}

/**
 * Fix vertical stripes (banding) from 5D Mark III (and maybe others).
 * 
 * These stripes are periodic, they repeat every 8 pixels.
 * It looks like some columns have different luma amplification;
 * correction factors are somewhere around 0.98 - 1.02, maybe camera-specific, maybe depends on
 * certain settings, I have no idea. So, this fix compares luma values within one pixel block,
 * computes the correction factors (using median to reject outliers) and decides
 * whether to apply the correction or not.
 * 
 * For speed reasons:
 * - Correction factors are computed from the first frame only.
 * - Only channels with error greater than 0.2% are corrected.
 */

#define FIXP_ONE 65536
#define FIXP_RANGE 65536

static int stripes_coeffs[8] = {0};
static int stripes_correction_needed = 0;

/* do not use typeof in macros, use __typeof__ instead.
   see: http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Alternate-Keywords.html#Alternate-Keywords
*/
#define MIN(a,b) \
   ({ __typeof__ ((a)+(b)) _a = (a); \
      __typeof__ ((a)+(b)) _b = (b); \
     _a < _b ? _a : _b; })

#define MAX(a,b) \
   ({ __typeof__ ((a)+(b)) _a = (a); \
       __typeof__ ((a)+(b)) _b = (b); \
     _a > _b ? _a : _b; })

#define ABS(a) \
   ({ __typeof__ (a) _a = (a); \
     _a > 0 ? _a : -_a; })

#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))

#define STR_APPEND(orig,fmt,...) do { int _len = strlen(orig); snprintf(orig + _len, sizeof(orig) - _len, fmt, ## __VA_ARGS__); } while(0)

#define PA ((int)(p->a))
#define PB ((int)(p->b_lo | (p->b_hi << 12)))
#define PC ((int)(p->c_lo | (p->c_hi << 10)))
#define PD ((int)(p->d_lo | (p->d_hi << 8)))
#define PE ((int)(p->e_lo | (p->e_hi << 6)))
#define PF ((int)(p->f_lo | (p->f_hi << 4)))
#define PG ((int)(p->g_lo | (p->g_hi << 2)))
#define PH ((int)(p->h))

#define SET_PA(x) { int v = (x); p->a = v; }
#define SET_PB(x) { int v = (x); p->b_lo = v; p->b_hi = v >> 12; }
#define SET_PC(x) { int v = (x); p->c_lo = v; p->c_hi = v >> 10; }
#define SET_PD(x) { int v = (x); p->d_lo = v; p->d_hi = v >> 8; }
#define SET_PE(x) { int v = (x); p->e_lo = v; p->e_hi = v >> 6; }
#define SET_PF(x) { int v = (x); p->f_lo = v; p->f_hi = v >> 4; }
#define SET_PG(x) { int v = (x); p->g_lo = v; p->g_hi = v >> 2; }
#define SET_PH(x) { int v = (x); p->h = v; }

#define RAW_MUL(p, x) ((((int)(p) - raw_info.black_level) * (int)(x) / FIXP_ONE) + raw_info.black_level)
#define F2H(ev) COERCE((int)(FIXP_RANGE/2 + ev * FIXP_RANGE/2), 0, FIXP_RANGE-1)
#define H2F(x) ((double)((x) - FIXP_RANGE/2) / (FIXP_RANGE/2))

static void add_pixel(int hist[8][FIXP_RANGE], int num[8], int offset, int pa, int pb)
{
    int a = pa;
    int b = pb;
    
    if (MIN(a,b) < 32)
        return; /* too noisy */

    if (MAX(a,b) > raw_info.white_level / 1.1)
        return; /* too bright */
        
    /**
     * compute correction factor for b, that makes it as bright as a
     *
     * first, work around quantization error (which causes huge spikes on histogram)
     * by adding a small random noise component
     * e.g. if raw value is 13, add some uniformly distributed noise,
     * so the value will be between -12.5 and 13.5.
     * 
     * this removes spikes on the histogram, thus canceling bias towards "round" values
     */
    double af = a + (rand() % 1024) / 1024.0 - 0.5;
    double bf = b + (rand() % 1024) / 1024.0 - 0.5;
    double factor = af / bf;
    double ev = log2(factor);
    
    /**
     * add to histogram (for computing the median)
     */
    int weight = log2(a);
    hist[offset][F2H(ev)] += weight;
    num[offset] += weight;
}


static void detect_vertical_stripes_coeffs()
{
    static int hist[8][FIXP_RANGE];
    static int num[8];
    
    memset(hist, 0, sizeof(hist));
    memset(num, 0, sizeof(num));

    /* compute 7 histograms: b./a, c./a ... h./a */
    /* that is, adjust all columns to make them as bright as a */
    /* process green pixels only, assuming the image is RGGB */
    struct raw_pixblock * row;
    for (row = raw_info.buffer; (void*)row < (void*)raw_info.buffer + raw_info.pitch * raw_info.height; row += 2 * raw_info.pitch / sizeof(struct raw_pixblock))
    {
        /* first line is RG */
        struct raw_pixblock * rg;
        for (rg = row; (void*)rg < (void*)row + raw_info.pitch - sizeof(struct raw_pixblock); rg++)
        {
            /* next line is GB */
            struct raw_pixblock * gb = rg + raw_info.pitch / sizeof(struct raw_pixblock);

            struct raw_pixblock * p = rg;
            int pb = PB - raw_info.black_level;
            int pd = PD - raw_info.black_level;
            int pf = PF - raw_info.black_level;
            int ph = PH - raw_info.black_level;
            p++;
            int pb2 = PB - raw_info.black_level;
            int pd2 = PD - raw_info.black_level;
            int pf2 = PF - raw_info.black_level;
            int ph2 = PH - raw_info.black_level;
            p = gb;
            //int pa = PA - raw_info.black_level;
            int pc = PC - raw_info.black_level;
            int pe = PE - raw_info.black_level;
            int pg = PG - raw_info.black_level;
            p++;
            int pa2 = PA - raw_info.black_level;
            int pc2 = PC - raw_info.black_level;
            int pe2 = PE - raw_info.black_level;
            int pg2 = PG - raw_info.black_level;
            
            /**
             * verification: introducing strong banding in one column
             * should not affect the coefficients from the other columns
             **/

            //~ pe = pe * 1.1;
            //~ pe2 = pe2 * 1.1;
            
            /**
             * Make all columns as bright as a2
             * use linear interpolation, so when processing column b, for example,
             * let bi = (b * 1 + b2 * 7) / (7+1)
             * let ei = (e * 4 + e2 * 4) / (4+4)
             * and so on, to avoid getting tricked by smooth gradients.
             */

            add_pixel(hist, num, 1, pa2, (pb * 1 + pb2 * 7) / 8);
            add_pixel(hist, num, 2, pa2, (pc * 2 + pc2 * 6) / 8);
            add_pixel(hist, num, 3, pa2, (pd * 3 + pd2 * 5) / 8);
            add_pixel(hist, num, 4, pa2, (pe * 4 + pe2 * 4) / 8);
            add_pixel(hist, num, 5, pa2, (pf * 5 + pf2 * 3) / 8);
            add_pixel(hist, num, 6, pa2, (pg * 6 + pg2 * 2) / 8);
            add_pixel(hist, num, 7, pa2, (ph * 7 + ph2 * 1) / 8);
        }
    }

    int j,k;
    
    int max[8] = {0};
    for (j = 0; j < 8; j++)
        for (k = 1; k < FIXP_RANGE-1; k++)
            max[j] = MAX(max[j], hist[j][k]);

    /* compute the median correction factor (this will reject outliers) */
    for (j = 0; j < 8; j++)
    {
        if (num[j] < raw_info.frame_size / 128) continue;
        int t = 0;
        for (k = 0; k < FIXP_RANGE; k++)
        {
            t += hist[j][k];
            if (t >= num[j]/2)
            {
                int c = pow(2, H2F(k)) * FIXP_ONE;
                stripes_coeffs[j] = c;
                break;
            }
        }
    }

#if 0
    /* debug graphs */
    FILE* f = fopen("raw2dng.m", "w");
    fprintf(f, "h = {}; x = {}; c = \"rgbcmy\"; \n");
    for (j = 2; j < 8; j++)
    {
        fprintf(f, "h{end+1} = [");
        for (k = 1; k < FIXP_RANGE-1; k++)
        {
            fprintf(f, "%d ", hist[j][k]);
        }
        fprintf(f, "];\n");

        fprintf(f, "x{end+1} = [");
        for (k = 1; k < FIXP_RANGE-1; k++)
        {
            fprintf(f, "%f ", H2F(k) );
        }
        fprintf(f, "];\n");
        fprintf(f, "plot(log2(%d/%d) + [0 0], [0 %d], ['*-' c(%d)]); hold on;\n", stripes_coeffs[j], FIXP_ONE, max[j], j-1);
    }
    fprintf(f, "for i = 1:6, plot(x{i}, h{i}, c(i)); hold on; end;");
    fprintf(f, "axis([-0.05 0.05])");
    fclose(f);
    system("octave-cli --persist raw2dng.m");
#endif

    stripes_coeffs[0] = FIXP_ONE;

    /* do we really need stripe correction, or it won't be noticeable? or maybe it's just computation error? */
    stripes_correction_needed = 0;
    for (j = 0; j < 8; j++)
    {
        double c = (double)stripes_coeffs[j] / FIXP_ONE;
        if (c < 0.998 || c > 1.002)
            stripes_correction_needed = 1;
    }
    
    if (stripes_correction_needed)
    {
        printf("\n\nVertical stripes correction:\n");
        for (j = 0; j < 8; j++)
        {
            if (stripes_coeffs[j])
                printf("  %.5f", (double)stripes_coeffs[j] / FIXP_ONE);
            else
                printf("    1  ");
        }
        printf("\n");
    }
}

static void apply_vertical_stripes_correction()
{
    /**
     * inexact white level will result in banding in highlights, especially if some channels are clipped
     * 
     * so... we'll try to use a better estimation of white level *for this particular purpose*
     * start with a gross under-estimation, then consider white = max(all pixels)
     * just in case the exif one is way off
     * reason: 
     *   - if there are no pixels above the true white level, it shouldn't hurt;
     *     worst case, the brightest pixel(s) will be underexposed by 0.1 EV or so
     *   - if there are, we will choose the true white level
     */
     
    int white = raw_info.white_level * 2 / 3;
    
    struct raw_pixblock * row;
    
    for (row = raw_info.buffer; (void*)row < (void*)raw_info.buffer + raw_info.pitch * raw_info.height; row += raw_info.pitch / sizeof(struct raw_pixblock))
    {
        struct raw_pixblock * p;
        for (p = row; (void*)p < (void*)row + raw_info.pitch; p++)
        {
            white = MAX(white, PA);
            white = MAX(white, PB);
            white = MAX(white, PC);
            white = MAX(white, PD);
            white = MAX(white, PE);
            white = MAX(white, PF);
            white = MAX(white, PG);
            white = MAX(white, PH);
        }
    }
    
    int black = raw_info.black_level;
    for (row = raw_info.buffer; (void*)row < (void*)raw_info.buffer + raw_info.pitch * raw_info.height; row += raw_info.pitch / sizeof(struct raw_pixblock))
    {
        struct raw_pixblock * p;
        for (p = row; (void*)p < (void*)row + raw_info.pitch; p++)
        {
            int pa = PA;
            int pb = PB;
            int pc = PC;
            int pd = PD;
            int pe = PE;
            int pf = PF;
            int pg = PG;
            int ph = PH;
            
            /**
             * Thou shalt not exceed the white level (the exact one, not the exif one)
             * otherwise you'll be blessed with banding instead of nice and smooth highlight recovery
             * 
             * At very dark levels, you will introduce roundoff errors, so don't correct there
             */
            
            if (stripes_coeffs[0] && pa && pa < white && pa > black + 64) SET_PA(MIN(white, RAW_MUL(pa, stripes_coeffs[0])));
            if (stripes_coeffs[1] && pb && pb < white && pa > black + 64) SET_PB(MIN(white, RAW_MUL(pb, stripes_coeffs[1])));
            if (stripes_coeffs[2] && pc && pc < white && pa > black + 64) SET_PC(MIN(white, RAW_MUL(pc, stripes_coeffs[2])));
            if (stripes_coeffs[3] && pd && pd < white && pa > black + 64) SET_PD(MIN(white, RAW_MUL(pd, stripes_coeffs[3])));
            if (stripes_coeffs[4] && pe && pe < white && pa > black + 64) SET_PE(MIN(white, RAW_MUL(pe, stripes_coeffs[4])));
            if (stripes_coeffs[5] && pf && pf < white && pa > black + 64) SET_PF(MIN(white, RAW_MUL(pf, stripes_coeffs[5])));
            if (stripes_coeffs[6] && pg && pg < white && pa > black + 64) SET_PG(MIN(white, RAW_MUL(pg, stripes_coeffs[6])));
            if (stripes_coeffs[7] && ph && ph < white && pa > black + 64) SET_PH(MIN(white, RAW_MUL(ph, stripes_coeffs[7])));
        }
    }
}

void fix_vertical_stripes()
{
    /* for speed: only detect correction factors from the first frame */
    static int first_time = 1;
    if (first_time)
    {
        detect_vertical_stripes_coeffs();
        first_time = 0;
    }
    
    /* only apply stripe correction if we need it, since it takes a little CPU time */
    if (stripes_correction_needed)
    {
        apply_vertical_stripes_correction();
    }
}

static inline int FC(int row, int col)
{
    if ((row%2) == 0 && (col%2) == 0)
    {
        return 0;  /* red */
    }
    else if ((row%2) == 1 && (col%2) == 1)
    {
        return 2;  /* blue */
    }
    else
    {
        return 1;  /* green */
    }
}


void find_and_fix_cold_pixels(int force_analysis)
{
    #define MAX_COLD_PIXELS 200000
  
    struct xy { int x; int y; };
    
    static struct xy cold_pixel_list[MAX_COLD_PIXELS];
    static int cold_pixels = -1;
    
    int w = raw_info.width;
    int h = raw_info.height;
    
    /* scan for bad pixels in the first frame only, or on request*/
    if (cold_pixels < 0 || force_analysis)
    {
        cold_pixels = 0;
        
        /* at sane ISOs, noise stdev is well less than 50, so 200 should be enough */
        int cold_thr = MAX(0, raw_info.black_level - 200);

        /* analyse all pixels of the frame */
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                int p = raw_get_pixel(x, y);
                int is_cold = (p < cold_thr);

                /* create a list containing the cold pixels */
                if (is_cold && cold_pixels < MAX_COLD_PIXELS)
                {
                    cold_pixel_list[cold_pixels].x = x;
                    cold_pixel_list[cold_pixels].y = y;
                    cold_pixels++;
                }
            }
        }
        printf("\rCold pixels : %d                             \n", (cold_pixels));
    }  

    /* repair the cold pixels */
    for (int p = 0; p < cold_pixels; p++)
    {
        int x = cold_pixel_list[p].x;
        int y = cold_pixel_list[p].y;
      
        int neighbours[100];
        int k = 0;
        int fc0 = FC(x, y);

        /* examine the neighbours of the cold pixel */
        for (int i = -4; i <= 4; i++)
        {
            for (int j = -4; j <= 4; j++)
            {
                /* exclude the cold pixel itself from the examination */
                if (i == 0 && j == 0)
                {
                    continue;
                }

                /* exclude out-of-range coords */
                if (x+j < 0 || x+j >= w || y+i < 0 || y+i >= h)
                {
                    continue;
                }
                
                /* examine only the neighbours of the same color */
                if (FC(x+j, y+i) != fc0)
                {
                    continue;
                }

                int p = raw_get_pixel(x+j, y+i);
                neighbours[k++] = -p;
            }
        }
        
        /* replace the cold pixel with the median of the neighbours */
        raw_set_pixel(x, y, -median_int_wirth(neighbours, k));
    }
    
}

#ifdef CHROMA_SMOOTH

static void chroma_smooth_3x3(unsigned short * inp, unsigned short * out, int* raw2ev, int* ev2raw)
{
    int w = raw_info.width;
    int h = raw_info.height;
    int x,y;

    for (y = 4; y < h-5; y += 2)
    {
        for (x = 4; x < w-4; x += 2)
        {
            /**
             * for each red pixel, compute the median value of red minus interpolated green at the same location
             * the median value is then considered the "true" difference between red and green
             * same for blue vs green
             * 
             *
             * each red pixel has 4 green neighbours, so we may interpolate as follows:
             * - mean or median(t,b,l,r)
             * - choose between mean(t,b) and mean(l,r) (idea from AHD)
             * 
             * same for blue; note that a RG/GB cell has 6 green pixels that we need to analyze
             * 2 only for red, 2 only for blue, and 2 shared
             *    g
             *   gRg
             *    gBg
             *     g
             *
             * choosing the interpolation direction seems to give cleaner results
             * the direction is choosen over the entire filtered area (so we do two passes, one for each direction, 
             * and at the end choose the one for which total interpolation error is smaller)
             * 
             * error = sum(abs(t-b)) or sum(abs(l-r))
             * 
             * interpolation in EV space (rather than linear) seems to have less color artifacts in high-contrast areas
             * 
             * we can use this filter for 3x3 RG/GB cells or 5x5
             */
            int i,j;
            int k = 0;
            int med_r[9];
            int med_b[9];
            
            /* first try to interpolate in horizontal direction */
            int eh = 0;
            for (i = -2; i <= 2; i += 2)
            {
                for (j = -2; j <= 2; j += 2)
                {
                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                    int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                  //int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                    int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                  //int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1];
                    g2 = raw2ev[g2];
                    g3 = raw2ev[g3];
                  //g4 = raw2ev[g4];
                    g5 = raw2ev[g5];
                  //g6 = raw2ev[g6];
                    
                    int gr = (g1+g3)/2;
                    int gb = (g2+g5)/2;
                    eh += ABS(g1-g3) + ABS(g2-g5);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            /* difference from green, with horizontal interpolation */
            int drh = opt_med9(med_r);
            int dbh = opt_med9(med_b);
            
            /* next, try to interpolate in vertical direction */
            int ev = 0;
            k = 0;
            for (i = -2; i <= 2; i += 2)
            {
                for (j = -2; j <= 2; j += 2)
                {
                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                  //int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                    int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                  //int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                    int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1];
                    g2 = raw2ev[g2];
                  //g3 = raw2ev[g3];
                    g4 = raw2ev[g4];
                  //g5 = raw2ev[g5];
                    g6 = raw2ev[g6];
                    
                    int gr = (g2+g4)/2;
                    int gb = (g1+g6)/2;
                    ev += ABS(g2-g4) + ABS(g1-g6);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            /* difference from green, with vertical interpolation */
            int drv = opt_med9(med_r);
            int dbv = opt_med9(med_b);

            /* back to our filtered pixels (RG/GB cell) */
            int g1 = inp[x+1 +     y * w];
            int g2 = inp[x   + (y+1) * w];
            int g3 = inp[x-1 +   (y) * w];
            int g4 = inp[x   + (y-1) * w];
            int g5 = inp[x+2 + (y+1) * w];
            int g6 = inp[x+1 + (y+2) * w];
            
            g1 = raw2ev[g1];
            g2 = raw2ev[g2];
            g3 = raw2ev[g3];
            g4 = raw2ev[g4];
            g5 = raw2ev[g5];
            g6 = raw2ev[g6];

            /* which of the two interpolations will we choose? */
            int grv = (g2+g4)/2;
            int grh = (g1+g3)/2;
            int gbv = (g1+g6)/2;
            int gbh = (g2+g5)/2;
            int gr = ev < eh ? grv : grh;
            int gb = ev < eh ? gbv : gbh;
            int dr = ev < eh ? drv : drh;
            int db = ev < eh ? dbv : dbh;
            
            int r0 = inp[x   +     y * w];
            int b0 = inp[x+1 + (y+1) * w];

            /* if we are close to the noise floor, use both directions, beacuse otherwise it will affect the noise structure and introduce false detail */
            /* todo: smooth transition between the two methods? better thresholding condition? */
            int thr = 64;
            if (r0 < raw_info.black_level+thr || b0 < raw_info.black_level+thr || ABS(drv - drh) < thr || ABS(grv-grh) < thr || ABS(gbv-gbh) < thr)
            {
                dr = (drv+drh)/2;
                db = (dbv+dbh)/2;
                gr = (g1+g2+g3+g4)/4;
                gb = (g1+g2+g5+g6)/4;
            }

            /* replace red and blue pixels with filtered values, keep green pixels unchanged */
            /* don't touch overexposed areas */
            if (out[x   +     y * w] < raw_info.white_level)
                out[x   +     y * w] = ev2raw[COERCE(gr + dr, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)];
            
            if (out[x+1  + (y+1)* w] < raw_info.white_level)
                out[x+1 + (y+1) * w] = ev2raw[COERCE(gb + db, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)];
        }
    }
}

/* processes top, bottom, left and right neighbours */
static void chroma_smooth_2x2(unsigned short * inp, unsigned short * out, int* raw2ev, int* ev2raw)
{
    int w = raw_info.width;
    int h = raw_info.height;
    int x,y;

    for (y = 4; y < h-5; y += 2)
    {
        for (x = 4; x < w-4; x += 2)
        {
            /**
             * for each red pixel, compute the median value of red minus interpolated green at the same location
             * the median value is then considered the "true" difference between red and green
             * same for blue vs green
             * 
             *
             * each red pixel has 4 green neighbours, so we may interpolate as follows:
             * - mean or median(t,b,l,r)
             * - choose between mean(t,b) and mean(l,r) (idea from AHD)
             * 
             * same for blue; note that a RG/GB cell has 6 green pixels that we need to analyze
             * 2 only for red, 2 only for blue, and 2 shared
             *    g
             *   gRg
             *    gBg
             *     g
             *
             * choosing the interpolation direction seems to give cleaner results
             * the direction is choosen over the entire filtered area (so we do two passes, one for each direction, 
             * and at the end choose the one for which total interpolation error is smaller)
             * 
             * error = sum(abs(t-b)) or sum(abs(l-r))
             * 
             * interpolation in EV space (rather than linear) seems to have less color artifacts in high-contrast areas
             * 
             * we can use this filter for 3x3 RG/GB cells or 5x5
             */
            int i,j;
            int k = 0;
            int med_r[5];
            int med_b[5];
            
            /* first try to interpolate in horizontal direction */
            int eh = 0;
            for (i = -2; i <= 2; i += 2)
            {
                for (j = -2; j <= 2; j += 2)
                {
                    if (ABS(i) + ABS(j) == 4) continue;
                    
                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                    int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                  //int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                    int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                  //int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1];
                    g2 = raw2ev[g2];
                    g3 = raw2ev[g3];
                  //g4 = raw2ev[g4];
                    g5 = raw2ev[g5];
                  //g6 = raw2ev[g6];
                    
                    int gr = (g1+g3)/2;
                    int gb = (g2+g5)/2;
                    eh += ABS(g1-g3) + ABS(g2-g5);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            /* difference from green, with horizontal interpolation */
            int drh = opt_med5(med_r);
            int dbh = opt_med5(med_b);
            
            /* next, try to interpolate in vertical direction */
            int ev = 0;
            k = 0;
            for (i = -2; i <= 2; i += 2)
            {
                for (j = -2; j <= 2; j += 2)
                {
                    if (ABS(i) + ABS(j) == 4) continue;

                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                  //int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                    int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                  //int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                    int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1];
                    g2 = raw2ev[g2];
                  //g3 = raw2ev[g3];
                    g4 = raw2ev[g4];
                  //g5 = raw2ev[g5];
                    g6 = raw2ev[g6];
                    
                    int gr = (g2+g4)/2;
                    int gb = (g1+g6)/2;
                    ev += ABS(g2-g4) + ABS(g1-g6);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            /* difference from green, with vertical interpolation */
            int drv = opt_med5(med_r);
            int dbv = opt_med5(med_b);

            /* back to our filtered pixels (RG/GB cell) */
            int g1 = inp[x+1 +     y * w];
            int g2 = inp[x   + (y+1) * w];
            int g3 = inp[x-1 +   (y) * w];
            int g4 = inp[x   + (y-1) * w];
            int g5 = inp[x+2 + (y+1) * w];
            int g6 = inp[x+1 + (y+2) * w];
            
            g1 = raw2ev[g1];
            g2 = raw2ev[g2];
            g3 = raw2ev[g3];
            g4 = raw2ev[g4];
            g5 = raw2ev[g5];
            g6 = raw2ev[g6];

            /* which of the two interpolations will we choose? */
            int grv = (g2+g4)/2;
            int grh = (g1+g3)/2;
            int gbv = (g1+g6)/2;
            int gbh = (g2+g5)/2;
            int gr = ev < eh ? grv : grh;
            int gb = ev < eh ? gbv : gbh;
            int dr = ev < eh ? drv : drh;
            int db = ev < eh ? dbv : dbh;
            
            int r0 = inp[x   +     y * w];
            int b0 = inp[x+1 + (y+1) * w];

            /* if we are close to the noise floor, use both directions, beacuse otherwise it will affect the noise structure and introduce false detail */
            /* todo: smooth transition between the two methods? better thresholding condition? */
            int thr = 64;
            if (r0 < raw_info.black_level+thr || b0 < raw_info.black_level+thr || ABS(drv - drh) < thr || ABS(grv-grh) < thr || ABS(gbv-gbh) < thr)
            {
                dr = (drv+drh)/2;
                db = (dbv+dbh)/2;
                gr = (g1+g2+g3+g4)/4;
                gb = (g1+g2+g5+g6)/4;
            }

            /* replace red and blue pixels with filtered values, keep green pixels unchanged */
            /* don't touch overexposed areas */
            if (out[x   +     y * w] < raw_info.white_level)
                out[x   +     y * w] = ev2raw[COERCE(gr + dr, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)];
            
            if (out[x+1  + (y+1)* w] < raw_info.white_level)
                out[x+1 + (y+1) * w] = ev2raw[COERCE(gb + db, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)];
        }
    }
}

static void chroma_smooth_5x5(unsigned short * inp, unsigned short * out, int* raw2ev, int* ev2raw)
{
    int w = raw_info.width;
    int h = raw_info.height;
    int x,y;

    for (y = 6; y < h-7; y += 2)
    {
        for (x = 6; x < w-6; x += 2)
        {
            /**
             * for each red pixel, compute the median value of red minus interpolated green at the same location
             * the median value is then considered the "true" difference between red and green
             * same for blue vs green
             * 
             *
             * each red pixel has 4 green neighbours, so we may interpolate as follows:
             * - mean or median(t,b,l,r)
             * - choose between mean(t,b) and mean(l,r) (idea from AHD)
             * 
             * same for blue; note that a RG/GB cell has 6 green pixels that we need to analyze
             * 2 only for red, 2 only for blue, and 2 shared
             *    g
             *   gRg
             *    gBg
             *     g
             *
             * choosing the interpolation direction seems to give cleaner results
             * the direction is choosen over the entire filtered area (so we do two passes, one for each direction, 
             * and at the end choose the one for which total interpolation error is smaller)
             * 
             * error = sum(abs(t-b)) or sum(abs(l-r))
             * 
             * interpolation in EV space (rather than linear) seems to have less color artifacts in high-contrast areas
             * 
             * we can use this filter for 3x3 RG/GB cells or 5x5
             */
            int i,j;
            int k = 0;
            int med_r[25];
            int med_b[25];
            
            /* first try to interpolate in horizontal direction */
            int eh = 0;
            for (i = -4; i <= 4; i += 2)
            {
                for (j = -4; j <= 4; j += 2)
                {
                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                    int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                  //int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                    int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                  //int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1];
                    g2 = raw2ev[g2];
                    g3 = raw2ev[g3];
                  //g4 = raw2ev[g4];
                    g5 = raw2ev[g5];
                  //g6 = raw2ev[g6];
                    
                    int gr = (g1+g3)/2;
                    int gb = (g2+g5)/2;
                    eh += ABS(g1-g3) + ABS(g2-g5);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            /* difference from green, with horizontal interpolation */
            int drh = opt_med25(med_r);
            int dbh = opt_med25(med_b);
            
            /* next, try to interpolate in vertical direction */
            int ev = 0;
            k = 0;
            for (i = -4; i <= 4; i += 2)
            {
                for (j = -4; j <= 4; j += 2)
                {
                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                  //int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                    int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                  //int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                    int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1];
                    g2 = raw2ev[g2];
                  //g3 = raw2ev[g3];
                    g4 = raw2ev[g4];
                  //g5 = raw2ev[g5];
                    g6 = raw2ev[g6];
                    
                    int gr = (g2+g4)/2;
                    int gb = (g1+g6)/2;
                    ev += ABS(g2-g4) + ABS(g1-g6);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            /* difference from green, with vertical interpolation */
            int drv = opt_med25(med_r);
            int dbv = opt_med25(med_b);

            /* back to our filtered pixels (RG/GB cell) */
            int g1 = inp[x+1 +     y * w];
            int g2 = inp[x   + (y+1) * w];
            int g3 = inp[x-1 +   (y) * w];
            int g4 = inp[x   + (y-1) * w];
            int g5 = inp[x+2 + (y+1) * w];
            int g6 = inp[x+1 + (y+2) * w];
            
            g1 = raw2ev[g1];
            g2 = raw2ev[g2];
            g3 = raw2ev[g3];
            g4 = raw2ev[g4];
            g5 = raw2ev[g5];
            g6 = raw2ev[g6];

            /* which of the two interpolations will we choose? */
            int grv = (g2+g4)/2;
            int grh = (g1+g3)/2;
            int gbv = (g1+g6)/2;
            int gbh = (g2+g5)/2;
            int gr = ev < eh ? grv : grh;
            int gb = ev < eh ? gbv : gbh;
            int dr = ev < eh ? drv : drh;
            int db = ev < eh ? dbv : dbh;
            
            int r0 = inp[x   +     y * w];
            int b0 = inp[x+1 + (y+1) * w];

            /* if we are close to the noise floor, use both directions, beacuse otherwise it will affect the noise structure and introduce false detail */
            /* todo: smooth transition between the two methods? better thresholding condition? */
            int thr = 64;
            if (r0 < raw_info.black_level+thr || b0 < raw_info.black_level+thr || ABS(drv - drh) < thr || ABS(grv-grh) < thr || ABS(gbv-gbh) < thr)
            {
                dr = (drv+drh)/2;
                db = (dbv+dbh)/2;
                gr = (g1+g2+g3+g4)/4;
                gb = (g1+g2+g5+g6)/4;
            }
            
            /* replace red and blue pixels with filtered values, keep green pixels unchanged */
            /* don't touch overexposed areas */
            if (out[x   +     y * w] < raw_info.white_level)
                out[x   +     y * w] = ev2raw[COERCE(gr + dr, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)];
            
            if (out[x+1  + (y+1)* w] < raw_info.white_level)
                out[x+1 + (y+1) * w] = ev2raw[COERCE(gb + db, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)];
        }
    }
}

void chroma_smooth()
{
    int black = raw_info.black_level;
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

    int w = raw_info.width;
    int h = raw_info.height;

    unsigned short * aux = malloc(w * h * sizeof(short));
    unsigned short * aux2 = malloc(w * h * sizeof(short));

    int x,y;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            aux[x + y*w] = aux2[x + y*w] = raw_get_pixel(x, y);
    
    chroma_smooth_2x2(aux, aux2, raw2ev, ev2raw);
    
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            raw_set_pixel(x, y, aux2[x + y*w]);

    free(aux);
    free(aux2);
}
#endif
