#ifndef __PLUGIN_H
#define __PLUGIN_H

#define MODULE_FUNC_INIT 0x11110000
#define MODULE_FUNC_DONE 0x22220000

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
#ifdef PLUGIN_C_FILE // only include this from the plugin.c file
#define OS_FUNCTION( fid, fret, fname, ... ) extern fret fname (__VA_ARGS__); REGISTER_PLUGIN_COMMAND( fid, fname );
#else
#define OS_FUNCTION( fid, fret, fname, ... ) extern fret fname (__VA_ARGS__);
#endif // PLUGIN_C_FILE
#endif // PLUGIN_CLIENT

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
#include <stdarg.h>
#include "all_headers.h"
#ifdef INIT_FUNC
#undef INIT_FUNC
#endif

#ifdef TASK_CREATE
#undef TASK_CREATE
#endif
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
	size_t data_rel_ro_local_start;
	size_t data_rel_ro_local_end;
	size_t data_rel_ro_start;
	size_t data_rel_ro_end;
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

struct plugin_descriptor {
	struct task_create* tasks;
};

#endif
