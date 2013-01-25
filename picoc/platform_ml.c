#include "picoc.h"
#include "interpreter.h"

/* deallocate any storage */
void PlatformCleanup()
{
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
void PicocPlatformScanFile(const char *FileName)
{
    char *SourceStr = PlatformReadFile(FileName);
    if (!SourceStr) return;

    console_printf("%s:\n"
                   "==============================\n", 
                   FileName);
    console_puts(SourceStr);
    console_puts(  "==============================\n\n");
    msleep(500);
    PicocParse(FileName, SourceStr, strlen(SourceStr), TRUE, FALSE, TRUE);

    free_dma_memory(SourceStr);
}

/* mark where to end the program for platforms which require this */
#include "setjmp.h"
jmp_buf PicocExitBuf;

/* exit the program */
void __attribute__((noreturn)) PlatformExit(int RetVal)
{
    PicocExitValue = RetVal;
    longjmp(PicocExitBuf, 1);
}

void abort()
{
    PlatformExit(-1);
}
