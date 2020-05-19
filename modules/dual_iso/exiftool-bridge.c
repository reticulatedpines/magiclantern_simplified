#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "exiftool-bridge.h"

#define DEFAULT_MODEL_ID 0x285

void copy_tags_from_source(const char* source, const char* dest)
{
    char exif_cmd[1000];
    printf("%-16s: copying EXIF from %s\n", dest, source);
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -tagsFromFile \"%s\" -all:all \"-xmp:subject=Dual-ISO\" \"-UniqueCameraModel<Model\" \"%s\" -overwrite_original -q", source, dest);
    int r = system(exif_cmd);
    if(r!=0)
    {
        printf("**WARNING** exiftool couldn't update DNG tag data\n");
    }
}

const char * get_camera_model(const char* filename)
{
    static char model[100];
    char exif_cmd[10000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -Model -b \"%s\"", filename);
    FILE* exif_file = popen(exif_cmd, "r");
    if(exif_file) 
    {
        if (fgets(model, sizeof(model), exif_file) == NULL)
            goto err;
        pclose(exif_file);
    }
    else
    {
        err:
        printf("**WARNING** Could not identify the camera from EXIF. Assuming 5D Mark III.\n");
        return "EOS 5D Mark III";
    }
    
    if (strncmp(model, "Canon ", 6) == 0)
    {
        return model + 6;
    }
    
    return model;
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

void set_white_level(const char* file, int level)
{
    char exif_cmd[1000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool \"%s\" -WhiteLevel=%d -overwrite_original -q", file, level);
    int r = system(exif_cmd);
    if(r!=0)
    {
        printf("**WARNING** exiftool couldn't update white level\n");
    }
}

static int verify_raw_embedding(const char* dng_file, const char* raw_file)
{
    int ans = 0;
    FILE *fr=0, *fd=0;
    char *bufr=0, *bufd=0;
    
    fr = fopen(raw_file, "rb");
    if (!fr) goto end;

    char exif_cmd[1000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool \"%s\" -OriginalRawFileData -b", dng_file);

    fd = popen(exif_cmd, "r");
    if (!fd) goto end;
    
    #ifdef _O_BINARY
    _setmode(_fileno(fd), _O_BINARY);
    #endif
    
    bufr = malloc(1024*1024);
    if (!bufr) goto end;

    bufd = malloc(1024*1024);
    if (!bufd) goto end;
    
    int total = 0;
    while (1)
    {
        int a = fread(bufr, 1, 1024*1024, fr);
        int b = fread(bufd, 1, 1024*1024, fd);
        
        if (a != b)
        {
            break;
        }
        
        if (memcmp(bufr, bufd, a))
        {
            break;
        }
        
        if (a <= 0 && total > 0)
        {
            printf("%-16s: verified %s (%d bytes)\n", dng_file, raw_file, total);
            ans = 1;
            break;
        }

        total += a;
    }

end:
    if (fr) fclose(fr);
    if (fd) fclose(fd);
    if (bufr) free(bufr);
    if (bufd) free(bufd);
    return ans;
}

void embed_original_raw(const char* dng_file, const char* raw_file, int delete_original)
{
    printf("%-16s: %s into %s\n", raw_file, delete_original ? "moving" : "copying", dng_file);
    char exif_cmd[1000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool \"%s\" \"-OriginalRawFileData<=%s\" \"-OriginalRawFileName=%s\" -overwrite_original -q", dng_file, raw_file, raw_file);
    int r = system(exif_cmd);
    if(r!=0)
    {
        printf("%-16s: could not embed original raw\n", dng_file);
        return;
    }

    if (!verify_raw_embedding(dng_file, raw_file))
    {
        printf("%-16s: verification of %s failed\n", dng_file, raw_file);
        return;
    }

    if (delete_original)
    {
        unlink(raw_file);
    }
}

int dng_has_original_raw(const char* dng_file)
{
    char exif_cmd[1000];

    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -OriginalRawFileData \"%s\"", dng_file);
    FILE* exif_file = popen(exif_cmd, "r");
    if (exif_file)
    {
        char ch;
        int has_original_raw = fscanf(exif_file, "%c", &ch) == 1;
        pclose(exif_file);
        return has_original_raw;
    }

    printf("%-16s: could not run exiftool\n", dng_file);
    return 0;
}

/* returns 1 on success */
int extract_original_raw(const char* dng_file, const char* raw_file)
{
    printf("%-16s: extracting from %s\n", raw_file, dng_file);

    char exif_cmd[1000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool \"%s\" -OriginalRawFileData -b > \"%s\"", dng_file, raw_file);
    int r = system(exif_cmd);
    if(r!=0)
    {
        printf("%-16s: could not extract original raw\n", dng_file);
        return 0;
    }
    return 1;
}

void dng_backup_metadata(const char* dng_file)
{
    printf("%-16s: backing up metadata\n", dng_file);
    
    /* only backup application-specific tags (exclude exif tags which are copied from CR2 or set from cr2hdr) */

    char exif_cmd[1000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -j -G -q -w json \"%s\" --EXIF:all --File:all --Exiftool:all --MakerNotes:all --Composite:all ", dng_file);
    int r = system(exif_cmd);
    if(r!=0)
    {
        printf("%-16s: could not backup metadata\n", dng_file);
    }
}

void dng_restore_metadata(const char* dng_file)
{
    printf("%-16s: restoring metadata\n", dng_file);

    char json_file[1000];
    int len = snprintf(json_file, sizeof(json_file), "%s", dng_file);
    json_file[len-3] = 'j';
    json_file[len-2] = 's';
    json_file[len-1] = 'o';
    json_file[len-0] = 'n';
    json_file[len+1] = '\0';
    
    char exif_cmd[1000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool \"-json=%s\" \"%s\" -q -overwrite_original", json_file, dng_file);
    int r = system(exif_cmd);
    if(r!=0)
    {
        printf("%-16s: could not restore metadata\n", dng_file);
        return;
    }
    unlink(json_file);
}
