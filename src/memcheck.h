#ifndef _memcheck_h_
#define _memcheck_h_

/* this file needs to get included in dryos.h to trace mallc/free and detect errors */

extern void *memcheck_malloc( size_t len, const char *file, unsigned int line, int mode);
extern void *memcheck_free( void * buf, int mode);
//extern void *memcheck_realloc( void * buf, size_t len, const char *file, unsigned int line );

#define malloc(len)         memcheck_malloc(len,__FILE__,__LINE__,1)
#define free(buf)           memcheck_free(buf,1)
//#define realloc(buf,len)    ml_debug_realloc(buf,len,__FILE__,__LINE__)

#define AllocateMemory(len) memcheck_malloc(len,__FILE__,__LINE__,0)
#define FreeMemory(buf)     memcheck_free(buf,0)

#endif
