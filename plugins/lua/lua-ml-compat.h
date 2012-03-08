#ifndef __lua_ml_compat_h
#define __lua_ml_compat_h

#define PLUGIN_CLIENT
#include "plugin.h"

#define stderr ((FILE*)1)
#define stdout ((FILE*)2)
#define stdin NULL
#define EOF -1
#define BUFSIZ 256

#define TDEBUG(s) bmp_printf(FONT_LARGE, 100,100, s); msleep(100);

extern int do_fwrite(const void* ptr, size_t size, size_t count, FILE* stream);
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
extern char* do_getenv(const char* name);

extern int sprintf(char * str, const char * fmt, ...);
extern int memcmp(const void* s1, const void* s2,size_t n);
extern void *memchr(const void *s, int c, size_t n);
size_t strspn(const char *s1, const char *s2);

extern int tolower(int c);
extern int toupper(int c);

extern int islower(int x);
extern int isupper(int x);
extern int isalpha(int x);
extern int isdigit(int x);
extern int isxdigit(int x);
extern int isalnum(int x);
extern int ispunct(int x);
extern int isgraph(int x);
extern int isspace(int x);
extern int iscntrl(int x);

#define memcpy my_memcpy
#define strcoll strcmp

#endif // __lua_ml_compat_h
