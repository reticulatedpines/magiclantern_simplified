#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "exiftool-bridge.h"

#define DEFAULT_MODEL_ID 0x285

void copy_tags_from_source(const char* source, const char* dest)
{
    char exif_cmd[1000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -tagsFromFile \"%s\" -all:all \"-UniqueCameraModel<Model\" \"%s\" -overwrite_original", source, dest);
    int r = system(exif_cmd);
    if(r!=0)
    {
        printf("**WARNING** exiftool couldn't overwrite DNG tag data\n");
    }
}

unsigned int get_model_id(const char* filename)
{
    char out_filename[] = "model.txt";
    unsigned tag = DEFAULT_MODEL_ID;
    char exif_cmd[10000];
    snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -CanonModelID -b \"%s\"> \"%s\"", filename, out_filename);
    int r = system(exif_cmd);
    if(r==0) 
    {
        FILE* exif_file = fopen(out_filename, "rb");
        fscanf(exif_file, "%u", &tag);
        fclose(exif_file);
        unlink(out_filename);
    }
    else
    {
        printf("**WARNING** exiftool didn't work, returing a default MODEL_ID of 0x%x\n", tag);
    }
    return tag & 0xFFF;
}

