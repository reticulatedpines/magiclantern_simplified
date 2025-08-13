
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

#include <getopt.h>


#define ST_SYNC         0
#define ST_BIT          1
#define ST_SPACE        2

#define LEVEL_UNK       0
#define LEVEL_HI        1
#define LEVEL_LO        2

/* may get tweaked to better match the bit times of the recording */
uint32_t len_sync = 20;
uint32_t len_zero = 3;
uint32_t len_one = 10;
uint32_t len_space = 3;
uint32_t inverted = 0;

/* this parameter should match the setting in dumper, assuming bit times above were untouched */
uint32_t slowdown = 1;

/* make them fit the signal amplitudes */
int32_t level_hi = 120;
int32_t level_lo = 110;

/* auto DC-offset compensation, the lower the number, the slower it learns. 
   tries to undo sound card's DC filtering by integrating.
   for rates close to the sampling rate, 0.6 seems to be a good starting point.
*/
float avg_fact = 0;

float dc_offset = 0;

int get_level(short v)
{
    static int last_level = LEVEL_UNK;
    
    if (v >= level_hi)
    {
        last_level = inverted?LEVEL_LO:LEVEL_HI;
    }
    else if (v <= level_lo)
    {
        last_level = inverted?LEVEL_HI:LEVEL_LO;
    }
    
    return last_level;
}

int main(int argc, char *argv[])
{
    uint8_t buf[1024];
    int bytes_written = 0;
    int len = 0;
    int p = 0;
    int state = ST_SYNC;
    int pp = 0;
    int lv = 0;
    int cnt = 0;
    int bits = 0;
    int n = 0;
    unsigned char byte = 0;
    FILE *fo = NULL;
    FILE *f = NULL;
    char *out_filename = "dump";
    char *in_filename = "-";

    struct option long_options[] = {
        {"len_sync",  required_argument, NULL, 'y' },
        {"len_space", required_argument, NULL, 's' },
        {"len_zero",  required_argument, NULL, 'z' },
        {"len_one",   required_argument, NULL, 'o' },
        {"level_hi",  required_argument, NULL, 'h' },
        {"level_lo",  required_argument, NULL, 'l' },
        {"inverted",  no_argument, &inverted, 1 },
        {0,         0,                 0,  0 }
    };

    int index = 0;
    char opt = ' ';
    while ((opt = getopt_long(argc, argv, "y:s:z:o:h:l:", long_options, &index)) != -1)
    {
        switch (opt)
        {
            case 'y':
                if(!optarg)
                {
                    printf("Error: Missing parameter\n");
                    return -1;
                }
                else
                {
                    len_sync = atoi(optarg);
                }
                break;
                
            case 's':
                if(!optarg)
                {
                    printf("Error: Missing parameter\n");
                    return -1;
                }
                else
                {
                    len_space = atoi(optarg);
                }
                break;
                
            case 'z':
                if(!optarg)
                {
                    printf("Error: Missing parameter\n");
                    return -1;
                }
                else
                {
                    len_zero = atoi(optarg);
                }
                break;
                
            case 'o':
                if(!optarg)
                {
                    printf("Error: Missing parameter\n");
                    return -1;
                }
                else
                {
                    len_one = atoi(optarg);
                }
                break;
                
            case 'h':
                if(!optarg)
                {
                    printf("Error: Missing parameter\n");
                    return -1;
                }
                else
                {
                    level_hi = atoi(optarg);
                }
                break;
                
            case 'l':
                if(!optarg)
                {
                    printf("Error: Missing parameter\n");
                    return -1;
                }
                else
                {
                    level_lo = atoi(optarg);
                }
                break;

            case 0:
                break;

            default:
                return -1;
        }
    }

    if(optind >= argc)
    {
        printf("Error: Missing input filename\n");
        return -1;
    }


    if(optind < argc)
    {
        in_filename = argv[optind];
    }
    optind++;
    
    if(optind < argc)
    {
        out_filename = argv[optind];
    }
    
    /* open input file */
    if(!strcmp(in_filename, "-"))
    {
        printf("Reading from stdin\n");
        f = stdin;
    }
    else
    {
        f = fopen(in_filename, "r+b");
    }

    if(!f)
    {
        printf("Failed to open input file '%s'\n", in_filename);
        return;
    }
    
    /* open output filename */
    if(!strcmp(out_filename, "-"))
    {
        printf("Writing to stdout\n");
        fo = stdout;
    }
    else
    {
        fo = fopen(out_filename, "w+b");
    }

    if(!fo)
    {
        printf("Failed to open output file '%s'\n", out_filename);
        return;
    }

    while (!feof(f))
    {
        pp += len;
        len = fread(buf, 1, sizeof(buf), f);
        p=0;
        while (p<len)
        {
            uint8_t signal = buf[p++];
            
            dc_offset = (dc_offset * (1 - avg_fact)) + (signal * avg_fact);

            lv = get_level(signal + dc_offset);
            
            if (lv == LEVEL_UNK)
            {
                continue;
            }
            
#if 0
            fwrite((lv == LEVEL_HI)?"^":"_",1,1,fo);
            continue;
#endif
            switch (state)
            {
                case ST_SYNC:
                    if(lv == LEVEL_HI)
                    {
                        ++cnt;
                    }
                    else
                    {
                        printf("sync: %d\n", cnt);
                        if(cnt >= len_sync * slowdown)
                        {
                            cnt = 1;
                            state = ST_BIT;
                            bits = 0;
                            byte = 0;
                        }
                        else
                        {
                            cnt = 0;
                            state = ST_SYNC;
                            printf("%06X [0x%08X]: SYNC ERROR!\n", n, pp+p);
                        }
                    }
                    break;
                    
                case ST_BIT:
                    if(lv == LEVEL_LO)
                    {
                        ++cnt;
                    }
                    else
                    {
                        printf("bit: %d\n", cnt);
                        if(cnt >= len_one * slowdown && cnt <= 2 * len_one * slowdown)
                        {
                            cnt = 1;
                            byte |= (1<<bits);
                            if((++bits) == 8)
                            {
                                fwrite(&byte, 1, 1, fo);
                                bytes_written++;
                                state = ST_SYNC;
                                ++n;
                            }
                            else
                            {
                                state = ST_SPACE;
                            }
                        }
                        else if(cnt >= len_zero * slowdown && cnt < len_one * slowdown)
                        {
                            cnt = 1;
                            if((++bits) == 8)
                            {
                                fwrite(&byte, 1, 1, fo);
                                bytes_written++;
                                state = ST_SYNC;
                                ++n;
                            }
                            else
                            {
                                state = ST_SPACE;
                            }
                        }
                        else
                        {
                            cnt = 1;
                            state = ST_SYNC;
                            printf("%06X [0x%08X]: BIT SYNC ERROR!\n", n, pp+p);
                        }
                    }
                    break;
                    
                case ST_SPACE:
                    if(lv == LEVEL_HI)
                    {
                        ++cnt;
                    }
                    else
                    {
                        printf("space: %d\n", cnt);
                        if(cnt >= len_space * slowdown)
                        {
                            cnt = 1;
                            state = ST_BIT;
                        }
                        else
                        {
                            cnt = 0;
                            state = ST_SYNC;
                            printf("%06X [0x%08X]: SPACE SYNC ERROR!\n", n, pp+p);
                        }
                    }
                    break;
            }
        }
    }
    
    printf("Done, wrote %d byte, DC offset was %2.2f\n", bytes_written, dc_offset);

    fclose(fo);
    fclose(f);
}
