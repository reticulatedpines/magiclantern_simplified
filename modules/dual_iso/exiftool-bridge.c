#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "exiftool-bridge.h"

#define DEFAULT_MODEL_ID 0x285

void copy_tags_from_source(const char* source, const char* dest)
{
    char exif_cmd[1000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -tagsFromFile \"%s\" -all:all \"-xmp:subject=Dual-ISO\" \"-UniqueCameraModel<Model\" \"%s\" -overwrite_original", source, dest);
    int r = system(exif_cmd);
    if(r!=0)
    {
        printf("**WARNING** exiftool couldn't update DNG tag data\n");
    }
}

unsigned int get_model_id(const char* filename)
{
    unsigned tag = DEFAULT_MODEL_ID;
    char exif_cmd[10000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -CanonModelID -b \"%s\"", filename);
    FILE* exif_file = popen(exif_cmd, "r");
    if(exif_file) 
    {
        if (!fscanf(exif_file, "%u", &tag))
            goto err;
        pclose(exif_file);
    }
    else
    {
        err:
        printf("**WARNING** could not identify the camera (exiftool problem). Assuming it's a 5D Mark III\n");
    }
    return tag & 0xFFF;
}

/*
This function uses EXIF information to calculate the following two ratios:
  Red balance is the ratio G/R for a neutral color (typically > 1)
  Blue balance is the ratio G/B for a neutral color (typically > 1)
Use only on dual ISO shots!
*/
void read_white_balance(const char* filename, float* red_balance, float* blue_balance)
{
    //Assume that the RedBalance and BlueBalance values are not trustworthy

    char exif_cmd[1000];
    int error = 0;
    int mode;
    int wb_r, wb_g1, wb_g2, wb_b;
    int raw_r, raw_g1, raw_g2, raw_b;

    //Determine WB mode (0 means Auto)
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -WhiteBalance -b \"%s\"", filename);
    FILE* exif_file = popen(exif_cmd, "r");
    if (exif_file)
    {
        if (fscanf(exif_file, "%d", &mode) != 1) error = 1;
        pclose(exif_file);
    }
    else error = 1;
    
    if (error) goto err;

    //If WB mode is not Auto, use WB_RGGBLevelsAsShot values
    //If WB mode is Auto, read WB_RGGBLevelsMeasured values
    //  (not WB_RGGBLevelsAuto values because they can be temperature-shifted)
    //  Use WB_RGGBLevelsMeasured values if they have a significant difference between the G channels
    //  Otherwise, use RawMeasuredRGGB values
    if (mode != 0)
    {
        snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -WB_RGGBLevelsAsShot -b \"%s\"", filename);
    }
    else
    {
        snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -WB_RGGBLevelsMeasured -b \"%s\"", filename);
    }
    exif_file = popen(exif_cmd, "r");
    if (exif_file)
    {
        if (fscanf(exif_file, "%d %d %d %d", &wb_r, &wb_g1, &wb_g2, &wb_b) != 4) error = 1;
        else
        {
            if ((mode !=0) || (wb_g1-wb_g2 > wb_g2/2) || (wb_g2-wb_g1 > wb_g1/2))
            {
                printf("White balance   : from %s\n", mode != 0 ? "WB_RGGBLevelsAsShot" : "WB_RGGBLevelsMeasured");
                //WB_RGGBLevels* values are multipliers, so there is an implied inverse
                *red_balance = ((float)wb_r)/wb_g1;
                *blue_balance = ((float)wb_b)/wb_g2;
            }
            else
            {
                snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -RawMeasuredRGGB -b \"%s\"", filename);
                FILE* exif_file2 = popen(exif_cmd, "r");
                if (exif_file2)
                {
                    if (fscanf(exif_file2, "%d %d %d %d", &raw_r, &raw_g1, &raw_g2, &raw_b) != 4) error = 1;
                    else
                    {
                        printf("White balance   : from RawMeasuredRGGB\n");
                        //RawMeasuredRGGB values are proportional to the values of a neutral color
                        *red_balance = ((float)raw_g1)/raw_r;
                        *blue_balance = ((float)raw_g2)/raw_b;
                    }
                    pclose(exif_file2);
                }
            }
        }
        pclose(exif_file);
    }
    else error = 1;

err:
    if (error) printf("**WARNING** could not extract white balance information, exiftool may need to be updated\n");
}

