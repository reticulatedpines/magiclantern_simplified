#ifndef __lua_ml_compat_h
#define __lua_ml_compat_h

#include <stdarg.h>
#include "dryos.h"

#define stderr NULL
#define stdin NULL
#define stdout NULL
#define EOF -1
#define BUFSIZ 256

extern void* AllocateMemory(size_t);
extern void FreeMemory(void*);
extern void my_memcpy(void*,void*,size_t);

extern int do_fwrite(void* ptr, size_t size, size_t count, FILE* stream);
extern int do_fread(void* ptr, size_t size, size_t count, FILE* stream);
extern int do_fflush(FILE* stream);
extern int do_fprintf(FILE* stream, const char * str, ...);
extern int do_getc(FILE * stream);
extern int do_feof(FILE * stream);
extern char* do_fgets(char* str, int num, FILE * stream);
extern FILE* do_fopen(const char* filename, const char* mode);
extern FILE* do_freopen(const char * filename, const char* mode, FILE * stream);
extern int do_ferror(FILE * stream);
extern int do_fclose(FILE * stream);

extern char* strstr(const char* str1, const char* str2);
extern char* strpbrk(const char* str1, const char* str2);
extern char* strchr(const char* str, int c);

extern void do_abort();
extern char* do_getenv(char* name);

extern int sprintf(char * str, const char * fmt, ...);
extern int memcmp(const void* s1, const void* s2,size_t n);
extern void *memchr(const void *s, int c, size_t n);
size_t strspn(const char *s1, const char *s2);

extern int tolower(int c);
extern int toupper(int c);

#define islower(x) (((x)>='a') && ((x)<='z'))
#define isupper(x) (((x)>='A') && ((x)<='Z'))
#define isalpha(x) (islower(x) || isupper(x))
#define isdigit(x) (((x)>='0') && ((x)<='9'))
#define isxdigit(x) (isdigit(x) || (((x)>='A') && ((x)<='F')) || (((x)>='a') && ((x)<='f')))
#define isalnum(x) (isalpha(x) || isdigit(x))
#define ispunct(x) (strchr("!\"#%&'();<=>?[\\]*+,-./:^_{|}~",x)!=0)
#define isgraph(x) (ispunct(x) || isalnum(x))
#define isspace(x) (strchr(" \r\n\t",x)!=0)
#define iscntrl(x) (strchr("\x07\x08\r\n\x0C\x0B\x09",x)!=0)

#define strcoll strcmp

#endif // __lua_ml_compat_h
