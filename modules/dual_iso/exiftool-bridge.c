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

