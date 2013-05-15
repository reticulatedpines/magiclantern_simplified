
#include "dryos.h"
#include "menu.h"
#include "console.h"
#include "libtcc.h"
#include "module.h"
#include "config.h"
#include "string.h"
#include "property.h"

extern int sscanf(const char *str, const char *format, ...);


/* this must be public as it is used by modules */
char *module_card_drive = CARD_DRIVE;

static module_entry_t module_list[MODULE_COUNT_MAX];
static TCCState *module_state = NULL;

static struct menu_entry module_submenu[];
static struct menu_entry module_menu[];

CONFIG_INT("module.autoload", module_autoload_enabled, 0);
CONFIG_INT("module.console", module_console_enabled, 0);

static struct msg_queue * module_mq = 0;
#define MSG_MODULE_LOAD_ALL 1
#define MSG_MODULE_UNLOAD_ALL 2

void module_load_all(void)
{ 
    msg_queue_post(module_mq, MSG_MODULE_LOAD_ALL); 
}
void module_unload_all(void)
{
    msg_queue_post(module_mq, MSG_MODULE_UNLOAD_ALL); 
}

static void _module_load_all(void);
static void _module_unload_all(void);

static int module_load_symbols(TCCState *s, char *filename)
{
    uint32_t size = 0;
    FILE* file = NULL;
    char *buf = NULL;
    uint32_t count = 0;
    uint32_t pos = 0;

    if( FIO_GetFileSize( filename, &size ) != 0 )
    {
        console_printf("Error loading '%s': File does not exist\n", filename);
        return -1;
    }
    buf = alloc_dma_memory(size);
    if(!buf)
    {
        console_printf("Error loading '%s': File too large\n", filename);
        return -1;
    }

    file = FIO_Open(filename, O_RDONLY | O_SYNC);
    if(!file)
    {
        console_printf("Error loading '%s': File does not exist\n", filename);
        free_dma_memory(buf);
        return -1;
    }
    FIO_ReadFile(file, buf, size);
    FIO_CloseFile(file);

    while(buf[pos])
    {
        char address_buf[16];
        char symbol_buf[128];
        uint32_t length = 0;
        uint32_t address = 0;

        while(buf[pos + length] && buf[pos + length] != ' ' && length < sizeof(address_buf))
        {
            address_buf[length] = buf[pos + length];
            length++;
        }
        address_buf[length] = '\000';

        pos += length + 1;
        length = 0;

        while(buf[pos + length] && buf[pos + length] != '\r' && buf[pos + length] != '\n' && length < sizeof(symbol_buf))
        {
            symbol_buf[length] = buf[pos + length];
            length++;
        }
        symbol_buf[length] = '\000';

        pos += length + 1;

        while(buf[pos + length] && (buf[pos + length] == '\r' || buf[pos + length] == '\n'))
        {
            pos++;
        }
        sscanf(address_buf, "%x", &address);

        tcc_add_symbol(s, symbol_buf, (void*)address);
        count++;
    }
    //console_printf("Added %d Magic Lantern symbols\n", count);


    /* these are just to make the code compile */
    void longjmp();
    void setjmp();

    /* ToDo: parse the old plugin sections as all needed OS stubs are already described there */
    tcc_add_symbol(s, "msleep", &msleep);
    tcc_add_symbol(s, "longjmp", &longjmp);
    tcc_add_symbol(s, "strcpy", &strcpy);
    tcc_add_symbol(s, "setjmp", &setjmp);
    tcc_add_symbol(s, "alloc_dma_memory", &alloc_dma_memory);
    tcc_add_symbol(s, "free_dma_memory", &free_dma_memory);
    tcc_add_symbol(s, "vsnprintf", &vsnprintf);
    tcc_add_symbol(s, "strlen", &strlen);
    tcc_add_symbol(s, "memcpy", &memcpy);
    tcc_add_symbol(s, "console_printf", &console_printf);
    tcc_add_symbol(s, "task_create", &task_create);

    free_dma_memory(buf);
    return 0;
}

/* this is not perfect, as .Mo and .mO aren't detected. important? */
static int module_valid_filename(char* filename)
{
    int n = strlen(filename);
    if ((n > 3) && (streq(filename + n - 3, ".MO") || streq(filename + n - 3, ".mo")) && (filename[0] != '.') && (filename[0] != '_'))
        return 1;
    return 0;
}

static void _module_load_all(void)
{
    TCCState *state = NULL;
    uint32_t module_cnt = 0;
    struct fio_file file;
    uint32_t update_properties = 0;

    /* ensure all modules are unloaded */
    console_printf("Unloading modules...\n");
    _module_unload_all();

    /* initialize linker */
    state = tcc_new();
    tcc_set_options(state, "-nostdlib");
    if(module_load_symbols(state, MAGIC_SYMBOLS) < 0)
    {
        NotifyBox(2000, "Missing symbol file: " MAGIC_SYMBOLS );
        tcc_delete(state);
        return;
    }

    console_printf("Scanning modules...\n");
    struct fio_dirent * dirent = FIO_FindFirstEx( MODULE_PATH, &file );
    if( IS_ERROR(dirent) )
    {
        NotifyBox(2000, "Module dir missing" );
        tcc_delete(state);
        return;
    }

    do
    {
        if (file.mode & ATTR_DIRECTORY) continue; // is a directory
        if (module_valid_filename(file.name))
        {
            char module_name[MODULE_FILENAME_LENGTH];

            /* get filename, remove extension and append _init to get the init symbol */

            /* ensure the buffer is null terminated */
            memset(module_name, 0x00, sizeof(module_name));
            strncpy(module_name, file.name, MODULE_NAME_LENGTH);
            strncpy(module_list[module_cnt].filename, file.name, MODULE_FILENAME_LENGTH);

            uint32_t pos = 0;
            while(module_name[pos])
            {
                /* extension starting? terminate string */
                if(module_name[pos] == '.')
                {
                    module_name[pos] = '\000';
                    break;
                }
                else if(module_name[pos] >= 'A' && module_name[pos] <= 'Z')
                {
                    /* make lowercase */
                    module_name[pos] |= 0x20;
                }
                pos++;
            }
            strncpy(module_list[module_cnt].name, module_name, sizeof(module_list[module_cnt].name));
            snprintf(module_list[module_cnt].status, sizeof(module_list[module_cnt].status), "???");
            snprintf(module_list[module_cnt].long_status, sizeof(module_list[module_cnt].long_status), "Seems linking failed. Unknown symbols?");

            console_printf("  [i] found: %s\n", file.name);
            module_cnt++;
            if (module_cnt >= MODULE_COUNT_MAX)
            {
                NotifyBox(2000, "Too many modules" );
                break;
            }
        }
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);

    /* load modules */
    console_printf("Load modules...\n");
    for (uint32_t mod = 0; mod < module_cnt; mod++)
    {
        console_printf("  [i] load: %s\n", module_list[mod].filename);
        snprintf(module_list[mod].long_filename, sizeof(module_list[mod].long_filename), "%s%s", MODULE_PATH, module_list[mod].filename);
        int32_t ret = tcc_add_file(state, module_list[mod].long_filename);

        /* seems bad, disable it */
        if(ret < 0)
        {
            module_list[mod].valid = 0;
            snprintf(module_list[mod].status, sizeof(module_list[mod].status), "FileErr");
            snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Load failed: %s, ret 0x%02X");
            console_printf("  [E] %s\n", module_list[mod].long_status);
        }
    }

    console_printf("Linking..\n");
    int32_t ret = tcc_relocate(state, TCC_RELOCATE_AUTO);
    if(ret < 0)
    {
        console_printf("  [E] failed to link modules\n");
        tcc_delete(state);
        return;
    }

    console_printf("Init modules...\n");
    /* init modules */
    for (uint32_t mod = 0; mod < module_cnt; mod++)
    {
        if(!module_list[mod].valid)
        {
            char module_info_name[32];
            console_printf("  [i] Init: '%s'\n", module_list[mod].name);

            /* now check for info structure */
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_INFO_PREFIX), module_list[mod].name);
            module_list[mod].info = tcc_get_symbol(state, module_info_name);
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_STRINGS_PREFIX), module_list[mod].name);
            module_list[mod].strings = tcc_get_symbol(state, module_info_name);
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_PARAMS_PREFIX), module_list[mod].name);
            module_list[mod].params = tcc_get_symbol(state, module_info_name);
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_PROPHANDLERS_PREFIX), module_list[mod].name);
            module_list[mod].prop_handlers = tcc_get_symbol(state, module_info_name);
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_CBR_PREFIX), module_list[mod].name);
            module_list[mod].cbr = tcc_get_symbol(state, module_info_name);

            /* check if the module symbol is defined. simple check for valid memory address just in case. */
            if((uint32_t)module_list[mod].info > 0x1000)
            {
                if(module_list[mod].info->api_magic == MODULE_MAGIC)
                {
                    if((module_list[mod].info->api_major == MODULE_MAJOR) && (module_list[mod].info->api_minor <= MODULE_MINOR))
                    {
                        module_list[mod].valid = 1;

                        console_printf("  [i] info    at: 0x%08X\n", (uint32_t)module_list[mod].info);
                        console_printf("  [i] strings at: 0x%08X\n", (uint32_t)module_list[mod].strings);
                        console_printf("  [i] params  at: 0x%08X\n", (uint32_t)module_list[mod].params);
                        console_printf("  [i] props   at: 0x%08X\n", (uint32_t)module_list[mod].prop_handlers);
                        console_printf("  [i] cbr     at: 0x%08X\n", (uint32_t)module_list[mod].cbr);
                        console_printf("-----------------------------\n");
                        if(module_list[mod].info->init)
                        {
                            module_list[mod].info->init();
                        }
                        if(module_list[mod].prop_handlers)
                        {
                            module_prophandler_t **props = module_list[mod].prop_handlers;
                            while(*props != NULL)
                            {
                                update_properties = 1;
                                console_printf("  [i] prop %s\n", (*props)->name);
                                prop_add_handler((*props)->property, (*props)->handler);
                                props++;
                            }
                        }
                        console_printf("-----------------------------\n");
                        snprintf(module_list[mod].status, sizeof(module_list[mod].status), "OK");
                        snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Module loaded successfully");
                    }
                    else
                    {
                        snprintf(module_list[mod].status, sizeof(module_list[mod].status), "OldAPI");
                        snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Wrong version (v%d.%d, expected v%d.%d)\n", module_list[mod].info->api_major, module_list[mod].info->api_minor, MODULE_MAJOR, MODULE_MINOR);
                        console_printf("  [E] %s\n", module_list[mod].long_status);
                    }
                }
                else
                {
                    snprintf(module_list[mod].status, sizeof(module_list[mod].status), "Magic");
                    snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Invalid Magic (0x%X)\n", module_list[mod].info->api_magic);
                    console_printf("  [E] %s\n", module_list[mod].long_status);
                }
            }
            else
            {
                snprintf(module_list[mod].status, sizeof(module_list[mod].status), "noInfo");
                snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "No info structure found (addr 0x%X)\n", (uint32_t)module_list[mod].info);
                console_printf("  [E] %s\n", module_list[mod].long_status);
            }
        }
    }
    
    if(update_properties)
    {
        prop_update_registration();
    }
    
    module_state = state;
    console_printf("Modules loaded\n");
    
    if(!module_console_enabled)
    {
        console_hide();
    }
}

static void _module_unload_all(void)
{
    if(module_state)
    {
        TCCState *state = module_state;
        module_state = NULL;
        
        /* first unregister all property handlers */
        prop_reset_registration();
        
        /* deinit and clean all modules */
        for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
        {
            if(module_list[mod].info && module_list[mod].info->deinit)
            {
                module_list[mod].info->deinit();
            }
            module_list[mod].valid = 0;
            module_list[mod].info = NULL;
            module_list[mod].strings = NULL;
            module_list[mod].params = NULL;
            module_list[mod].prop_handlers = NULL;
            module_list[mod].cbr = NULL;
            strcpy(module_list[mod].name, "");
            strcpy(module_list[mod].filename, "");
        }

        /* release the global module state */
        tcc_delete(state);
    }
}

void *module_load(char *filename)
{
    int ret = -1;
    TCCState *state = NULL;

    state = tcc_new();
    tcc_set_options(state, "-nostdlib");

    if(module_load_symbols(state, MAGIC_SYMBOLS) < 0)
    {
        NotifyBox(2000, "Missing symbol file: " MAGIC_SYMBOLS );
        tcc_delete(state);
        return NULL;
    }

    ret = tcc_add_file(state, filename);
    if(ret < 0)
    {
        tcc_delete(state);
        return NULL;
    }

    ret = tcc_relocate(state, TCC_RELOCATE_AUTO);
    if(ret < 0)
    {
        tcc_delete(state);
        return NULL;
    }

    return (void*)state;
}


int module_exec(void *module, char *symbol, int count, ...)
{
    int ret = -1;
    TCCState *state = (TCCState *)module;
    void *start_symbol = NULL;
    va_list args;
    va_start(args, count);

    start_symbol = tcc_get_symbol(state, symbol);

    /* check if the module symbol is defined. simple check for valid memory address just in case. */
    if((uint32_t)start_symbol > 0x1000)
    {
        /* no parameters, special case */
        if(count == 0)
        {
            uint32_t (*exec)() = start_symbol;
            ret = exec();
        }
        else
        {
            uint32_t (*exec)(uint32_t parm1, ...) = start_symbol;

            uint32_t *parms = malloc(sizeof(uint32_t) * count);
            for(int parm = 0; parm < count; parm++)
            {
                parms[parm] = va_arg(args,uint32_t);
            }

            switch(count)
            {
                case 1:
                    ret = exec(parms[0]);
                    break;
                case 2:
                    ret = exec(parms[0], parms[1]);
                    break;
                case 3:
                    ret = exec(parms[0], parms[1], parms[2]);
                    break;
                case 4:
                    ret = exec(parms[0], parms[1], parms[2], parms[3]);
                    break;
                case 5:
                    ret = exec(parms[0], parms[1], parms[2], parms[3], parms[4]);
                    break;
                case 6:
                    ret = exec(parms[0], parms[1], parms[2], parms[3], parms[4], parms[5]);
                    break;
                case 7:
                    ret = exec(parms[0], parms[1], parms[2], parms[3], parms[4], parms[5], parms[6]);
                    break;
                case 8:
                    ret = exec(parms[0], parms[1], parms[2], parms[3], parms[4], parms[5], parms[6], parms[7]);
                    break;
                default:
                    /* so many parameters?! */
                    NotifyBox(2000, "Passing too many parameters to '%s'", symbol );
                    break;
            }
            free(parms);
        }
    }
    va_end(args);
    return ret;
}

int module_unload(void *module)
{
    TCCState *state = (TCCState *)module;
    tcc_delete(state);
    return 0;
}

/* execute all callback routines of given type. maybe it will get extended to support varargs */
int module_exec_cbr(unsigned int type)
{
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        module_cbr_t *cbr = module_list[mod].cbr;
        if(module_list[mod].valid && cbr)
        {
            while(cbr->name)
            {
                if(cbr->type == type)
                {
                    cbr->handler(cbr->ctx);
                }
                cbr++;
            }
        }
    }
    
    return 0;
}


static MENU_UPDATE_FUNC(module_menu_update_autoload)
{
    int mod_number = (int) entry->priv;

    MENU_SET_VALUE("ON");
    MENU_SET_ICON(MNI_ON, 0);
    MENU_SET_WARNING(MENU_WARN_ADVICE, module_list[mod_number].long_filename);
}

static MENU_UPDATE_FUNC(module_menu_update_parameter)
{
    char *str = (char*)entry->priv;
    
    if(str)
    {
        MENU_SET_VALUE(str);
    }
    else
    {
        MENU_SET_VALUE("");
    }
}


static MENU_UPDATE_FUNC(module_menu_update_entry)
{
    int mod_number = (int) entry->priv;

    if(module_list[mod_number].valid)
    {
        if(module_list[mod_number].info->long_name)
        {
            MENU_SET_NAME(module_list[mod_number].info->long_name);
        }
        else
        {
            MENU_SET_NAME(module_list[mod_number].name);
        }
        MENU_SET_ICON(MNI_ON, 0);
        MENU_SET_ENABLED(1);
        MENU_SET_VALUE(module_list[mod_number].status);
        MENU_SET_WARNING(MENU_WARN_ADVICE, module_list[mod_number].long_status);
    }
    else if(strlen(module_list[mod_number].filename))
    {
        MENU_SET_NAME(module_list[mod_number].filename);
        MENU_SET_ICON(MNI_OFF, 0);
        MENU_SET_ENABLED(1);
        MENU_SET_VALUE(module_list[mod_number].status);
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, module_list[mod_number].long_status);
    }
    else
    {
        MENU_SET_NAME("");
        MENU_SET_ICON(MNI_NONE, 0);
        MENU_SET_ENABLED(1);
        MENU_SET_VALUE("(nonexistent)");
        MENU_SET_HELP("You should never see this");
    }
}

static MENU_SELECT_FUNC(module_menu_select_empty)
{
}


/* check which modules are loaded and hide others */
static void module_menu_update()
{
    int mod_number = 0;
    struct menu_entry * entry = module_menu;

    while (entry)
    {
        /* only update those which display module information */
        if(entry->update == module_menu_update_entry)
        {
            if(module_list[mod_number].valid)
            {
                MENU_SET_SHIDDEN(0);
            }
            else if(strlen(module_list[mod_number].filename))
            {
                MENU_SET_SHIDDEN(0);
            }
            else
            {
                MENU_SET_SHIDDEN(1);
            }
            mod_number++;
        }
        entry = entry->next;
    }
}

/* check which modules are loaded and hide others */
static void module_submenu_update(int mod_number)
{
    /* displaying two menus before parameters */
    int entry = 1;

    /* set autoload menu's priv to module id */
    module_submenu[0].priv = (void*)mod_number;

    /* make sure this module is loaded */
    if(module_list[mod_number].valid)
    {
        module_strpair_t *strings = module_list[mod_number].strings;
        module_parminfo_t *parms = module_list[mod_number].params;
        module_cbr_t *cbr = module_list[mod_number].cbr;
        module_prophandler_t **props = module_list[mod_number].prop_handlers;

        if (strings)
        {
            if(module_submenu[entry].priv != MENU_EOL_PRIV)
            {
                module_submenu[entry].name = "----Information---";
                module_submenu[entry].priv = (void*)0;
                module_submenu[entry].update = module_menu_update_parameter;
                module_submenu[entry].select = module_menu_select_empty;
                module_submenu[entry].shidden = 0;
                entry++;
            }
            
            while((strings->name != NULL) && (module_submenu[entry].priv != MENU_EOL_PRIV))
            {
                module_submenu[entry].name = strings->name;
                module_submenu[entry].priv = (void*)strings->value;
                module_submenu[entry].update = module_menu_update_parameter;
                module_submenu[entry].select = module_menu_select_empty;
                module_submenu[entry].shidden = 0;
                strings++;
                entry++;
            }
        }
        
        if (parms)
        {
            if(module_submenu[entry].priv != MENU_EOL_PRIV)
            {
                module_submenu[entry].name = "----Parameters----";
                module_submenu[entry].priv = (void*)0;
                module_submenu[entry].update = module_menu_update_parameter;
                module_submenu[entry].select = module_menu_select_empty;
                module_submenu[entry].shidden = 0;
                entry++;
            }

            while((parms->name != NULL) && (module_submenu[entry].priv != MENU_EOL_PRIV))
            {
                module_submenu[entry].name = parms->name;
                module_submenu[entry].priv = (void*)parms->type;
                module_submenu[entry].help = parms->desc;
                module_submenu[entry].update = module_menu_update_parameter;
                module_submenu[entry].select = module_menu_select_empty;
                module_submenu[entry].shidden = 0;
                parms++;
                entry++;
            }
        }
        
        if (props && *props)
        {
            if(module_submenu[entry].priv != MENU_EOL_PRIV)
            {
                module_submenu[entry].name = "----Properties----";
                #if !defined(FEATURE_UNREGISTER_PROP)
                module_submenu[entry].priv = " (no support)";
                #endif
                module_submenu[entry].update = module_menu_update_parameter;
                module_submenu[entry].select = module_menu_select_empty;
                module_submenu[entry].shidden = 0;
                entry++;
            }

            while((*props != NULL) && (module_submenu[entry].priv != MENU_EOL_PRIV))
            {
                module_submenu[entry].name = (*props)->name;
                module_submenu[entry].priv = (void*)0;
                module_submenu[entry].help = "";
                module_submenu[entry].update = module_menu_update_parameter;
                module_submenu[entry].select = module_menu_select_empty;
                module_submenu[entry].shidden = 0;
                props++;
                entry++;
            }
        }
        
        if (cbr)
        {
            if(module_submenu[entry].priv != MENU_EOL_PRIV)
            {
                module_submenu[entry].name = "----Callbacks-----";
                module_submenu[entry].priv = (void*)0;
                module_submenu[entry].update = module_menu_update_parameter;
                module_submenu[entry].select = module_menu_select_empty;
                module_submenu[entry].shidden = 0;
                entry++;
            }

            while((cbr->name != NULL) && (module_submenu[entry].priv != MENU_EOL_PRIV))
            {
                module_submenu[entry].name = cbr->name;
                module_submenu[entry].priv = (void*)cbr->symbol;
                module_submenu[entry].help = "";
                module_submenu[entry].update = module_menu_update_parameter;
                module_submenu[entry].select = module_menu_select_empty;
                module_submenu[entry].shidden = 0;
                cbr++;
                entry++;
            }
        }
    }

    /* disable other entries */
    while(module_submenu[entry].priv != MENU_EOL_PRIV)
    {
        module_submenu[entry].shidden = 1;
        entry++;
    }
}

static MENU_SELECT_FUNC(module_menu_load)
{
    console_show();
    module_load_all();
}

static MENU_SELECT_FUNC(module_menu_unload)
{
    module_unload_all();
}

static MENU_SELECT_FUNC(module_open_submenu)
{
    int mod_number = (int)priv;
    module_submenu_update(mod_number);
    menu_open_submenu();
}

static struct menu_entry module_submenu[] = {
        {
            .name = "Autoload module",
            .icon_type = MNI_ON,
            .max = 1,
            .update = module_menu_update_autoload,
            .select = module_menu_select_empty,
            .help = "Load automatically on startup. (Not implemented yet)",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        {
            .help = "",
        },
        MENU_EOL
};

#define MODULE_ENTRY(i) \
        { \
            .name = "Module", \
            .priv = (void*)i, \
            .select = module_open_submenu, \
            .select_Q = module_open_submenu, \
            .update = module_menu_update_entry, \
            .icon_type = IT_SUBMENU, \
            .submenu_width = 700, \
            .children = module_submenu, \
            .help = "", \
            .help2 = "", \
        },

static struct menu_entry module_menu[] = {
    {
        .name = "Load modules now...",
        .select = module_menu_load,
        .help = "Loads modules in "MODULE_PATH,
    },
    {
        .name = "Unload modules now...",
        .select = module_menu_unload,
        .help = "Unload loaded modules",
    },
    {
        .name = "Autoload modules on startup",
        .priv = &module_autoload_enabled,
        .max = 1,
        .help = "Loads modules every startup",
    },
    {
        .name = "Show console",
        .priv = &module_console_enabled,
        .max = 1,
        .help = "Keep console shown after modules were loaded",
    },
    {
        .name = "----Modules----",
        .icon_type = MNI_NONE,
        .select = module_menu_select_empty,
    },
    MODULE_ENTRY(0)
    MODULE_ENTRY(1)
    MODULE_ENTRY(2)
    MODULE_ENTRY(3)
    MODULE_ENTRY(4)
    MODULE_ENTRY(5)
    MODULE_ENTRY(6)
    MODULE_ENTRY(7)
    MODULE_ENTRY(8)
    MODULE_ENTRY(9)
    MODULE_ENTRY(10)
    MODULE_ENTRY(11)
    MODULE_ENTRY(12)
    MODULE_ENTRY(13)
    MODULE_ENTRY(14)
};

static void module_init()
{
    module_mq = (struct msg_queue *) msg_queue_create("module_mq", 1);
    menu_add("Modules", module_menu, COUNT(module_menu));
    module_menu_update();
}

void module_load_task(void* unused) 
{
    /* no clean shutdown hoom implemented yet */
#if defined(MODULE_CHECK_CRASH)
    char *lockfile = MODULE_PATH"LOADING.LCK";
    char *lockstr = "If you can read this, ML crashed last time. To save from faulty modules, autoload gets disabled.";
#endif

    if(module_autoload_enabled)
    {
#if defined(MODULE_CHECK_CRASH)
        uint32_t size;
        if( FIO_GetFileSize( lockfile, &size ) == 0 )
        {
            /* uh, it seems the camera didnt shut down cleanly, skip module loading this time */
            msleep(1000);
            NotifyBox(10000, "Camera was not shut down cleanly.\r\nSkipping module loading." );
        }
        else
        {
            FILE *handle = FIO_CreateFileEx(lockfile);
            FIO_WriteFile(handle, lockstr, strlen(lockstr));
            FIO_CloseFile(handle);
            
            /* now load modules */
            _module_load_all();
            module_menu_update();
        }
#else
        /* now load modules */
        _module_load_all();
        module_menu_update();
#endif
    }
        
    /* main loop, also wait until clean shutdown */
    TASK_LOOP
    {
        int msg;
        int err = msg_queue_receive(module_mq, (struct event**)&msg, 200);
        if (err) continue;
        
        switch(msg)
        {
            case MSG_MODULE_LOAD_ALL:
                _module_load_all();
                module_menu_update();
                break;

            case MSG_MODULE_UNLOAD_ALL:
                _module_unload_all();
                module_menu_update();
                beep();
                break;
            
            default:
                console_printf("invalid msg: %d\n", msg);
        }
    }

#if defined(MODULE_CHECK_CRASH)
    if(module_autoload_enabled)
    {
        /* remove lockfile */
        FIO_RemoveFile(lockfile);
    }
#endif
}

TASK_CREATE("module_load_task", module_load_task, 0, 0x1e, 0x4000 );

INIT_FUNC(__FILE__, module_init);

