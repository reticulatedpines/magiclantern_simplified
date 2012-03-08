#include "dryos.h"
#include "plugin.h"

// includes for exporting functions
#include "bmp.h"
extern int console_printf(const char* fmt, ...);
extern void console_puts(const char* str);
extern int console_vprintf(const char* fmt, va_list ap);
extern void	my_memcpy( void* dst, const void* src, size_t size );
extern int strcmp( const char* s1, const char* s2 );
extern int abs( int number );
extern int strtol( const char * str, char ** endptr, int base );
#include "plugin_export.h"

void* get_function(struct ext_plugin * plug, unsigned int id) {
	struct extern_function * f = &plug->functions;
	while ((char*)f < (char*)plug + plug->function_list_end) {
		if (f->id == id) return (void*)((char*)plug+(int)f->func);
		f++;
	}
	return 0;
}

void* get_function_str(struct ext_plugin * plug, const char* str) {
	struct extern_function * f = &plug->functions;
	while ((char*)f < (char*)plug + plug->function_list_end) {
		if (!strcmp(str,(char*)plug+(int)f->name)) return (void*)((char*)plug+(int)f->func);
		f++;
	}
	return 0;
}

extern struct os_command _plugin_commands_start[];
extern struct os_command _plugin_commands_end[];

#if defined CONFIG_60D
#define Allocator malloc
#define DeAllocator free
#else
#define Allocator AllocateMemory
#define DeAllocator FreeMemory
#endif

struct ext_plugin * load_plugin(const char* filename) {
	unsigned size;
	unsigned char* buf;
	unsigned char* retval;
	int i;
	struct ext_plugin * plug;
	int* got_start;
	int* got_end;
	int* got;
	int (*__init)(struct os_command*,int num_cmds, int base_addr);
	int numval;

    if( FIO_GetFileSize( filename, &size ) != 0 )
        goto getfilesize_fail;

	buf = alloc_dma_memory(size);
	if (!buf)
		goto malloc_fail;

	if ((unsigned)read_file(filename, buf, size)!=size)
		goto read_fail;

	retval = Allocator(size);
	if (!retval)
		goto copy_fail;
	msleep(1);
	memcpy(retval, buf, size);
	free_dma_memory(buf);

	plug = (struct ext_plugin*)retval;

	// poor man's linker: fix GOT and other reloc tables
	for (i=0; i<3; i++) {
		switch (i) {
			case 0: got_start = ((char*)plug)+plug->data_rel_local_start; got_end = ((char*)plug)+plug->data_rel_local_end; break;
			case 1: got_start = ((char*)plug)+plug->data_rel_start; got_end = ((char*)plug)+plug->data_rel_end; break;
			case 2: got_start = ((char*)plug)+plug->got_start; got_end = ((char*)plug)+plug->got_end; break;
		}
		got = got_start;
		while (got < got_end) {
			if (*got < size) { // if it's larger it's probably an absolute address
				*got += (int)plug;
			}
			got++;
		}
	}
	msleep(100); // crashes without this. minimum wait amount not tested yet

	// now it's fixed try to run it's init
	__init = (void*)plug;
	numval = __init(_plugin_commands_start,_plugin_commands_end - _plugin_commands_start,(int)plug);

	console_printf("Loaded %d commands from OS\n", numval);

	if (numval<=0) {
		DeAllocator(retval);
		return 0;
	} else {
		return plug;
	}
copy_fail:
read_fail:
	free_dma_memory(buf);
malloc_fail:
getfilesize_fail:
    DebugMsg( DM_MAGIC, 3, "plugin load failed");
    return 0;
}

void unload_plugin(struct ext_plugin * plug) {
	if (plug) DeAllocator(plug);
}
