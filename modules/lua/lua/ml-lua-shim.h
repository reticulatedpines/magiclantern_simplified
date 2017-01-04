
#ifndef ml_lua_shim_h
#define ml_lua_shim_h

//#include <dryos.h>
#include <console.h>
#include <string.h>

#define strcoll(a,b) strcmp(a,b)

int ftoa(char *s, float n);

#endif
