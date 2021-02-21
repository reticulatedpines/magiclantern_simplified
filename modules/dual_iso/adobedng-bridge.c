#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "adobedng-bridge.h"

static const char* search_path[] = {
    "./",                                                       /* working dir */
    ".\\",                                                      /* working dir (Windows) */
    "/Applications/Adobe DNG Converter.app/Contents/MacOS/",    /* Mac */
    "C:\\Program Files\\Adobe\\",                               /* Windows */
    "C:\\Program Files (x86)\\Adobe\\",                         /* Some 64-bit Windows */
};

#define COUNT(x)        ((int)(sizeof(x)/sizeof((x)[0])))

static int can_exec(const char *file)
{
    //~ printf("%s: %d\n", file, !access(file, X_OK));
    return !access(file, X_OK);
}

static const char* find_adobe_dng_converter()
{
    static char adobe_dng_path[1000] = "";
    
    /* valid path from previous attempt? just return it */
    if (adobe_dng_path[0])
        return adobe_dng_path;
    
    /* let's try some usual paths */
    int i;
    for (i = 0; i < COUNT(search_path); i++)
    {
        int is_win_path = (search_path[i][strlen(search_path[i])-1] == '\\');
        if (is_win_path)
        {
#if defined(WIN32) || defined(_WIN32)
            snprintf(adobe_dng_path, sizeof(adobe_dng_path), "%s%s", search_path[i], "Adobe DNG Converter.exe");
            if (can_exec(adobe_dng_path))
                return adobe_dng_path;
#else
            /* let's try Wine */
            snprintf(adobe_dng_path, sizeof(adobe_dng_path), "winepath \"%s%s\"", search_path[i], "Adobe DNG Converter.exe");
            FILE* f = popen(adobe_dng_path, "r");
            if (f)
            {
                int ok = fgets(adobe_dng_path, sizeof(adobe_dng_path), f) != 0;
                pclose(f);
                
                if (ok)
                {
                    strtok(adobe_dng_path, "\n");
                    if (can_exec(adobe_dng_path))
                        return adobe_dng_path;
                }
            }
#endif
        }
        else
        {
#if !defined(WIN32) && !defined(_WIN32)
            snprintf(adobe_dng_path, sizeof(adobe_dng_path), "%s%s", search_path[i], "Adobe DNG Converter");
            if (can_exec(adobe_dng_path))
                return adobe_dng_path;
#endif
        }
    }
    
    /* nothing found */
    adobe_dng_path[0] = 0;
    return 0;
}

void dng_compress(const char* source, int lossy)
{
    const char * adobe_dng = find_adobe_dng_converter();
    if (adobe_dng)
    {
        printf("Found %s\n", adobe_dng);
    }
    else
    {
        printf("Adobe DNG Converter not found.\n");
        return;
    }
    
    printf("Compressing %s...\n", source);
    
    /* Adobe DNG does not overwrite files, we need to trick it somehow */
    char tmp[1000];
    snprintf(tmp, sizeof(tmp), "%s.DNG", source);
    rename(source, tmp);
    
    /* Find filename part only, '-o' parameter to DNG converter does not accept path */
    const char *src_filename = strrchr(source, '/');
    if (!src_filename)
        src_filename =  strrchr(source, '\\');
    if (!src_filename)
        src_filename = source;
    else
        src_filename++;

    char compress_cmd[1000];
    char* start =
#if defined(WIN32) || defined(_WIN32)
    "start \"\" /wait "; /* force Windows to wait for the command to complete */
#else
    "";
#endif
    snprintf(compress_cmd, sizeof(compress_cmd), "%s\"%s\" -dng1.4 %s -o \"%s\" \"%s\"", start, adobe_dng, lossy ? "-lossy" : "", src_filename, tmp);
    printf("%s\n", compress_cmd);
    int r = system(compress_cmd);
    if(r!=0)
    {
        /* phuck; restore the old file */
        printf("Adobe DNG Converter returned error(%d).\n", r);
        rename(tmp, source);
    }
    else
    {
        /* looks OK, delete the temporary file */
        unlink(tmp);
    }
}
