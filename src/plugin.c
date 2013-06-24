#define PLUGIN_C_FILE // so plugin.h will include all exported function definitions

#include "compiler.h"
#include "plugin.h"
#include "all_headers.h"
#include "menu.h"

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

struct loaded_plugin {
	struct	ext_plugin* plug;
	char*				name;
	int					need_init;
	int					has_init;
};

static struct loaded_plugin* plugins;
static size_t plugins_count = 0;

#define Allocator SmallAlloc
#define DeAllocator SmallFree

struct ext_plugin * load_plugin(const char* filename) {
	unsigned size;
	unsigned char* buf;
	unsigned char* retval;
	unsigned int i;
	struct ext_plugin * plug;
	unsigned int* got_start;
	unsigned int* got_end;
	unsigned int* got;
	int (*__init)(struct os_command*,int num_cmds, int base_addr);
	int numval;
	unsigned int text_start;

    if( FIO_GetFileSize( filename, &size ) != 0 )
    {
		console_printf("file size fail: %s\n", filename);
        goto getfilesize_fail;
	}

	buf = alloc_dma_memory(size);
	if (!buf)
	{
		console_printf("malloc fail: %d\n", size);
		goto malloc_fail;
	}

	if ((unsigned)read_file(filename, buf, size)!=size)
	{
		console_printf("read fail: %d\n", size);
		goto read_fail;
	}

	retval = Allocator(size);
	if (!retval)
	{
		console_printf("malloc2 fail: %d\n", size);
		goto copy_fail;
	}
	msleep(1);
	memcpy(retval, buf, size);
	free_dma_memory(buf);

	plug = (struct ext_plugin*)retval;

	text_start = plug->function_list_end;
	if (text_start % 1024) {
		text_start+=1024 - text_start%1024;
	}

	// poor man's linker step 1: fix GOT table
	got_start = (unsigned int*)(((char*)plug)+plug->got_start);
	got_end = (unsigned int*)(((char*)plug)+plug->got_end);
	got = got_start;
	while (got < got_end) {
		if (*got <= size) { // if it's larger it's probably an absolute address
			if (*got >= text_start) { // if it is smaller it's probably a constant, or NULL
				*got += (unsigned int)plug;
			}
		}
		got++;
	}
	// poor man's linker step 2: relocate symbols
	// relocation information is put at the end of the bin file, by symtblgen.rb
	got = (unsigned int*)((char*)plug+size);
	got--;
	// the last entry is the size of the relocation table
	i = *got;
	while (i) {
		i--;
		got--;
		// do not relocate locations outside of the valid area:
		if (*got <= size) { // if it's larger it's probably an absolute address
			if (*got >= text_start) { // if it is smaller it's probably a constant, or NULL
				got_start = (unsigned int*)(((char*)plug)+(*got));
				// also do not relocate symbols outside of the valid area:
				if (*got_start <= size) { // if it's larger it's probably an absolute address
					if (*got_start >= text_start) { // if it is smaller it's probably a constant, or NULL
						// all relocations produced by gcc are REL_ARM_ABS32 type
						// (or use a GOT table, but  that was already covered)
						*got_start += (unsigned int)plug;
					}
				}
			}
		}
	}

	msleep(100); // crashes without this. minimum wait amount not tested yet

	// now it's fixed try to run it's init
	__init = (void*)plug;
	numval = __init(_plugin_commands_start,_plugin_commands_end - _plugin_commands_start,(int)plug);

	console_printf("Available commands in OS: %d. Loaded: %d\n", _plugin_commands_end - _plugin_commands_start, numval);

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
    DebugMsg( DM_MAGIC, 3, "plugin load failed (%s)", filename);
    return 0;
}

void unload_plugin(struct ext_plugin * plug) {
	if (plug) DeAllocator(plug);
}

const char* CDRV = CARD_DRIVE;

const char* get_card_drive() {
	return CDRV;
}

void menu_open_submenu();

static struct menu_entry plugin_menus[] = {
	{
		.name = "Configure plugins",
		.select = menu_open_submenu,
	},
};

int is_valid_plugin_filename(char* filename)
{
    int n = strlen(filename);
    if ((n > 4) && (streq(filename + n - 4, ".BIN") || streq(filename + n - 4, ".bin")) && (filename[0] != '.') && (filename[0] != '_'))
        return 1;
    return 0;
}

static void select_plugins_submenu(void* priv, int delta)
{
	struct loaded_plugin * plug = (struct loaded_plugin*)priv;
	if (!plug->plug) {
		char filename[50];
		snprintf(filename, sizeof(filename), CARD_DRIVE"ML/PLUGINS/%s", plug->name);
		plug->plug = load_plugin(filename);
	}
	if (plug->plug) {
		if (!plug->has_init) {
			plug->need_init = 1;
		}
	} else {
		console_printf("Plugin load failed!");
	}
}

static void display_plugins_submenu(void* priv, int x, int y, int selected)
{
	struct loaded_plugin * plug = (struct loaded_plugin*)priv;
	bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT, x, y, "%s", plug->name);
	menu_draw_icon(x, y, plug->need_init ? MNI_WARNING : plug->has_init ? MNI_ON : MNI_OFF, (intptr_t)"");
}

static void find_plugins() {
	struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( CARD_DRIVE "ML/PLUGINS/", &file );
    if( IS_ERROR(dirent) )
    {
        // no need to worry - if they are not present, don't use them
        //~ NotifyBox(2000, "PLUGINS dir missing" );
        //~ msleep(100);
        //~ NotifyBox(2000, "Please copy all ML files!" );
        return;
    }
    int k = 0;
    do {
        if (is_valid_plugin_filename(file.name))
        {
            k++;
        }
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
	if (k>0) {
		struct menu_entry * entries = AllocateMemory(sizeof(struct menu_entry)*(k+1));
		plugins = AllocateMemory(sizeof(struct loaded_plugin)*k);
		memset(entries, 0, sizeof(struct menu_entry)*(k+1));
		memset(plugins, 0, sizeof(struct loaded_plugin)*k);
		dirent = FIO_FindFirstEx( CARD_DRIVE "ML/PLUGINS/", &file );
		k = 0;
	    do {
			if (is_valid_plugin_filename(file.name))
			{
				entries[k].name = AllocateMemory(strlen(file.name)+1);
				snprintf((char*)entries[k].name, strlen(file.name)+1, "%s", file.name);
				entries[k].display = display_plugins_submenu;
				entries[k].select = select_plugins_submenu;
				entries[k].priv = &plugins[k];
				plugins[k].name = (char*)entries[k].name;
				k++;
			}
	    } while( FIO_FindNextEx( dirent, &file ) == 0);
		entries[k].priv = MENU_EOL_PRIV;
		plugin_menus[0].children = entries;
		plugins_count = k;
	}
}

static void plugins_task(void* unused) {
	if (!plugins_count) return; // no plugins loaded, nothing to do
	msleep(1000);
	for (;;) {
		msleep(250);
		if (plugins_count) {
			size_t k;
			for (k=0; k< plugins_count; k++) {
				if (plugins[k].plug && plugins[k].need_init) {
					struct plugin_descriptor *(*__init)(void) = get_function(plugins[k].plug, MODULE_FUNC_INIT);
					if (__init) {
						struct plugin_descriptor* pld = __init();
						if (pld) {
							struct task_create * task = pld->tasks;
							while (task && task->name) {
								if (!task->priority) {
									console_printf("Starting init task: %08x, %s\n",(char*)task->entry-(char*)plugins[k].plug, task->name);
									thunk entry = (thunk)task->entry;
									entry();
								} else {
									console_printf("Starting task: %08x, PRIO: %d, STACK: %d, %s\n",(char*)task->entry-(char*)plugins[k].plug, task->priority, task->stack_size, task->name);
									task_create(task->name, task->priority, task->stack_size, task->entry, task->arg);
								}
								task++;
							}
							plugins[k].has_init = true;
						}
					}
					plugins[k].need_init = 0;
				}
			}
		}
	}
}

static void plugins_init(void* unused) {
	find_plugins();
	if (plugins_count)
		menu_add("Debug", plugin_menus, COUNT(plugin_menus));
}

INIT_FUNC( __FILE__, plugins_init);
TASK_CREATE( "plugins_task", plugins_task, 0, 0x1f, 0x1000 );
