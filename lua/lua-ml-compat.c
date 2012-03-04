#include "lua-ml-compat.h"


int do_fwrite(void* ptr, size_t size, size_t count, FILE* stream) {
  if (stream) {
    return FIO_WriteFile(stream, ptr, size * count);
  }
  return 0;
}

int do_fflush(FILE* stream) {
  return 0;
}

int do_fprintf(FILE* stream, const char * str, ...) {
  if (stream) {
    // should do format
    do_fwrite((void*)str, strlen(str)+1, 1, stream);
  }
  return 0;
};

int do_getc ( FILE * stream ) {
  if (stream) {
    char buf;
    int res = FIO_ReadFile(stream, &buf, 1);
    if (res) return res;
  }
  return EOF;
}

int do_feof( FILE * stream ) {
  return 0;
}

int do_fread(void* ptr, size_t size, size_t count, FILE* stream) {
  if (stream) {
    return FIO_ReadFile(stream, ptr, size*count);
  }
  return 0;
}

FILE* do_fopen(const char* filename, const char* mode) {
  if (strchr(mode,"r")) {
    return FIO_Open(filename,O_RDONLY | O_SYNC);
  } else {
    return FIO_Open(filename,0);
  }
}

FILE* do_freopen(const char * filename, const char* mode, FILE * stream) {
  return NULL;
}

int do_ferror(FILE * stream) {
  if (stream) return 0;
  return 1;
}

int do_fclose(FILE * stream) {
  FIO_CloseFile(stream);
  return 0;
}

char* do_fgets(char* str, int num, FILE * stream) {
  int i;
  int c;
  int done = 0;
  if (str == 0 || num <= 0 || stream == 0)
    return 0;
  for (i = 0; !done && i < num - 1; i++) {
    c = do_getc(stream);
    if (c == EOF) {
      done = 1;
      i--;
    } else {
      str[i] = c;
      if (c == '\n')
        done = 1;
    }
  }
  str[i] = '\0';
  if (i == 0)
    return 0;
  else
    return str;
}

char *strstr(const char *s1, const char *s2)
{
  size_t n = strlen(s2);
  while(*s1)
    if(!memcmp(s1++,s2,n))
      return s1-1;
  return 0;
}

char* strchr(const char* s, int c) {
  while (*s != (char)c)
    if (!*s++)
      return 0;
  return (char *)s;
}

char* strpbrk(const char* s1, const char* s2)
{
  while(*s1)
    if(strchr(s2, *s1++))
      return (char*)--s1;
  return 0;
}

void do_abort() {
}

char* do_getenv(char* name) {
  return NULL;
}

#define MAX_VSNPRINTF_SIZE 4096

int sprintf(char* str, const char* fmt, ...)
{
    int num;
    va_list            ap;

    va_start( ap, fmt );
    num = vsnprintf( str, MAX_VSNPRINTF_SIZE, fmt, ap );
    va_end( ap );
    return num;
}

int memcmp(const void* s1, const void* s2,size_t n)
{
  const unsigned char *p1 = s1, *p2 = s2;
  while(n--)
    if( *p1 != *p2 )
      return *p1 - *p2;
    else
      *p1++,*p2++;
  return 0;
}

int toupper(int c)
{
  if(('a' <= c) && (c <= 'z'))
    return 'A' + c - 'a';
  return c;
}

int tolower(int c)
{
  if(('A' <= c) && (c <= 'Z'))
    return 'a' + c - 'A';
  return c;
}

void *memchr(const void *s, int c, size_t n)
{
  unsigned char *p = (unsigned char*)s;
  while( n-- )
    if( *p != (unsigned char)c )
      p++;
    else
      return p;
  return 0;
}

size_t strspn(const char *s1, const char *s2)
{
  size_t ret=0;
  while(*s1 && strchr(s2,*s1++))
    ret++;
  return ret;    
}
