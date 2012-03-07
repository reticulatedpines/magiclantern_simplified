#define PLUGIN_CLIENT
#include "plugin.h"

static struct os_command* commands;
static unsigned int num_of_commands;

int __init(struct os_command cmds[], int num_cmds) {
	commands = cmds;
	num_of_commands = num_cmds;
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

#define IMPORT_FUNC_R( v ) IMPORT_FUNC( v ); if (v) res++;

int load_std_functions() {
	int res = 0;
	IMPORT_FUNC_R(msleep);
	IMPORT_FUNC_R(bmp_printf);
	return res;
}

