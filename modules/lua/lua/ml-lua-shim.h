
#ifndef ml_lua_shim_h
#define ml_lua_shim_h

//#include <dryos.h>
#include <console.h>
#include <string.h>

/* rename realloc because there is a core function called realloc,
 * which we want to call, but only sometimes (not always) */ 
#define realloc my_realloc
void* my_realloc(void* ptr, size_t size);

#define strcoll(a,b) strcmp(a,b)

int ftoa(char *s, float n);

#endif
