#include "picoc.h"
#include "interpreter.h"

static char* SourceStr = NULL;
/* deallocate any storage */
void PlatformCleanup()
{
    if (SourceStr != NULL)
    {
        script_free_dma(SourceStr);
        SourceStr = NULL;
    }
}

/* get a line of interactive input */
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt)
{
    // XXX - unimplemented so far
    return NULL;
}

/* get a character of interactive input */
int PlatformGetCharacter()
{
    // XXX - unimplemented so far
    return 0;
}

/* write a character to the console */
void PlatformPutc(unsigned char OutCh, union OutputStreamInfo *Stream)
{
    int c = OutCh;
    console_puts(&c);
}

/* read a file into memory */
char *PlatformReadFile(const char *FileName)
{
    int size;
    char* f = (char*)read_entire_file(FileName, &size);
    if (!f) console_printf("Error loading '%s'\n", FileName);
    return f;
}

/* read and scan a file for definitions */
EXTERN void PicocPlatformScanFile(const char *FileName)
{
    SourceStr = PlatformReadFile(FileName);
    if (!SourceStr) return;

    console_printf("%s:\n", FileName);
    script_define_param_variables();
    console_puts(  "\n");
    
    script_msleep(100);
    PicocParse(FileName, SourceStr, strlen(SourceStr), TRUE, TRUE, FALSE);
}

/* exit the program */
void __attribute__((noreturn)) PlatformExit(int RetVal)
{
    script_exit(RetVal); // in picoc.c
}

void abort()
{
    PlatformExit(-1);
}
