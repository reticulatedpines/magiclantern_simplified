#include "dryos.h"
#include "plugin.h"
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

struct ext_plugin * load_plugin(const char* filename) {
	unsigned size;
	unsigned char* buf;
	unsigned char* retval;
    if( FIO_GetFileSize( filename, &size ) != 0 )
        goto getfilesize_fail;

	buf = alloc_dma_memory(size);
	if (!buf)
		goto malloc_fail;

	if ((unsigned)read_file(filename, buf, size)!=size)
		goto read_fail;

	retval = AllocateMemory(size);
	if (!retval)
		goto copy_fail;
	msleep(1);
	memcpy(retval, buf, size);
	free_dma_memory(buf);

	// poor man's linker: fix GOT table
	struct ext_plugin * plug = (struct ext_plugin*)retval;
	int* got_start = ((char*)plug)+plug->got_start;
	int* got_end = ((char*)plug)+plug->got_end;
	int* got = got_start;
	while (got < got_end) {
		if (*got < size) { // if it's larger it's probably an absolute address
			*got += (int)plug;
		}
		got++;
	}
	msleep(100); // crashes without this. minimum wait amount not tested yet

	// now it's fixed try to run it's init
	int (*__init)(struct os_command*,int num_cmds) = (void*)plug;
	int numval = __init(_plugin_commands_start,_plugin_commands_end - _plugin_commands_start);

	if (numval<=0) {
		FreeMemory(retval);
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
	if (plug) FreeMemory(plug);
}
