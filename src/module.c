
#include "dryos.h"
#include "console.h"
#include "libtcc.h"
#include "module.h"

static module_entry_t module_list[MODULE_COUNT_MAX];
static TCCState *module_state = NULL;

static int module_load_symbols(TCCState *s, char *filename)
{
    unsigned size = 0;
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

        tcc_add_symbol(s, symbol_buf, address);
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

void module_load_all(void)
{
    TCCState *state = NULL;
    uint32_t module_cnt = 0;
    struct fio_file file;

    /* ensure all modules are unloaded */
    console_printf("Unloading modules...\n");
    module_unload_all();
    
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
            strncpy(module_list[module_cnt].name, module_name, MODULE_NAME_LENGTH);

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
        char filename[64];

        console_printf("  [i] load: %s\n", module_list[mod].filename);
        snprintf(filename, sizeof(filename), "%s%s", MODULE_PATH, module_list[mod].filename);
        int32_t ret = tcc_add_file(state, filename);

        /* seems bad, disable it */
        if(ret < 0)
        {
            console_printf("  [E] load failed: %s ret %x\n", filename, ret);
            //NotifyBox(2000, "Plugin file '%s' seems to be invalid", module_list[mod] );
            strcpy(module_list[mod].filename, "");
        }
    }

    console_printf("Linking..\n");
    int32_t ret = tcc_relocate(state, TCC_RELOCATE_AUTO);
    if(ret < 0)
    {
        console_printf("  [E] failed to link modules\n");
        //NotifyBox(2000, "Failed to link" );
        tcc_delete(state);
        return;
    }

    console_printf("Init modules...\n");
    /* init modules */
    for (uint32_t mod = 0; mod < module_cnt; mod++)
    {
        if(strlen(module_list[mod].name) > 0)
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

            /* check if the module symbol is defined. simple check for valid memory address just in case. */
            if((uint32_t)module_list[mod].info > 0x1000)
            {
                if(module_list[mod].info->api_magic == MODULE_MAGIC)
                {
                    if(module_list[mod].info->api_major == MODULE_MAJOR && module_list[mod].info->api_minor <= MODULE_MINOR)
                    {
                        module_list[mod].valid = 1;
                        
                        console_printf("  [i] info    at: 0x%08X\n", (uint32_t)module_list[mod].info);
                        console_printf("  [i] strings at: 0x%08X\n", (uint32_t)module_list[mod].strings);
                        console_printf("  [i] params  at: 0x%08X\n", (uint32_t)module_list[mod].params);
                        console_printf("-----------------------------\n");
                        if(module_list[mod].info->init)
                        {
                            module_list[mod].info->init();
                        }
                        console_printf("-----------------------------\n");
                    }
                    else
                    {
                        console_printf("  [E] invalid version (v%d.%d, expected v%d.%d)\n", module_list[mod].info->api_major, module_list[mod].info->api_minor, MODULE_MAJOR, MODULE_MINOR);
                    }
                }
                else
                {
                    console_printf("  [E] invalid MAGIC (0x%X)\n", module_list[mod].info->api_magic);
                }
            }
            else
            {
                console_printf("  [E] no info found (0x%X)\n", (uint32_t)module_list[mod].info);
            }
        }
    }

    module_state = state;
    console_printf("Modules loaded\n");
}

void module_unload_all(void)
{
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        module_list[mod].valid = 0;
        module_list[mod].info = NULL;
        module_list[mod].strings = NULL;
        module_list[mod].params = NULL;
        strcpy(module_list[mod].name, "");
        strcpy(module_list[mod].filename, "");
    }
    
    if(module_state)
    {
        TCCState *state = module_state;
        module_state = NULL;
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
            for(uint32_t parm = 0; parm < count; parm++)
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
