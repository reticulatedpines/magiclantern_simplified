
#include "dryos.h"
#include "menu.h"
#include "console.h"
#include "libtcc.h"
#include "module.h"
#include "config.h"
#include "string.h"
#include "property.h"
#include "beep.h"
#include "bmp.h"
#include "lens.h"
#include "ml-cbr.h"

#ifndef CONFIG_MODULES_MODEL_SYM
#error Not defined file name with symbols
#endif
#define MAGIC_SYMBOLS                 "ML/MODULES/"CONFIG_MODULES_MODEL_SYM

/* unloads TCC after linking the modules */
/* note: this breaks module_exec and ETTR */
#define CONFIG_TCC_UNLOAD

extern int sscanf(const char *str, const char *format, ...);


static module_entry_t module_list[MODULE_COUNT_MAX];

#ifdef CONFIG_TCC_UNLOAD
static void* module_code = NULL;
#else
static TCCState *module_state = NULL;
#endif

static struct menu_entry module_submenu[];
static struct menu_entry module_menu[];

CONFIG_INT("module.autoload", module_autoload_disabled, 0);
CONFIG_INT("module.console", module_console_enabled, 0);
CONFIG_INT("module.ignore_crashes", module_ignore_crashes, 0);
char *module_lockfile = MODULE_PATH"LOADING.LCK";

static struct msg_queue * module_mq = 0;
// #define MSG_MODULE_LOAD_ALL 1
// #define MSG_MODULE_UNLOAD_ALL 2
#define MSG_MODULE_LOAD_OFFLINE_STRINGS 3 /* argument: module index in high half (FFFF0000) */
#define MSG_MODULE_UNLOAD_OFFLINE_STRINGS 4 /* same argument */

static int module_load_symbols(TCCState *s, char *filename)
{
    uint32_t size = 0;
    FILE* file = NULL;
    char *buf = NULL;
    uint32_t count = 0;
    uint32_t pos = 0;

    if( FIO_GetFileSize( filename, &size ) != 0 )
    {
        printf("Error loading '%s': File does not exist\n", filename);
        return -1;
    }
    buf = fio_malloc(size);
    if(!buf)
    {
        printf("Error loading '%s': File too large\n", filename);
        return -1;
    }

    file = FIO_OpenFile(filename, O_RDONLY | O_SYNC);
    if (!file)
    {
        printf("Error loading '%s': File does not exist\n", filename);
        fio_free(buf);
        return -1;
    }
    FIO_ReadFile(file, buf, size);
    FIO_CloseFile(file);

    while(pos < size && buf[pos])
    {
        char address_buf[16];
        char symbol_buf[128];
        uint32_t length = 0;
        uint32_t address = 0;

        while (pos + length < size &&
               buf[pos + length] &&
               buf[pos + length] != ' ' &&
               length < sizeof(address_buf))
        {
            address_buf[length] = buf[pos + length];
            length++;
        }
        address_buf[length] = '\000';

        pos += length + 1;
        length = 0;

        while (pos + length < size &&
               buf[pos + length] &&
               buf[pos + length] != '\r' &&
               buf[pos + length] != '\n' &&
               length < sizeof(symbol_buf))
        {
            symbol_buf[length] = buf[pos + length];
            length++;
        }
        symbol_buf[length] = '\000';

        pos += length + 1;
        length = 0;

        while (pos + length < size &&
               buf[pos + length] &&
              (buf[pos + length] == '\r' ||
               buf[pos + length] == '\n'))
        {
            pos++;
        }
        //~ sscanf(address_buf, "%x", &address);
        address = strtoul(address_buf, NULL, 16);

        tcc_add_symbol(s, symbol_buf, (void*)address);
        count++;
    }
    
    fio_free(buf);
    return 0;
}

/* this is not perfect, as .Mo and .mO aren't detected. important? */
static int module_valid_filename(char* filename)
{
    int len = strlen(filename);
    
    if((len < 3) || (filename[0] == '.') || (filename[0] == '_'))
    {
        return 0;
    }
    
    if(!strcmp(&filename[len - 3], ".MO") || !strcmp(&filename[len - 3], ".mo") )
    {
        return 1;
    }
    
    return 0;
}

/* must be called before unloading TCC */
static void module_update_core_symbols(TCCState* state)
{
    printf("Updating symbols...\n");

    extern struct module_symbol_entry _module_symbols_start[];
    extern struct module_symbol_entry _module_symbols_end[];
    struct module_symbol_entry * module_symbol_entry = _module_symbols_start;

    for( ; module_symbol_entry < _module_symbols_end ; module_symbol_entry++ )
    {
        void* old_address = *(module_symbol_entry->address);
        void* new_address = (void*) tcc_get_symbol(state, (char*) module_symbol_entry->name);
        if (new_address)
        {
            if (new_address != module_symbol_entry->address)
            {
                *(module_symbol_entry->address) = new_address;
                printf("  [i] upd: %s %x => %x\n", module_symbol_entry->name, old_address, new_address);
            }
            else
            {
                /* you have declared a non-static MODULE_SYMBOL (or MODULE_FUNCTION), which got exported into the SYM file */
                /* this will not work; fix it by declaring it as static */
                console_show();
                printf("  [E] wtf: %s (should be static)\n", module_symbol_entry->name);
            }
        }
        else
        {
            printf("  [i] 404: %s %x\n", module_symbol_entry->name, old_address);
        }
    }
}

// SJE ML doesn't include the struct definition for TCCState in libtcc.h, which means
// you can't access members by name.  Either this gets resolved somehow during the
// build (if so, don't know how), or ML code never uses struct members?
//
// Can't simply include the header, because it redefines malloc, which conflicts
// with ML also redefining malloc...
//
// Copying in the version in tcc/tcc.h, which is terribly ugly just for
// finding the module load address.  There must be a better way.
#if 0
#include <setjmp.h>
#define addr_t uint32_t
#define IO_BUF_SIZE 8192
#define INCLUDE_STACK_SIZE  32
#define IFDEF_STACK_SIZE    64
#define CACHED_INCLUDES_HASH_SIZE 512
#define PACK_STACK_SIZE     8
#define LDOUBLE_SIZE  8

typedef struct DLLReference {
    int level;
    void *handle;
    char name[1];
} DLLReference;

typedef struct CString {
    int size; /* size in bytes */
    void *data; /* either 'char *' or 'nwchar_t *' */
    int size_allocated;
    void *data_allocated; /* if non NULL, data has been malloced */
} CString;

/* type definition */
typedef struct CType {
    int t;
    struct Sym *ref;
} CType;

/* constant value */
typedef union CValue {
    long double ld;
    double d;
    float f;
    int i;
    unsigned int ui;
    unsigned int ul; /* address (should be unsigned long on 64 bit cpu) */
    long long ll;
    unsigned long long ull;
    struct CString *cstr;
    void *ptr;
    int tab[LDOUBLE_SIZE/4];
} CValue;

/* value on stack */
typedef struct SValue {
    CType type;      /* type */
    unsigned short r;      /* register + flags */
    unsigned short r2;     /* second register, used for 'long long'
                              type. If not used, set to VT_CONST */
    CValue c;              /* constant, if VT_CONST */
    struct Sym *sym;       /* symbol, if (VT_SYM | VT_CONST) */
} SValue;

typedef struct Sym {
    int v;    /* symbol token */
    char *asm_label;    /* associated asm label */
    long r;    /* associated register */
    union {
        long c;    /* associated number */
        int *d;   /* define token stream */
    };
    CType type;    /* associated type */
    union {
        struct Sym *next; /* next related symbol */
        long jnext; /* next jump label */
    };
    struct Sym *prev; /* prev symbol in stack */
    struct Sym *prev_tok; /* previous symbol for this token */
} Sym;

typedef struct Section {
    unsigned long data_offset; /* current data offset */
    unsigned char *data;       /* section data */
    unsigned long data_allocated; /* used for realloc() handling */
    int sh_name;             /* elf section name (only used during output) */
    int sh_num;              /* elf section number */
    int sh_type;             /* elf section type */
    int sh_flags;            /* elf section flags */
    int sh_info;             /* elf section info */
    int sh_addralign;        /* elf section alignment */
    int sh_entsize;          /* elf entry size */
    unsigned long sh_size;   /* section size (only used during output) */
    addr_t sh_addr;          /* address at which the section is relocated */
    unsigned long sh_offset; /* file offset */
    int nb_hashed_syms;      /* used to resize the hash table */
    struct Section *link;    /* link to another section */
    struct Section *reloc;   /* corresponding section for relocation, if any */
    struct Section *hash;     /* hash table for symbols */
    struct Section *next;
    char name[1];           /* section name */
} Section;

typedef struct CachedInclude {
    int ifndef_macro;
    int hash_next; /* -1 if none */
    char filename[1]; /* path specified in #include */
} CachedInclude;

typedef struct BufferedFile {
    uint8_t *buf_ptr;
    uint8_t *buf_end;
    int fd;
    struct BufferedFile *prev;
    int line_num;    /* current line number - here to simplify code */
    int ifndef_macro;  /* #ifndef macro / #endif search */
    int ifndef_macro_saved; /* saved ifndef_macro */
    int *ifdef_stack_ptr; /* ifdef_stack value at the start of the file */
    char filename[1024];    /* filename */
    unsigned char buffer[IO_BUF_SIZE + 1]; /* extra size for CH_EOB char */
} BufferedFile;


struct TCCState {

    int verbose; /* if true, display some information during compilation */
    int nostdinc; /* if true, no standard headers are added */
    int nostdlib; /* if true, no standard libraries are added */
    int nocommon; /* if true, do not use common symbols for .bss data */
    int static_link; /* if true, static linking is performed */
    int rdynamic; /* if true, all symbols are exported */
    int symbolic; /* if true, resolve symbols in the current module first */
    int alacarte_link; /* if true, only link in referenced objects from archive */

    char *tcc_lib_path; /* CONFIG_TCCDIR or -B option */
    char *soname; /* as specified on the command line (-soname) */
    char *rpath; /* as specified on the command line (-Wl,-rpath=) */

    /* output type, see TCC_OUTPUT_XXX */
    int output_type;
    /* output format, see TCC_OUTPUT_FORMAT_xxx */
    int output_format;

    /* C language options */
    int char_is_unsigned;
    int leading_underscore;
    
    /* warning switches */
    int warn_write_strings;
    int warn_unsupported;
    int warn_error;
    int warn_none;
    int warn_implicit_function_declaration;

    /* compile with debug symbol (and use them if error during execution) */
    int do_debug;
#ifdef CONFIG_TCC_BCHECK
    /* compile with built-in memory and bounds checker */
    int do_bounds_check;
#endif

    addr_t text_addr; /* address of text section */
    int has_text_addr;

    unsigned long section_align; /* section alignment */

    char *init_symbol; /* symbols to call at load-time (not used currently) */
    char *fini_symbol; /* symbols to call at unload-time (not used currently) */
    
#ifdef TCC_TARGET_I386
    int seg_size; /* 32. Can be 16 with i386 assembler (.code16) */
#endif

    /* array of all loaded dlls (including those referenced by loaded dlls) */
    DLLReference **loaded_dlls;
    int nb_loaded_dlls;

    /* include paths */
    char **include_paths;
    int nb_include_paths;

    char **sysinclude_paths;
    int nb_sysinclude_paths;

    /* library paths */
    char **library_paths;
    int nb_library_paths;

    /* crt?.o object path */
    char **crt_paths;
    int nb_crt_paths;

    /* error handling */
    void *error_opaque;
    void (*error_func)(void *opaque, const char *msg);
    int error_set_jmp_enabled;
    jmp_buf error_jmp_buf;
    int nb_errors;

    /* output file for preprocessing (-E) */
    FILE *ppfp;

    /* for -MD/-MF: collected dependencies for this compilation */
    char **target_deps;
    int nb_target_deps;

    /* compilation */
    BufferedFile *include_stack[INCLUDE_STACK_SIZE];
    BufferedFile **include_stack_ptr;

    int ifdef_stack[IFDEF_STACK_SIZE];
    int *ifdef_stack_ptr;

    /* included files enclosed with #ifndef MACRO */
    int cached_includes_hash[CACHED_INCLUDES_HASH_SIZE];
    CachedInclude **cached_includes;
    int nb_cached_includes;

    /* #pragma pack stack */
    int pack_stack[PACK_STACK_SIZE];
    int *pack_stack_ptr;

    /* inline functions are stored as token lists and compiled last
       only if referenced */
    struct InlineFunc **inline_fns;
    int nb_inline_fns;

    /* sections */
    Section **sections;
    int nb_sections; /* number of sections, including first dummy section */

    Section **priv_sections;
    int nb_priv_sections; /* number of private sections */

    /* got & plt handling */
    Section *got;
    Section *plt;
    struct sym_attr *sym_attrs;
    int nb_sym_attrs;
    /* give the correspondance from symtab indexes to dynsym indexes */
    int *symtab_to_dynsym;

    /* temporary dynamic symbol sections (for dll loading) */
    Section *dynsymtab_section;
    /* exported dynamic symbol section */
    Section *dynsym;
    /* copy of the gobal symtab_section variable */
    Section *symtab;
    /* tiny assembler state */
    Sym *asm_labels;

#ifdef TCC_TARGET_PE
    /* PE info */
    int pe_subsystem;
    unsigned pe_file_align;
    unsigned pe_stack_size;
# ifdef TCC_TARGET_X86_64
    Section *uw_pdata;
    int uw_sym;
    unsigned uw_offs;
# endif
#endif

#ifdef TCC_IS_NATIVE
    /* for tcc_relocate */
    void *runtime_mem;
# ifdef HAVE_SELINUX
    void *write_mem;
    unsigned long mem_size;
# endif
# if !defined TCC_TARGET_PE && (defined TCC_TARGET_X86_64 || defined TCC_TARGET_ARM)
    /* write PLT and GOT here */
    char *runtime_plt_and_got;
    unsigned runtime_plt_and_got_offset;
#  define TCC_HAS_RUNTIME_PLTGOT
# endif
#endif

    /* used by main and tcc_parse_args only */
    char **files; /* files seen on command line */
    int nb_files; /* number thereof */
    int nb_libraries; /* number of libs thereof */
    char *outfile; /* output filename */
    char *option_m; /* only -m32/-m64 handled */
    int print_search_dirs; /* option */
    int option_r; /* option -r */
    int do_bench; /* option -bench */
    int gen_deps; /* option -MD  */
    char *deps_outfile; /* option -MF */
};

#endif

static void _module_load_all(uint32_t list_only)
{
    TCCState *state = NULL;
    uint32_t module_cnt = 0;
    struct fio_file file;
    uint32_t update_properties = 0;

    if(module_console_enabled)
    {
        console_show();
    }
    else
    {
        console_hide();
    }

#ifdef CONFIG_TCC_UNLOAD
    if (module_code)
#else
    if (module_state)
#endif
    {
        printf("Modules already loaded.\n");
        beep();
        return;
    }

    /* initialize linker */
    state = tcc_new();
    tcc_set_options(state, "-nostdlib");
    if(module_load_symbols(state, MAGIC_SYMBOLS) < 0)
    {
        NotifyBox(2000, "Missing symbol file: " MAGIC_SYMBOLS );
        tcc_delete(state); console_show();
        return;
    }

    printf("Scanning modules...\n");
    struct fio_dirent * dirent = FIO_FindFirstEx( MODULE_PATH, &file );
    if( IS_ERROR(dirent) )
    {
        NotifyBox(2000, "Module dir missing" );
        tcc_delete(state); console_show();
        return;
    }

    do
    {
        if (file.mode & ATTR_DIRECTORY) continue; // is a directory
        if (module_valid_filename(file.name))
        {
            char module_name[MODULE_FILENAME_LENGTH];

            /* get filename, remove extension and append _init to get the init symbol */
            //printf("  [i] found: %s\n", file.name);
            
            /* ensure the buffer is null terminated */
            memset(module_name, 0x00, sizeof(module_name));
            strncpy(module_name, file.name, MODULE_NAME_LENGTH);
            strncpy(module_list[module_cnt].filename, file.name, MODULE_FILENAME_LENGTH);
            snprintf(module_list[module_cnt].long_filename, sizeof(module_list[module_cnt].long_filename), "%s%s", MODULE_PATH, module_list[module_cnt].filename);

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
            
            /* check for a .en file that tells the module is enabled */
            char enable_file[FIO_MAX_PATH_LENGTH];
            snprintf(enable_file, sizeof(enable_file), "%s%s.en", get_config_dir(), module_list[module_cnt].name);
            
            /* if enable-file is nonexistent, dont load module */
            if(!config_flag_file_setting_load(enable_file))
            {
                module_list[module_cnt].enabled = 0;
                snprintf(module_list[module_cnt].status, sizeof(module_list[module_cnt].status), "OFF");
                snprintf(module_list[module_cnt].long_status, sizeof(module_list[module_cnt].long_status), "Module disabled");
                //printf("  [i] %s\n", module_list[module_cnt].long_status);
            }
            else
            {
                module_list[module_cnt].enabled = 1;
            
                if(list_only)
                {
                    snprintf(module_list[module_cnt].status, sizeof(module_list[module_cnt].status), "");
                    snprintf(module_list[module_cnt].long_status, sizeof(module_list[module_cnt].long_status), "Module not loaded");
                }
                else
                {
                    snprintf(module_list[module_cnt].status, sizeof(module_list[module_cnt].status), "???");
                    snprintf(module_list[module_cnt].long_status, sizeof(module_list[module_cnt].long_status), "Seems linking failed. Unknown symbols?");
                }
            }

            module_cnt++;
            if (module_cnt >= MODULE_COUNT_MAX)
            {
                NotifyBox(2000, "Too many modules" );
                break;
            }
        }
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);
    
    /* sort modules */
    for (int i = 0; i < (int) module_cnt-1; i++)
    {
        for (int j = i+1; j < (int) module_cnt; j++)
        {
            if (
                    /* loaded modules first, then alphabetically */
                    (module_list[i].enabled == 0 && module_list[j].enabled) || 
                    (module_list[i].enabled == module_list[j].enabled && strcmp(module_list[i].name, module_list[j].name) > 0)
               )
            {
                module_entry_t aux = module_list[i];
                module_list[i] = module_list[j];
                module_list[j] = aux;
            }
        }
    }
    

    /* dont load anything, just return */
    if(list_only)
    {
        tcc_delete(state);
        return;
    }
    
    /* load modules */
    printf("Load modules...\n");
    for (uint32_t mod = 0; mod < module_cnt; mod++)
    {
        if(module_list[mod].enabled)
        {
            printf("  [i] load: %s\n", module_list[mod].filename);

            int32_t ret = tcc_add_file(state, module_list[mod].long_filename);

            // SJE FIXME trying to determine module base address
            // so I can use addr2line.  The listed address seems wrong,
            // don't know why.  Instead I am dumping some function address
            // from whatever module I'm testing, which is annoyingly module
            // specific
#if 0
            int size = 0;
            void *data_addr = NULL;
            data_addr = tcc_get_section_ptr(state, ".text", &size);
            DryosDebugMsg(0, 15, "loading module: %s", module_list[mod].filename);
            DryosDebugMsg(0, 15, "module priv: 0x%x", module_list[mod].cbr);
            DryosDebugMsg(0, 15, "module .text: 0x%x", data_addr);
            DryosDebugMsg(0, 15, "module text_addr: 0x%x", state->text_addr);
//            DryosDebugMsg(0, 15, "sections: %d", state->nb_sections);
//            for (int ii = 1; ii < state->nb_sections; ii++)
//            {
//                Section *s = state->sections[ii];
//                DryosDebugMsg(0, 15, "section: %s", s->name);
//                DryosDebugMsg(0, 15, "section sh_addr: 0x%x", s->sh_addr);
//                DryosDebugMsg(0, 15, "section data_offset: 0x%x", s->data_offset);
//                DryosDebugMsg(0, 15, "section data: 0x%x", s->data);
//            }
#endif

            module_list[mod].valid = 1;

            /* seems bad, disable it */
            if(ret < 0)
            {
                module_list[mod].error = 1;
                snprintf(module_list[mod].status, sizeof(module_list[mod].status), "FileErr");
                snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Load failed: %s, ret 0x%02X");
                printf("  [E] %s\n", module_list[mod].long_status);
            }
        }
    }

    printf("Linking..\n");
#ifdef CONFIG_TCC_UNLOAD
    int32_t size = tcc_relocate(state, NULL);
    int32_t reloc_status = -1;
    
    if (size > 0)
    {
        void* buf = (void*) malloc(size);
        
        reloc_status = tcc_relocate(state, buf);
        module_code = buf;
    }
    if(size < 0 || reloc_status < 0)
#else
    int32_t ret = tcc_relocate(state, TCC_RELOCATE_AUTO);
    if(ret < 0)
#endif
    {
        printf("  [E] failed to link modules\n");
        for (uint32_t mod = 0; mod < module_cnt; mod++)
        {
            if(module_list[mod].enabled)
            {
                module_list[mod].error = 1;
                snprintf(module_list[mod].status, sizeof(module_list[mod].status), "Err");
                snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Linking failed");
            }
        }
        tcc_delete(state); console_show();
        return;
    }
    
    /* load modules symbols */
    printf("Register modules...\n");
    for (uint32_t mod = 0; mod < module_cnt; mod++)
    {
        if(module_list[mod].valid && module_list[mod].enabled && !module_list[mod].error)
        {
            char module_info_name[32];

            /* now check for info structure */
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_INFO_PREFIX), module_list[mod].name);
            module_list[mod].info = tcc_get_symbol(state, module_info_name);
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_STRINGS_PREFIX), module_list[mod].name);
            module_list[mod].strings = tcc_get_symbol(state, module_info_name);
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_PROPHANDLERS_PREFIX), module_list[mod].name);
            module_list[mod].prop_handlers = tcc_get_symbol(state, module_info_name);
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_CBR_PREFIX), module_list[mod].name);
            module_list[mod].cbr = tcc_get_symbol(state, module_info_name);
            snprintf(module_info_name, sizeof(module_info_name), "%s%s", STR(MODULE_CONFIG_PREFIX), module_list[mod].name);
            module_list[mod].config = tcc_get_symbol(state, module_info_name);

            /* check if the module symbol is defined. simple check for valid memory address just in case. */
            if((uint32_t)module_list[mod].info > 0x1000)
            {
                if(module_list[mod].info->api_magic == MODULE_MAGIC)
                {
                    if((module_list[mod].info->api_major == MODULE_MAJOR) && (module_list[mod].info->api_minor <= MODULE_MINOR))
                    {
                        /* this module seems to be sane */
                    }
                    else
                    {
                        module_list[mod].error = 1;
                        snprintf(module_list[mod].status, sizeof(module_list[mod].status), "OldAPI");
                        snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Wrong version (v%d.%d, expected v%d.%d)\n", module_list[mod].info->api_major, module_list[mod].info->api_minor, MODULE_MAJOR, MODULE_MINOR);
                        printf("  [E] %s\n", module_list[mod].long_status);
                        
                        /* disable active stuff, since the results are unpredictable */
                        module_list[mod].cbr = 0;
                        module_list[mod].prop_handlers = 0;
                    }
                }
                else
                {
                    module_list[mod].error = 1;
                    snprintf(module_list[mod].status, sizeof(module_list[mod].status), "Magic");
                    snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Invalid Magic (0x%X)\n", module_list[mod].info->api_magic);
                    printf("  [E] %s\n", module_list[mod].long_status);
                }
            }
            else
            {
                module_list[mod].error = 1;
                snprintf(module_list[mod].status, sizeof(module_list[mod].status), "noInfo");
                snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "No info structure found (addr 0x%X)\n", (uint32_t)module_list[mod].info);
                printf("  [E] %s\n", module_list[mod].long_status);
            }
        }
    }
    
    printf("Load configs...\n");
    for (uint32_t mod = 0; mod < module_cnt; mod++)
    {
        if(module_list[mod].enabled && module_list[mod].valid && !module_list[mod].error)
        {
            char filename[64];
            snprintf(filename, sizeof(filename), "%s%s.cfg", get_config_dir(), module_list[mod].name);
            module_config_load(filename, &module_list[mod]);
        }
    }
    
    /* before we execute code, make sure a) data caches are drained and b) instruction caches are clean */
    sync_caches();
    
    /* go through all modules and initialize them */
    printf("Init modules...\n");
    for (uint32_t mod = 0; mod < module_cnt; mod++)
    {
        if(module_list[mod].valid && module_list[mod].enabled && !module_list[mod].error)
        {
            printf("  [i] Init: '%s'\n", module_list[mod].name);
            if(0)
            {
                printf("  [i] info    at: 0x%08X\n", (uint32_t)module_list[mod].info);
                printf("  [i] strings at: 0x%08X\n", (uint32_t)module_list[mod].strings);
                printf("  [i] props   at: 0x%08X\n", (uint32_t)module_list[mod].prop_handlers);
                printf("  [i] cbr     at: 0x%08X\n", (uint32_t)module_list[mod].cbr);
                printf("  [i] config  at: 0x%08X\n", (uint32_t)module_list[mod].config);
                printf("-----------------------------\n");
            }
            
            /* initialize module */
            if(module_list[mod].info->init)
            {
                int err = module_list[mod].info->init();
                
                if (err)
                {
                    module_list[mod].error = err;

                    snprintf(module_list[mod].status, sizeof(module_list[mod].status), "Err");
                    snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Module init failed");

                    /* disable active stuff, since the results are unpredictable */
                    module_list[mod].cbr = 0;
                    module_list[mod].prop_handlers = 0;
                }
            }
            
            if(!module_list[mod].error)
            {
                module_prophandler_t **props = module_list[mod].prop_handlers;
                module_cbr_t *cbr = module_list[mod].cbr;
                
                /* register property handlers */
                while(props && *props)
                {
                    update_properties = 1;
                    printf("  [i] prop %s\n", (*props)->name);
                    prop_add_handler((*props)->property, (*props)->handler);
                    props++;
                }
                
                /* register ml-cbr callback handlers */
                while(cbr && cbr->name)
                {
                    /* register "named" callbacks through ml-cbr */
                    if(cbr->type == CBR_NAMED)
                    {
                        printf("  [i] ml-cbr '%s' 0%08X (%s)\n", cbr->name, cbr->handler, cbr->symbol);
                        ml_register_cbr(cbr->name, (cbr_func)cbr->handler, 0);
                    }
                    else
                    {
                        printf("  [i] cbr '%s' -> 0%08X\n", cbr->name, cbr->handler);
                    }
                    cbr++;
                }
            }
            
            if(0)
            {
                printf("-----------------------------\n");
            }
            if (!module_list[mod].error)
            {
                snprintf(module_list[mod].status, sizeof(module_list[mod].status), "OK");
                snprintf(module_list[mod].long_status, sizeof(module_list[mod].long_status), "Module loaded successfully");
            }
        }
    }
    
    
    if(update_properties)
    {
        prop_update_registration();
    }

    module_update_core_symbols(state);
    
    #ifdef CONFIG_TCC_UNLOAD
    tcc_delete(state);
    #else
    module_state = state;
    #endif
    
    printf("Modules loaded\n");
}

static void _module_unload_all(void)
{
    /* unloading is not yet clean, we can end up with tasks running from freed memory or stuff like that */
    /* we will just call the "deinit" routine for now */
    /* for experiments on module unloading, see the module-unloading branch */
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        if(module_list[mod].valid && module_list[mod].enabled && !module_list[mod].error)
        {
            if(module_list[mod].info && module_list[mod].info->deinit)
            {
                module_list[mod].info->deinit();
                module_list[mod].valid = 0;
            }
            
            module_cbr_t *cbr = module_list[mod].cbr;
        
            /* register ml-cbr callback handlers */
            while(cbr && cbr->name)
            {
                /* unregister "named" callbacks through ml-cbr */
                if(cbr->type == CBR_NAMED)
                {
                    ml_unregister_cbr(cbr->name, (cbr_func)cbr->handler);
                }
                cbr++;
            }
        }
    }
}

void* module_load(char *filename)
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

unsigned int module_get_symbol(void *module, char *symbol)
{
#ifndef CONFIG_TCC_UNLOAD
    if (module == NULL) module = module_state;
#endif
    if (module == NULL) return 0;
    
    TCCState *state = (TCCState *)module;
    
    return (int) tcc_get_symbol(state, symbol);
}

int module_exec(void *module, char *symbol, int count, ...)
{
    int ret = -1;
#ifndef CONFIG_TCC_UNLOAD
    if (module == NULL) module = module_state;
#endif
    if (module == NULL) return ret;
    
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

            uint32_t parms[10];
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
int FAST module_exec_cbr(unsigned int type)
{
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        module_cbr_t *cbr = module_list[mod].cbr;
        if(module_list[mod].valid)
        {
            while(cbr && cbr->name)
            {
                if(cbr->type == type)
                {
                    int ret = cbr->handler(cbr->ctx);
                    
                    if (ret != CBR_RET_CONTINUE)
                    {
                        return ret;
                    }
                }
                cbr++;
            }
        }
    }
    
    return CBR_RET_CONTINUE;
}

/* translate camera specific key to portable module key */
#define MODULE_TRANSLATE_KEY(in,out,dest) if(dest == MODULE_KEY_PORTABLE) { if(in != -1){ if(key == in) { return out; } }} else {if(out != -1){ if(key == out) { return in; } }}

/* these are to ensure that the checked keys are defined. we have to ensure they're defined before using. are there better ways to ensure? */
#if !defined(BGMT_WHEEL_UP)
#define BGMT_WHEEL_UP -1
#endif
#if !defined(BGMT_WHEEL_DOWN)
#define BGMT_WHEEL_DOWN -1
#endif
#if !defined(BGMT_WHEEL_LEFT)
#define BGMT_WHEEL_LEFT -1
#endif
#if !defined(BGMT_WHEEL_RIGHT)
#define BGMT_WHEEL_RIGHT -1
#endif
#if !defined(BGMT_PRESS_SET)
#define BGMT_PRESS_SET -1
#endif
#if !defined(BGMT_UNPRESS_SET)
#define BGMT_UNPRESS_SET -1
#endif
#if !defined(BGMT_MENU)
#define BGMT_MENU -1
#endif
#if !defined(BGMT_INFO)
#define BGMT_INFO -1
#endif
#if !defined(BGMT_PLAY)
#define BGMT_PLAY -1
#endif
#if !defined(BGMT_TRASH)
#define BGMT_TRASH -1
#endif
#if !defined(BGMT_PRESS_DP)
#define BGMT_PRESS_DP -1
#endif
#if !defined(BGMT_UNPRESS_DP)
#define BGMT_UNPRESS_DP -1
#endif
#if !defined(BGMT_RATE)
#define BGMT_RATE -1
#endif
#if !defined(BGMT_REC)
#define BGMT_REC -1
#endif
#if !defined(BGMT_PRESS_ZOOM_IN)
#define BGMT_PRESS_ZOOM_IN -1
#endif
#if !defined(BGMT_LV)
#define BGMT_LV -1
#endif
#if !defined(BGMT_Q)
#define BGMT_Q -1
#endif
#if !defined(BGMT_PICSTYLE)
#define BGMT_PICSTYLE -1
#endif
#if !defined(BGMT_JOY_CENTER)
#define BGMT_JOY_CENTER -1
#endif
#if !defined(BGMT_PRESS_UP)
#define BGMT_PRESS_UP -1
#endif
#if !defined(BGMT_PRESS_UP_RIGHT)
#define BGMT_PRESS_UP_RIGHT -1
#endif
#if !defined(BGMT_PRESS_UP_LEFT)
#define BGMT_PRESS_UP_LEFT -1
#endif
#if !defined(BGMT_PRESS_RIGHT)
#define BGMT_PRESS_RIGHT -1
#endif
#if !defined(BGMT_PRESS_LEFT)
#define BGMT_PRESS_LEFT -1
#endif
#if !defined(BGMT_PRESS_DOWN_RIGHT)
#define BGMT_PRESS_DOWN_RIGHT -1
#endif
#if !defined(BGMT_PRESS_DOWN_LEFT)
#define BGMT_PRESS_DOWN_LEFT -1
#endif
#if !defined(BGMT_PRESS_DOWN)
#define BGMT_PRESS_DOWN -1
#endif
#if !defined(BGMT_UNPRESS_UDLR)
#define BGMT_UNPRESS_UDLR -1
#endif
#if !defined(BGMT_PRESS_HALFSHUTTER)
#define BGMT_PRESS_HALFSHUTTER -1
#endif
#if !defined(BGMT_UNPRESS_HALFSHUTTER)
#define BGMT_UNPRESS_HALFSHUTTER -1
#endif
#if !defined(BGMT_PRESS_FULLSHUTTER)
#define BGMT_PRESS_FULLSHUTTER -1
#endif
#if !defined(BGMT_UNPRESS_FULLSHUTTER)
#define BGMT_UNPRESS_FULLSHUTTER -1
#endif
#if !defined(BGMT_PRESS_FLASH_MOVIE)
#define BGMT_PRESS_FLASH_MOVIE -1
#endif
#if !defined(BGMT_UNPRESS_FLASH_MOVIE)
#define BGMT_UNPRESS_FLASH_MOVIE -1
#endif
#if !defined(BGMT_TOUCH_1_FINGER)
#define BGMT_TOUCH_1_FINGER -1
#endif
#if !defined(BGMT_UNTOUCH_1_FINGER)
#define BGMT_UNTOUCH_1_FINGER -1
#endif
#if !defined(BGMT_TOUCH_2_FINGER)
#define BGMT_TOUCH_2_FINGER -1
#endif
#if !defined(BGMT_UNTOUCH_2_FINGER)
#define BGMT_UNTOUCH_2_FINGER -1
#endif
int module_translate_key(int key, int dest)
{
    MODULE_TRANSLATE_KEY(BGMT_WHEEL_UP             , MODULE_KEY_WHEEL_UP             , dest);
    MODULE_TRANSLATE_KEY(BGMT_WHEEL_DOWN           , MODULE_KEY_WHEEL_DOWN           , dest);
    MODULE_TRANSLATE_KEY(BGMT_WHEEL_LEFT           , MODULE_KEY_WHEEL_LEFT           , dest);
    MODULE_TRANSLATE_KEY(BGMT_WHEEL_RIGHT          , MODULE_KEY_WHEEL_RIGHT          , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_SET            , MODULE_KEY_PRESS_SET            , dest);
    MODULE_TRANSLATE_KEY(BGMT_UNPRESS_SET          , MODULE_KEY_UNPRESS_SET          , dest);
    MODULE_TRANSLATE_KEY(BGMT_MENU                 , MODULE_KEY_MENU                 , dest);
    MODULE_TRANSLATE_KEY(BGMT_INFO                 , MODULE_KEY_INFO                 , dest);
    MODULE_TRANSLATE_KEY(BGMT_PLAY                 , MODULE_KEY_PLAY                 , dest);
    MODULE_TRANSLATE_KEY(BGMT_TRASH                , MODULE_KEY_TRASH                , dest);
    MODULE_TRANSLATE_KEY(BGMT_RATE                 , MODULE_KEY_RATE                 , dest);
    MODULE_TRANSLATE_KEY(BGMT_REC                  , MODULE_KEY_REC                  , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_ZOOM_IN   , MODULE_KEY_PRESS_ZOOMIN         , dest);
    MODULE_TRANSLATE_KEY(BGMT_LV                   , MODULE_KEY_LV                   , dest);
    MODULE_TRANSLATE_KEY(BGMT_PICSTYLE             , MODULE_KEY_PICSTYLE             , dest);
    MODULE_TRANSLATE_KEY(BGMT_JOY_CENTER           , MODULE_KEY_JOY_CENTER           , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_UP             , MODULE_KEY_PRESS_UP             , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_UP_RIGHT       , MODULE_KEY_PRESS_UP_RIGHT       , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_UP_LEFT        , MODULE_KEY_PRESS_UP_LEFT        , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_RIGHT          , MODULE_KEY_PRESS_RIGHT          , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_LEFT           , MODULE_KEY_PRESS_LEFT           , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_DOWN_RIGHT     , MODULE_KEY_PRESS_DOWN_RIGHT     , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_DOWN_LEFT      , MODULE_KEY_PRESS_DOWN_LEFT      , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_DOWN           , MODULE_KEY_PRESS_DOWN           , dest);
    MODULE_TRANSLATE_KEY(BGMT_UNPRESS_UDLR         , MODULE_KEY_UNPRESS_UDLR         , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_HALFSHUTTER    , MODULE_KEY_PRESS_HALFSHUTTER    , dest);
    MODULE_TRANSLATE_KEY(BGMT_UNPRESS_HALFSHUTTER  , MODULE_KEY_UNPRESS_HALFSHUTTER  , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_FULLSHUTTER    , MODULE_KEY_PRESS_FULLSHUTTER    , dest);
    MODULE_TRANSLATE_KEY(BGMT_UNPRESS_FULLSHUTTER  , MODULE_KEY_UNPRESS_FULLSHUTTER  , dest);
    MODULE_TRANSLATE_KEY(BGMT_TOUCH_1_FINGER       , MODULE_KEY_TOUCH_1_FINGER       , dest);
    MODULE_TRANSLATE_KEY(BGMT_UNTOUCH_1_FINGER     , MODULE_KEY_UNTOUCH_1_FINGER     , dest);
    MODULE_TRANSLATE_KEY(BGMT_TOUCH_2_FINGER       , MODULE_KEY_TOUCH_2_FINGER       , dest);
    MODULE_TRANSLATE_KEY(BGMT_UNTOUCH_2_FINGER     , MODULE_KEY_UNTOUCH_2_FINGER     , dest);
    MODULE_TRANSLATE_KEY(BGMT_Q                    , MODULE_KEY_Q                    , dest);
    MODULE_TRANSLATE_KEY(BGMT_PRESS_DP             , MODULE_KEY_PRESS_DP             , dest);
    MODULE_TRANSLATE_KEY(BGMT_UNPRESS_DP           , MODULE_KEY_UNPRESS_DP           , dest);
    /* these are not simple key codes, so they will not work with MODULE_TRANSLATE_KEY */
    //~ MODULE_TRANSLATE_KEY(BGMT_PRESS_FLASH_MOVIE    , MODULE_KEY_PRESS_FLASH_MOVIE    , dest);
    //~ MODULE_TRANSLATE_KEY(BGMT_UNPRESS_FLASH_MOVIE  , MODULE_KEY_UNPRESS_FLASH_MOVIE  , dest);
    
    return 0;
}
#undef MODULE_TRANSLATE_KEY

int module_send_keypress(int module_key)
{
    int key = module_translate_key(module_key, MODULE_KEY_CANON);
    switch (module_key)
    {
        case MODULE_KEY_PRESS_HALFSHUTTER:
            SW1(1,0);
            break;

        case MODULE_KEY_UNPRESS_HALFSHUTTER:
            SW1(0,0);
            break;

        case MODULE_KEY_PRESS_FULLSHUTTER:
            SW2(1,0);
            break;

        case MODULE_KEY_UNPRESS_FULLSHUTTER:
            SW1(0,0);
            break;
            
        default:
            fake_simple_button(key);
            break;
    }
    return 0;
}

int handle_module_keys(struct event * event)
{
    int count = 1;
    
    if (event->param == BGMT_WHEEL_UP || event->param == BGMT_WHEEL_DOWN || event->param == BGMT_WHEEL_LEFT ||  event->param == BGMT_WHEEL_RIGHT)
    {
        /* on newer cameras, scrollwheel events may come up grouped in a single event */
        /* since we only pass single key events to modules, we have to ungroup these events */
        /* (that is, call the handler as many times as needed) */
        count = MAX(count, event->arg);
    }
    
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        module_cbr_t *cbr = module_list[mod].cbr;
        if(module_list[mod].valid && cbr)
        {
            while(cbr->name)
            {
                if(cbr->type == CBR_KEYPRESS)
                {
                    int pass_event = 1;
                    /* one event may include multiple key presses - decompose it */
                    for (int i = 0; i < count; i++)
                    {
                        int portable_key = module_translate_key(event->param, MODULE_KEY_PORTABLE);
                        pass_event &= cbr->handler(portable_key);
                    }
                    if (!pass_event)
                    {
                        /* key handled */
                        return 0;
                    }
                }
                if(cbr->type == CBR_KEYPRESS_RAW)
                {
                    /* raw event includes counter - let's pass it only once */
                    int pass_event = cbr->handler((int)event);

                    if (!pass_event)
                    {
                        /* key handled */
                        return 0;
                    }
                }
                cbr++;
            }
        }
    }
    
    /* noone handled */
    return 1;
}

int module_display_filter_enabled()
{
#ifdef CONFIG_DISPLAY_FILTERS
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        module_cbr_t *cbr = module_list[mod].cbr;
        if(module_list[mod].valid && cbr)
        {
            while(cbr->name)
            {
                if(cbr->type == CBR_DISPLAY_FILTER)
                {
                    /* arg=0: should this display filter run? */
                    cbr->ctx = cbr->handler(0);
                    if (cbr->ctx)
                        return 1;
                }
                cbr++;
            }
        }
    }
#endif
    return 0;
}

int module_display_filter_update()
{
#ifdef CONFIG_DISPLAY_FILTERS
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        module_cbr_t *cbr = module_list[mod].cbr;
        if(module_list[mod].valid && cbr)
        {
            while(cbr->name)
            {
                /* run the first module display filter that returned 1 in module_display_filter_enabled */ 
                if(cbr->type == CBR_DISPLAY_FILTER && cbr->ctx)
                {
                    /* arg!=0: draw the filtered image in these buffers */
                    struct display_filter_buffers buffers;
                    display_filter_get_buffers((uint32_t**)&(buffers.src_buf), (uint32_t**)&(buffers.dst_buf));
                    
                    /* do not call the CBR with invalid arguments */
                    if (buffers.src_buf && buffers.dst_buf)
                    {
                        cbr->handler((intptr_t) &buffers);
                    }
                    
                    /* do not allow other display filters to run */
                    return 1;
                }
                cbr++;
            }
        }
    }
#endif
    return 0;
}

// Toggles the enable status of a module.
// Won't take effect until next restart, see _module_load_all()
void toggle_module_enabled(int mod_number)
{
    char enable_file[FIO_MAX_PATH_LENGTH];
    
    if (mod_number < 0 || mod_number > MODULE_COUNT_MAX)
        return;

    module_list[mod_number].enabled = !module_list[mod_number].enabled;
    snprintf(enable_file, sizeof(enable_file), "%s%s.en", get_config_dir(), module_list[mod_number].name);
    config_flag_file_setting_save(enable_file, module_list[mod_number].enabled);
    ASSERT(is_file(enable_file) == module_list[mod_number].enabled);
}

static MENU_SELECT_FUNC(module_menu_update_select)
{
    int mod_number = (int)priv;
    toggle_module_enabled(mod_number);

    // If we're disabling a module, nothing more to do.
    if (!module_list[mod_number].enabled)
        return;

    // We're enabling a module, check if it depends on any others.
    // If so, also enable those.

    const char *mod_name = module_list[mod_number].long_filename;

    extern void *tcc_load_offline_section(char *filename, char *section_name);
    char *mod_deps = tcc_load_offline_section(mod_name, ".module_deps");

    // If the section exists, mod_deps should be an array of null-terminated strings,
    // each a module name.  This is generated by mark_cross_module_deps.py during build.
    if (mod_deps != NULL)
    {
        char *mod_name = mod_deps;
        size_t name_len = strlen(mod_name);
        while (name_len != 0)
        {
            int dep_number = module_get_number(mod_name);
            // we don't want unloading a module to also unload deps,
            // users may separately want the features they provide
            if (!module_list[dep_number].enabled)
                toggle_module_enabled(dep_number);

            mod_name += (name_len + 1);
            name_len = strlen(mod_name);
        }
    }

    free(mod_deps);
}

static int startswith(const char* str, const char* prefix)
{
    const char* s = str;
    const char* p = prefix;
    for (; *p; s++,p++)
        if (*s != *p) return 0;
    return 1;
}

/* used if we don't have any module strings */
static module_strpair_t module_default_strings [] = {
    {0, 0}
};

static MENU_UPDATE_FUNC(module_menu_update_entry)
{
    int mod_number = (int) entry->priv;

    if(module_list[mod_number].valid)
    {
        if(module_list[mod_number].info && module_list[mod_number].info->long_name)
        {
            MENU_SET_NAME(module_list[mod_number].info->long_name);
        }
        else
        {
            MENU_SET_NAME(module_list[mod_number].name);
        }

        if (!module_list[mod_number].enabled)
        {
            MENU_SET_ICON(MNI_NEUTRAL, 0);
            MENU_SET_ENABLED(0);
            MENU_SET_VALUE("OFF, will not load");
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This module will no longer be loaded at next reboot.");
        }
        else
        {
            MENU_SET_ICON(MNI_ON, 0);
            MENU_SET_ENABLED(1);
            MENU_SET_VALUE(module_list[mod_number].status);
            if (module_list[mod_number].error)
            {
                MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "%s.", module_list[mod_number].long_status);
            }
            else
            {
                MENU_SET_WARNING(MENU_WARN_INFO, "%s. Press %s for more info.", module_list[mod_number].long_status, Q_BTN_NAME);
            }
        }
    }
    else if(strlen(module_list[mod_number].filename))
    {
        MENU_SET_NAME(module_list[mod_number].filename);
        str_make_lowercase(info->name);
        if (module_list[mod_number].enabled)
        {
            MENU_SET_ICON(MNI_ON, 0);
            MENU_SET_ENABLED(0);
            MENU_SET_VALUE("ON, will load");
            MENU_SET_WARNING(MENU_WARN_ADVICE, "This module will be loaded at next reboot.");
        }
        else
        {
            MENU_SET_ICON(MNI_OFF, 0);
            MENU_SET_ENABLED(1);
            MENU_SET_VALUE(module_list[mod_number].status);
            if (module_list[mod_number].strings && module_list[mod_number].strings != module_default_strings)
            {
                MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "%s. Press %s for more info.", module_list[mod_number].long_status, Q_BTN_NAME);
            }
            else
            {
                MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "%s.", module_list[mod_number].long_status);
            }
        }
    }
    else
    {
        MENU_SET_NAME("");
        MENU_SET_ICON(MNI_NONE, 0);
        MENU_SET_ENABLED(1);
        MENU_SET_VALUE("(nonexistent)");
        MENU_SET_HELP("You should never see this");
    }

    static int last_menu_activity_time = 0;
    static void* prev_selected = 0;
    if (entry->selected && entry != prev_selected)
    {
        last_menu_activity_time = get_ms_clock();
        prev_selected = entry;
    }

    if (entry->selected)
    {
        if (!module_list[mod_number].valid && !module_list[mod_number].strings)
        {
            char* fn = module_list[mod_number].long_filename;
            if (fn)
            {
                msg_queue_post(module_mq, MSG_MODULE_LOAD_OFFLINE_STRINGS | (mod_number << 16));
            }
        }
    }

    /* clean up offline strings if the module menu is no longer used */
    if (!entry->selected && get_ms_clock() > 3000 + last_menu_activity_time)
    {
        if (
                !module_list[mod_number].valid &&
                module_list[mod_number].strings && 
                module_list[mod_number].strings != module_default_strings
            )
        {
            /* module strings loaded from elf, module not loaded, clean them up */
            msg_queue_post(module_mq, MSG_MODULE_UNLOAD_OFFLINE_STRINGS | (mod_number << 16));
        }
    }

    /* show info based on module strings metadata */
    if (module_list[mod_number].strings)
    {
        const char* summary = module_get_string(mod_number, "Summary");
        if (summary)
        {
            int has_dot = summary[strlen(summary)-1] == '.';
            MENU_SET_HELP("%s%s", summary, has_dot ? "" : ".");
        }

        if (module_list[mod_number].valid == module_list[mod_number].enabled)
        {
            const char* name = module_get_string(mod_number, "Name");
            if (name)
            {
                int fg = COLOR_GRAY(40);
                int bg = COLOR_BLACK;
                int fnt = SHADOW_FONT(FONT(FONT_MED_LARGE, fg, bg));
                bmp_printf(fnt | FONT_ALIGN_RIGHT | FONT_TEXT_WIDTH(340), 680, info->y+2, "%s", name);
            }
        }
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
            ASSERT(mod_number == (int) entry->priv);

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

    /* make sure we have as many menu entries as modules */
    ASSERT(mod_number == MODULE_COUNT_MAX);
}

/* check which modules are loaded and hide others */
static void module_submenu_update(int mod_number)
{
    /* set autoload menu's priv to module id */
    module_submenu[0].priv = (void*)mod_number;
};

static int module_info_type = 0;

static MENU_SELECT_FUNC(module_info_toggle)
{
    module_info_type++;
    if (module_info_type > 1)
    {
        module_info_type = 0;
        menu_close_submenu();
    }
}

const char* module_get_string(int mod_number, const char* name)
{
    if(mod_number < 0 || mod_number >= MODULE_COUNT_MAX)
    {
        return NULL;
    }
    
    module_strpair_t *strings = module_list[mod_number].strings;

    if (strings)
    {
        for ( ; strings->name != NULL; strings++)
        {
            if (streq(strings->name, name))
            {
                return strings->value;
            }
        }
    }
    
    return NULL;
}

// returns -1 if name cannot be found
int module_get_number(const char *name)
{
    int i = 0;
    while(i < MODULE_COUNT_MAX)
    {
        if (strcmp(name, module_list[i].name) == 0)
        {
            return i;
        }
        i++;
    }
    return -1;
}

const char* module_get_name(int mod_number)
{
    if(mod_number < 0 || mod_number >= MODULE_COUNT_MAX)
    {
        return NULL;
    }
    
    return module_list[mod_number].name;
}

/*  returns the next loaded module id, or -1 when the end was reached.
    if passing -1 as the mod_number, it will return the first loaded module number.
*/
int module_get_next_loaded(int mod_number)
{
    if(mod_number < 0)
    {
        mod_number = -1;
    }
    
    while(1)
    {
        mod_number++;
        
        if(mod_number >= MODULE_COUNT_MAX)
        {
            return -1;
        }
        
        if(module_list[mod_number].valid && module_list[mod_number].enabled)
        {
            return mod_number;
        }
    }
}

static int module_is_special_string(const char* name)
{
    if (
            streq(name, "Name") ||
            streq(name, "Description") ||
            streq(name, "Build user") ||
            streq(name, "Build date") ||
            streq(name, "Last update") ||
            streq(name, "Summary") ||
            streq(name, "Forum") ||
            startswith(name, "Help page") ||
        0)
            return 1;
    return 0;
}

static int module_show_about_page(int mod_number)
{
    module_strpair_t *strings = module_list[mod_number].strings;

    if (strings)
    {
        int max_width = 0;
        int max_width_value = 0;
        int num_extra_strings = 0;
        for ( ; strings->name != NULL; strings++)
        {
            if (!module_is_special_string(strings->name))
            {
                max_width = MAX(max_width, strlen(strings->name) + strlen(strings->value) + 3);
                max_width_value = MAX(max_width_value, strlen(strings->value) + 2);
                num_extra_strings++;
            }
        }

        const char* desc = module_get_string(mod_number, "Description");
        const char* name = module_get_string(mod_number, "Name");
        const char* module_build_user = module_get_string(mod_number, "Build user");
        const char* module_build_date = module_get_string(mod_number, "Build date");
        const char* module_last_update = module_get_string(mod_number, "Last update");

        if (name && desc)
        {
            bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

            int fnt_special = FONT(FONT_MED, COLOR_CYAN, COLOR_BLACK);

            bmp_printf(FONT_LARGE, 10, 10, "%s", name);
            big_bmp_printf(FONT_MED | FONT_ALIGN_JUSTIFIED | FONT_TEXT_WIDTH(690), 10, 60, "%s", desc);

            int xm = 710 - max_width_value * font_med.width;
            int xl = 710 - max_width * font_med.width;
            xm = xm - xl + 10;
            xl = 10;

            int lines_for_update_msg;
            if (module_last_update == NULL)
                lines_for_update_msg = 2;
            else
                lines_for_update_msg = strchr(module_last_update, '\n') ? 3 : 2;

            int yr = 480 - (num_extra_strings + lines_for_update_msg) * font_med.height;

            for (strings = module_list[mod_number].strings ; strings->name != NULL; strings++)
            {
                if (!module_is_special_string(strings->name))
                {
                    bmp_printf(fnt_special, xl, yr, "%s", strings->name);
                    bmp_printf(fnt_special, xm, yr, ": %s", strings->value);
                    yr += font_med.height;
                }
            }
            
            if (module_build_date && module_build_user)
            {
                bmp_printf(fnt_special, 10, 480-font_med.height, "Built on %s by %s.", module_build_date, module_build_user);
            }
            
            if (module_last_update)
            {
                bmp_printf(fnt_special, 10, 480-font_med.height * lines_for_update_msg, "Last update: %s", module_last_update);
            }
            
            return 1;
        }
    }

    return 0;
}

static MENU_UPDATE_FUNC(module_menu_info_update)
{
    int mod_number = (int) module_submenu[0].priv;
    
    int x = info->x;
    int y = info->y;
    int x_val = info->x_val;
    if (!x || !y)
        return;

    info->custom_drawing = CUSTOM_DRAW_THIS_MENU;

    if (module_info_type == 0 && strlen(module_list[mod_number].long_filename))
    {
        /* try to show an About page for the module */
        if (module_show_about_page(mod_number))
            return;
    }
     
    /* make sure this module is being used */
    if(module_list[mod_number].valid && !module_list[mod_number].error)
    {
        module_strpair_t *strings = module_list[mod_number].strings;
        module_cbr_t *cbr = module_list[mod_number].cbr;
        module_prophandler_t **props = module_list[mod_number].prop_handlers;

        if (strings)
        {
            y += 10;
            bmp_printf(FONT_MED, x - 32, y, "Information:");
            y += font_med.height;
            for ( ; strings->name != NULL; strings++)
            {
                if (strings->value == NULL)
                    // strchr() will segfault if strings->value is NULL, at least make it easier
                    // to diagnose.
                    DryosDebugMsg(0, 15, "WARN: null strings->value");
                if (strchr(strings->value, '\n'))
                {
                    continue; /* don't display multiline strings here */
                }
                
                int is_short_string = strlen(strings->value) * font_med.width + x_val < 710;
                
                if (module_is_special_string(strings->name) && !is_short_string)
                {
                    continue; /* don't display long strings that are already shown on the info page */
                }
                
                bmp_printf(FONT_MED, x, y, "%s", strings->name);
                if (is_short_string)
                {
                    /* short string */
                    bmp_printf(FONT_MED, x_val, y, "%s", strings->value);
                }
                else
                {
                    /* long string */
                    if ((strlen(strings->name) + strlen(strings->value)) * font_med.width > 710)
                    {
                        /* doesn't fit? print on the next line */
                        y += font_med.height;
                    }
                    
                    /* right-align if possible */
                    int new_x = MAX(x, 710 - strlen(strings->value) * font_med.width);
                    bmp_printf(FONT_MED, new_x, y, "%s", strings->value);
                }
                y += font_med.height;
            }
        }
         
        if (props && *props)
        {
            y += 10;
            bmp_printf(FONT_MED, x - 32, y, 
                #if defined(CONFIG_UNREGISTER_PROP)
                "Properties:"
                #else
                "Properties (no support):"
                #endif
            );
            y += font_med.height;
            for (; *props != NULL; props++)
            {
                bmp_printf(FONT_MED, x, y, "%s", (*props)->name);
                y += font_med.height;
            }
        }

        if (cbr)
        {
            y += 10;
            bmp_printf(FONT_MED, x - 32, y, "Callbacks:");
            y += font_med.height;

            for ( ; cbr->name != NULL; cbr++)
            {
                bmp_printf(FONT_MED, x, y, "%s", cbr->name);
                bmp_printf(FONT_MED, x_val, y, "%s", cbr->symbol);
                y += font_med.height;
            }
        }
    }
    else
    {
        bmp_printf(FONT_MED, x - 32, y, "%s", module_list[mod_number].long_filename);
        y += font_med.height;
        bmp_printf(FONT_MED, x - 32, y, "More info after you load this module.");
    }
}

static MENU_SELECT_FUNC(module_open_submenu)
{
    int mod_number = (int)priv;
    module_submenu_update(mod_number);
    menu_open_submenu();
}

static MENU_SELECT_FUNC(console_toggle)
{
    module_console_enabled = !module_console_enabled;
    if (module_console_enabled)
        console_show();
    else
        console_hide();
}

static struct menu_entry module_submenu[] = {
        {
            .name = "Module info",
            .update = module_menu_info_update,
            .select = module_info_toggle,
            .icon_type = IT_ACTION,
        },
        MENU_EOL
};

#define MODULE_ENTRY(i) \
        { \
            .name = "Module", \
            .priv = (void*)i, \
            .select = module_menu_update_select, \
            .select_Q = module_open_submenu, \
            .update = module_menu_update_entry, \
            .submenu_width = 700, \
            .submenu_height = 400, \
            .children = module_submenu, \
        },

static struct menu_entry module_menu[] = {
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
    MODULE_ENTRY(15)
    MODULE_ENTRY(16)
    MODULE_ENTRY(17)
    MODULE_ENTRY(18)
    MODULE_ENTRY(19)
    MODULE_ENTRY(20)
    MODULE_ENTRY(21)
    MODULE_ENTRY(22)
    MODULE_ENTRY(23)
    MODULE_ENTRY(24)
    MODULE_ENTRY(25)
    MODULE_ENTRY(26)
    MODULE_ENTRY(27)
    MODULE_ENTRY(28)
    MODULE_ENTRY(29)
    MODULE_ENTRY(30)
    MODULE_ENTRY(31)
    MODULE_ENTRY(32)
    MODULE_ENTRY(33)
    MODULE_ENTRY(34)
    MODULE_ENTRY(35)
    MODULE_ENTRY(36)
    MODULE_ENTRY(37)
    MODULE_ENTRY(38)
    MODULE_ENTRY(39)
    MODULE_ENTRY(40)
    MODULE_ENTRY(41)
    MODULE_ENTRY(42)
    MODULE_ENTRY(43)
    MODULE_ENTRY(44)
    MODULE_ENTRY(45)
    MODULE_ENTRY(46)
    MODULE_ENTRY(47)
    MODULE_ENTRY(48)
    MODULE_ENTRY(49)
    MODULE_ENTRY(50)
    MODULE_ENTRY(51)
    MODULE_ENTRY(52)
    MODULE_ENTRY(53)
    MODULE_ENTRY(54)
    MODULE_ENTRY(55)
    MODULE_ENTRY(56)
    MODULE_ENTRY(57)
    MODULE_ENTRY(58)
    MODULE_ENTRY(59)
    MODULE_ENTRY(60)
    MODULE_ENTRY(61)
    MODULE_ENTRY(62)
    MODULE_ENTRY(63)
};

static struct menu_entry module_debug_menu[] = {
    {
        .name = "Show console",
        .priv = &module_console_enabled,
        .select = console_toggle,
        .max = 1,
        .help = "Keep console shown after modules were loaded",
    },
    {
        .name = "Modules debug",
        .select = menu_open_submenu,
        .submenu_width = 710,
        .help = "Diagnostic options for modules.",
        .children =  (struct menu_entry[]) {
            {
                 .name = "Disable all modules",
                 .priv = &module_autoload_disabled,
                 .max = 1,
                 .help = "For troubleshooting.",
            },
            {
                .name = "Load modules after crash",
                .priv = &module_ignore_crashes,
                .max = 1,
                .help = "Load modules even after camera crashed and you took battery out.",
            },
            MENU_EOL,
        },
    },
};

struct config_var * module_get_config_var(const char * name)
{
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        module_config_t * config = module_list[mod].config;
        if(module_list[mod].valid && config)
        {
            for (module_config_t * mconfig = config; mconfig && mconfig->name; mconfig++)
            {
                if (streq(mconfig->ref->name, name))
                    return (struct config_var *) mconfig->ref;
            }
        }
    }
    return 0;
}

struct config_var* module_config_var_lookup(int* ptr)
{
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        module_config_t * config = module_list[mod].config;
        if(module_list[mod].valid && config)
        {
            for (module_config_t * mconfig = config; mconfig && mconfig->name; mconfig++)
            {
                if (mconfig->ref->value == ptr)
                    return (struct config_var *) mconfig->ref;
            }
        }
    }
    return 0;
}

static void module_init()
{
    module_mq = (struct msg_queue *) msg_queue_create("module_mq", 10);
    menu_add("Modules", module_menu, COUNT(module_menu));
    menu_add("Debug", module_debug_menu, COUNT(module_debug_menu));
    module_menu_update();
}

static void module_load_offline_strings(int mod_number)
{
    if (!module_list[mod_number].strings)
    {
        /* default value, if it won't work */
        module_list[mod_number].strings = module_default_strings;
        
        char* fn = module_list[mod_number].long_filename;
        
        /* we should free this one after we are done with it */
        extern void* tcc_load_offline_section(char* filename, char* section_name);
        module_strpair_t * strings = tcc_load_offline_section(fn, ".module_strings");
 
        if (strings)
        {
            int looks_ok = 1;
            
            /* relocate strings from the unprocessed elf section */
            /* question: is the string structure always at the very beginning of the section? */
            for (module_strpair_t * str = strings; str->name != NULL; str++)
            {
                if ((intptr_t)str->name < 10000) /* does it look like a non-relocated pointer? */
                {
                    str->name = (void*) str->name + (intptr_t) strings;
                    str->value = (void*) str->value + (intptr_t) strings;
                }
                else
                {
                    looks_ok = 0;
                    break;
                }
            }
            
            if (looks_ok)
            {
                /* use as module strings */
                module_list[mod_number].strings = strings;
            }
        }
    }
}

static void module_unload_offline_strings(int mod_number)
{
    if (module_list[mod_number].strings)
    {
        free(module_list[mod_number].strings);
        module_list[mod_number].strings = 0;
    }
}

static void module_load_task(void* unused) 
{
    char *lockstr = "If you can read this, ML crashed last time. To save from faulty modules, autoload gets disabled.";

    if(!module_autoload_disabled)
    {
        uint32_t size;
        if(!module_ignore_crashes && FIO_GetFileSize( module_lockfile, &size ) == 0 )
        {
            /* uh, it seems the camera didnt shut down cleanly, skip module loading this time */
            msleep(1000);
            NotifyBox(10000, "Camera was not shut down cleanly.\r\nSkipping module loading." );
        }
        else
        {
            FILE *handle = FIO_CreateFile(module_lockfile);
            if (handle)
            {
                FIO_WriteFile(handle, lockstr, strlen(lockstr));
                FIO_CloseFile(handle);
            }
            
            /* now load modules */
            _module_load_all(0);
            module_menu_update();
        }
    }
    else
    {
        /* only list modules */
        _module_load_all(1);
        module_menu_update();
    }

    /* main loop, also wait until clean shutdown */
    TASK_LOOP
    {
        int msg;
        int err = msg_queue_receive(module_mq, (struct event**)&msg, 200);
        if (err) continue;
        
        switch(msg & 0xFFFF)
        {
            case MSG_MODULE_LOAD_OFFLINE_STRINGS:
            {
                int mod_number = msg >> 16;
                module_load_offline_strings(mod_number);
                break;
            }
            
            case MSG_MODULE_UNLOAD_OFFLINE_STRINGS:
            {
                int mod_number = msg >> 16;
                module_unload_offline_strings(mod_number);
                break;
            }
            
            default:
                printf("invalid msg: %d\n", msg);
        }
    }
}

void module_save_configs()
{
    /* save configuration */
    printf("Save configs...\n");
    for(int mod = 0; mod < MODULE_COUNT_MAX; mod++)
    {
        if(module_list[mod].valid && module_list[mod].enabled && !module_list[mod].error && module_list[mod].config)
        {
            /* save config */
            char filename[64];
            snprintf(filename, sizeof(filename), "%s%s.cfg", get_config_dir(), module_list[mod].name);

            uint32_t ret = module_config_save(filename, &module_list[mod]);
            if(ret)
            {
                printf("  [E] Error: %d\n", ret);
            }
        }
    }
}

/* clean shutdown, unlink lockfile */
int module_shutdown()
{
    _module_unload_all();
    
    if(!module_autoload_disabled)
    {
        /* remove lockfile */
        FIO_RemoveFile(module_lockfile);
    }
    return 0;
}

TASK_CREATE("module_task", module_load_task, 0, 0x1e, 0x4000 );

INIT_FUNC(__FILE__, module_init);

