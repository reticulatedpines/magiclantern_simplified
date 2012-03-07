#ifndef __PLUGIN_H
#define __PLUGIN_H

// standard functions
#ifdef PLUGIN_CLIENT
#define OS_VOID_FUNCTION( fid, fname, ... ) enum { os_##fname = fid }; void (*fname) (__VA_ARGS__);
#define OS_INT_FUNCTION( fid, fname, ... ) enum { os_##fname = fid }; int (*fname) (__VA_ARGS__);
#define OS_STR_FUNCTION( fid, fname, ... ) enum { os_##fname = fid }; char* (*fname) (__VA_ARGS__);
#define IMPORT_FUNC( v ) v = get_function( os_##v )
#else
#include "dryos.h"
#include "bmp.h"
#define REGISTER_PLUGIN_COMMAND( fid, ffunc ) \
__attribute__((section(".plugin_commands"))) \
__attribute__((used)) \
static struct os_command _os_command_##fid##_block = { \
        .id             = fid, \
        .func           = (void*)ffunc, \
}
#define OS_VOID_FUNCTION( fid, fname, ... ) REGISTER_PLUGIN_COMMAND( fid, fname );
#define OS_INT_FUNCTION( fid, fname, ... ) REGISTER_PLUGIN_COMMAND( fid, fname );
#define OS_STR_FUNCTION( fid, fname, ... ) REGISTER_PLUGIN_COMMAND( fid, fname );
#endif

struct os_command {
	unsigned int	id;
	void*			func;
};

struct extern_function {
	unsigned int	id;
	void*			func;
	void*			name;
	void*			reserved;
};

#ifdef PLUGIN_CLIENT
#include "plugin_export.h"

#define REGISTERSTRUCT( fid, fname, ffunc ) \
__attribute__((section(".functions"))) \
__attribute__((used)) \
static struct extern_function _extern_function_##fid##_block = { \
		.name			= #fname, \
        .func           = ffunc, \
        .id             = fid, \
		.reserved		= 0, \
}

#define EXTERN_VOID_FUNC( id, name, ... ) \
void name(__VA_ARGS__); \
REGISTERSTRUCT( id, name, name ); \
void name(__VA_ARGS__)

#define EXTERN_STR_FUNC( id, name, ... ) \
char* name(__VA_ARGS__); \
REGISTERSTRUCT( id, name, name ); \
char* name(__VA_ARGS__)

#define EXTERN_INT_FUNC( id, name, ... ) \
int name(__VA_ARGS__); \
REGISTERSTRUCT( id, name, name ); \
int name(__VA_ARGS__)

extern void* get_function(unsigned int id);

#else

struct ext_plugin {
	size_t initreserved; // jump command
	size_t function_list_end;	// = (count(functions)+2)*4
	size_t ipltgot_start;
	size_t ipltgot_end;
	size_t got_start;
	size_t got_end;
	size_t pltgot_start;
	size_t pltgot_end;
	struct extern_function functions;
};

extern void* get_function(struct ext_plugin * plug, unsigned int id);
extern void* get_function_str(struct ext_plugin * plug, const char* str);
extern struct ext_plugin * load_plugin(const char* name);
extern void unload_plugin(struct ext_plugin * plug);

#endif

#endif
