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

// Red balance is the ratio G/R for a neutral color (typically > 1)
// Blue balance is the ratio G/B for a neutral color (typically > 1)
void read_white_balance(const char* filename, float* red_balance, float* blue_balance)
{
    char exif_cmd[1000];
    int error = 0;
    int mode;
    int r, g1, g2, b;

    //Determine WB mode (0 means Auto)
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -WhiteBalance -b \"%s\"", filename);
    FILE* exif_file = popen(exif_cmd, "r");
    if (exif_file)
    {
        if (fscanf(exif_file, "%d", &mode) != 1) error = 1;
        pclose(exif_file);
    }
    else error = 1;

    //If WB mode is Auto, read the WB_RGGBLevelsMeasured values, otherwise read the WB_RGGBLevelsAsShot values
    //WB_RGGBLevelsAuto values are avoided because they may contain (incorrect) adjustments by Canon
    //If WB_RGGBLevels* is missing, fall back on MeasuredRGGB values
    if (mode == 0)
    {
        snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -WB_RGGBLevelsMeasured -b \"%s\"", filename);
    }
    else
    {
        snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -WB_RGGBLevelsAsShot -b \"%s\"", filename);
    }
    exif_file = popen(exif_cmd, "r");
    if (exif_file)
    {
        if (fscanf(exif_file, "%d %d %d %d", &r, &g1, &g2, &b) != 4) //failed to read WB_RGGBLevels*
        {
            snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -MeasuredRGGB -b \"%s\"", filename);
            FILE* exif_file2 = popen(exif_cmd, "r");
            if (exif_file2)
            {
                if (fscanf(exif_file2, "%d %d %d %d", &r, &g1, &g2, &b) != 4) error = 1; //failed to read MeasuredRGGB
                else
                {
                    //MeasuredRGGB values are proportional to the values of a neutral color
                    *red_balance = ((float)g1)/r;
                    *blue_balance = ((float)g2)/b;
                }
                pclose(exif_file2);
            }
        }
        else
        {
            //WB_RGGBLevels* values are multipliers, so there is an implied inverse
            *red_balance = ((float)r)/g1;
            *blue_balance = ((float)b)/g2;
        }
        pclose(exif_file);
    }
    else error = 1;

    if (error) printf("**WARNING** could not understand white balance information\n");
}

