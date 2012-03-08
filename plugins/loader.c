#define PLUGIN_CLIENT
#include "plugin.h"

static struct os_command* commands;
static unsigned int num_of_commands;
static unsigned int base_addr;

int __init(struct os_command cmds[], unsigned int num_cmds, unsigned int base) {
	commands = cmds;
	num_of_commands = num_cmds;
	base_addr = base;
#ifdef PLUGIN_INIT
	return PLUGIN_INIT(cmds, num_cmds);
#else
	return load_std_functions();
#endif
}

void* get_function(unsigned int id) {
	struct os_command * cmd = commands;
	int i = num_of_commands;
	while (i) {
		if (cmd->id == id) return cmd->func;
		i--;
		cmd++;
	}
	return 0;
}

unsigned int get_base_ptr() {
	return base_addr;
}

void* fix_fptr(void* f) {
	return (void*)((char*)f+base_addr);
}

#define IMPORT_FUNC_R( v ) IMPORT_FUNC( v ); if (v) res++;

int load_std_functions() {
	int res = 0;
	// s/OS_FUNCTION(.\{-},.\{-},\t\(.\{-}\),.*/IMPORT_FUNC_R( \1 );/
	IMPORT_FUNC_R( msleep );
	IMPORT_FUNC_R( bmp_printf );
	IMPORT_FUNC_R( FIO_Open );
	IMPORT_FUNC_R( FIO_ReadFile );
	IMPORT_FUNC_R( console_vprintf );
	IMPORT_FUNC_R( console_puts );
	IMPORT_FUNC_R( FIO_WriteFile );
	IMPORT_FUNC_R( FIO_CloseFile );
	IMPORT_FUNC_R( strlen );
	IMPORT_FUNC_R( vsnprintf );
	IMPORT_FUNC_R( AllocateMemory );
	IMPORT_FUNC_R( FreeMemory );
	IMPORT_FUNC_R( my_memcpy );
	IMPORT_FUNC_R( strcmp );
	IMPORT_FUNC_R( abs );
	IMPORT_FUNC_R( strtol );
	IMPORT_FUNC_R( strcpy );
	return res;
}

