#ifndef __PLUGIN_H
#define __PLUGIN_H

// some defines that we'll probably need during plugin writing
#ifdef PLUGIN_CLIENT
#include <stdarg.h>
#include "arm-mcr.h"

typedef void FILE;
#endif

// standard functions
#ifdef PLUGIN_CLIENT
#define OS_FUNCTION( fid, fret, fname, ... ) enum { os_##fname = fid }; fret (*fname) (__VA_ARGS__);
#define IMPORT_FUNC( v ) v = get_function( os_##v )
#else
#define REGISTER_PLUGIN_COMMAND( fid, ffunc ) \
__attribute__((section(".plugin_commands"))) \
__attribute__((used)) \
static struct os_command _os_command_##fid##_block = { \
        .id             = fid, \
        .func           = (void*)ffunc, \
}
#define OS_FUNCTION( fid, rret, fname, ... ) REGISTER_PLUGIN_COMMAND( fid, fname );
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

#define EXTERN_FUNC( id, rval, name, ... ) \
rval name(__VA_ARGS__); \
REGISTERSTRUCT( id, name, name ); \
rval name(__VA_ARGS__)

extern void* get_function(unsigned int id);
extern unsigned int get_base_ptr();
extern void* fix_ptr(void* f);

#else

struct ext_plugin {
	size_t initreserved; // jump command
	size_t function_list_end;	// = (count(functions)+2)*4
	size_t data_rel_local_start;
	size_t data_rel_local_end;
	size_t data_rel_start;
	size_t data_rel_end;
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
