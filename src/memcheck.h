#ifndef _memcheck_h_
#define _memcheck_h_

/* this file needs to get included in dryos.h to trace mallc/free and detect errors */

extern void *ml_debug_malloc( size_t len, const char *file, unsigned int line, int mode);
extern void *ml_debug_free( void * buf, int mode);
extern void *ml_debug_realloc( void * buf, size_t len, const char *file, unsigned int line );

#define malloc(len)         ml_debug_malloc(len,__FILE__,__LINE__,1)
#define free(buf)           ml_debug_free(buf,1)
#define realloc(buf,len)    ml_debug_realloc(buf,len,__FILE__,__LINE__)

#define AllocateMemory(len) ml_debug_malloc(len,__FILE__,__LINE__,0)
#define FreeMemory(buf)     ml_debug_free(buf,0)

#endif
