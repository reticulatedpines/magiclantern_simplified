#ifndef __scnprintf_h__
#define __scnprintf_h__

extern int scnprintf( char* str, size_t n, const char* fmt, ... );
extern int vscnprintf( char* str, size_t n, const char* fmt, va_list ap );

/* since Canon's vsnprintf returns the number of chars actually written,
 * we were using the return value of snprintf/vsnprintf
 * as described in https://lwn.net/Articles/69419/ 
 * disallow this usage; we really meant scnprintf/vscnprintf
 * note: if we don't care about return value, they are the same as snprintf/vsnprintf
 */
#define snprintf (void)scnprintf
#define vsnprintf (void)vscnprintf

#endif /* __scnprintf_h__ */
