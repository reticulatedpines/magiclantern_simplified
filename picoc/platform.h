/* all platform-specific includes and defines go in this file */
#ifndef PLATFORM_H
#define PLATFORM_H

#define  ML_HOST

#define LARGE_INT_POWER_OF_TEN 1000000000   /* the largest power of ten which fits in an int on this architecture */
#define ALIGN_TYPE void *                   /* the default data type to use for alignment */

#define GLOBAL_TABLE_SIZE 97                /* global variable table */
#define STRING_TABLE_SIZE 97                /* shared string table size */
#define STRING_LITERAL_TABLE_SIZE 97        /* string literal table size */
#define PARAMETER_MAX 16                    /* maximum number of parameters to a function */
#define LINEBUFFER_MAX 256                  /* maximum number of characters on a line */
#define LOCAL_TABLE_SIZE 11                 /* size of local variable table (can expand) */
#define STRUCT_TABLE_SIZE 11                /* size of struct/union member table (can expand) */

#define INTERACTIVE_PROMPT_START "starting picoc " PICOC_VERSION "\n"
#define INTERACTIVE_PROMPT_STATEMENT "picoc> "
#define INTERACTIVE_PROMPT_LINE "     > "

/* host platform includes */

// uses some extra 32k of RAM
#define NO_FP

#include <stdlib.h>
#include <string.h>
//#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>
#include <math.h>
#include "../src/dryos.h"
#include "../src/bmp.h"
#include "../src/lens.h"
#define BUILTIN_MINI_STDLIB
#define USE_MALLOC_STACK
#define NO_STRING_FUNCTIONS
#define NO_HASH_INCLUDE

#ifndef NO_FP
    #define PICOC_LIBRARY // sin, cos, pow
#endif

#define assert(x)
#define malloc AllocateMemory
#define NO_CALLOC
#define NO_REALLOC
#define free FreeMemory
#define memcpy my_memcpy
#undef BIG_ENDIAN
#define PicocPlatformSetExitPoint()

//~ #define DEBUG_LEXER
//~ #define DEBUG_HEAP
//~ #define DEBUG_EXPRESSIONS

extern int ExitBuf[];
#define G_FONT FONT_LARGE
#define E_X 0
#define E_Y 50

#endif /* PLATFORM_H */
