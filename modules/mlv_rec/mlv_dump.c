

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/raw.h"
#include "mlv.h"


int main (int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Usage: %s <file.mlv>\n", argv[0]);
        return 0;
    }
    
    FILE *file = fopen(argv[1], "r");
    if(!file)
    {
        printf("Failed to open file '%s'\n", argv[1]);
        return 0;
    }
    
    do
    {
        mlv_hdr_t buf;
        
        if(fread(&buf, sizeof(mlv_hdr_t), 1, file) != 1)
        {
            printf("End of file\n");
            return 0;
        }
        fseek(file, -sizeof(mlv_hdr_t), SEEK_CUR);
        
        /* file header */
        if(!memcmp(buf.blockType, "MLVI", 4))
        {
            mlv_file_hdr_t file_hdr;
            
            if(fread(&file_hdr, sizeof(mlv_file_hdr_t), 1, file) != 1)
            {
                printf("MLVI: Reached end?\n");
                return 0;
            }
            fseek(file, file_hdr.blockSize-sizeof(mlv_file_hdr_t), SEEK_CUR);
            
            printf("File Header (MLVI)\n");
            printf("    Size        : 0x%08X\n", file_hdr.blockSize);
            printf("    Ver         : %s\n", file_hdr.versionString);
            printf("    GUID        : %08llX\n", file_hdr.fileGuid);
            printf("    FPS         : %f\n", (double)file_hdr.sourceFpsNom / (double)file_hdr.sourceFpsDenom);
            printf("    Frames Video: %d\n", file_hdr.videoFrameCount);
            printf("    Frames Audio: %d\n", file_hdr.audioFrameCount);
            
        }
        else
        {
            printf("Block: %c%c%c%c\n", buf.blockType[0], buf.blockType[1], buf.blockType[2], buf.blockType[3]);
            printf("    Size: 0x%08X\n", buf.blockSize);
            printf("    Time: %f ms\n", (double)buf.timestamp / 1000.0d);
            
            if(!memcmp(buf.blockType, "VIDF", 4))
            {
                mlv_vidf_hdr_t block_hdr;
                
                if(fread(&block_hdr, sizeof(mlv_vidf_hdr_t), 1, file) != 1)
                {
                    printf("Reached file end within block.\n");
                    return 0;
                }
                fseek(file, block_hdr.blockSize-sizeof(mlv_vidf_hdr_t), SEEK_CUR);
                
                printf("     Pad: 0x%08X\n", block_hdr.frameSpace);
                printf("   Frame: #%d\n", block_hdr.frameNumber);
            }
            else if(!memcmp(buf.blockType, "RAWI", 4))
            {
                mlv_rawi_hdr_t block_hdr;
                
                if(fread(&block_hdr, sizeof(mlv_rawi_hdr_t), 1, file) != 1)
                {
                    printf("Reached file end within block.\n");
                    return 0;
                }
                fseek(file, block_hdr.blockSize-sizeof(mlv_rawi_hdr_t), SEEK_CUR);
                
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
            else 
            {
                fseek(file, buf.blockSize, SEEK_CUR);
            }
        }
    }
    while(!feof(file));
    
}