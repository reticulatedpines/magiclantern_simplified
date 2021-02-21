#include "dryos.h"
#include "property.h"
#include "bmp.h"
#include "string.h"
#include "menu.h"
#include "beep.h"
#include "config.h"
#include "fio-ml.h"

#define CARD_DRIVE_INIT(_letter, _type) { .drive_letter = _letter, .type = _type,  .cluster_size = 0, .free_space_raw = 0, .file_number = 0, .folder_number = 0, }

static struct card_info available_cards[] = { CARD_DRIVE_INIT("A","CF"), CARD_DRIVE_INIT("B", "SD"), CARD_DRIVE_INIT("C","EXT") };

#if defined(CONFIG_CF_SLOT)
static struct card_info * ML_CARD = &available_cards[CARD_A];
#else
static struct card_info * ML_CARD = &available_cards[CARD_B];
#endif

#if defined(CONFIG_DUAL_SLOT) || defined(CONFIG_CF_SLOT)
static struct card_info * SHOOTING_CARD = &available_cards[CARD_A];
#else
static struct card_info * SHOOTING_CARD = &available_cards[CARD_B];
#endif

// File I/O wrappers for handling the dual card slot on 5D3

static char dcim_dir_suffix[6];
static char dcim_dir[FIO_MAX_PATH_LENGTH];

/* enable to slow down the write speed, which improves compatibility with certain cards */
/* only enable if needed */
CONFIG_INT("cf.workaround", cf_card_workaround, 0);

#if 0
MENU_UPDATE_FUNC(card_info_display)
{
    //~ int pcmcia  = *(uint8_t*)0x68c88;
    //~ int ide     = *(uint8_t*)0x68c89;
    //~ int udma    = *(uint8_t*)0x68c8A;
    char* make  = (char*)0x68c8B;
    char* model = (char*)0x68cAA;
    int cf_present = is_dir("A:/");

    MENU_SET_VALUE("%s %s", cf_present ? make : "N/A", model);
    MENU_SET_ICON(cf_present ? MNI_ON : MNI_OFF, 0);
}
#endif

struct card_info* get_ml_card()
{
    return ML_CARD;
}


struct card_info* get_shooting_card()
{
    return SHOOTING_CARD;
}

struct card_info* get_card(int cardId)
{
    ASSERT(cardId >= 0 && cardId < 3);
    return &available_cards[cardId];
}

int get_free_space_32k(const struct card_info* card)
{
    return card->free_space_raw * (card->cluster_size>>10) / (32768>>10);
}


static CONFIG_INT("card.test", card_test_enabled, 1);
static CONFIG_INT("card.force_type", card_force_type, 1);

#ifndef CONFIG_INSTALLER
#ifdef CONFIG_5D3
static void card_test(struct card_info * card)
{
    // some cards have timing issues on 5D3
    // ML will test for this bug at startup, and refuse to run on cards that can cause trouble
    // http://www.magiclantern.fm/forum/index.php?topic=2528.0

    char drive_path[4];
    snprintf(drive_path, sizeof(drive_path), "%s:/", card->drive_letter);
    
    if (!cf_card_workaround)
    {
        /* save the config with workaround enabled now, because if the test fails, we may no longer able to save it */
        cf_card_workaround = 1;
        config_save();
        cf_card_workaround = 0;
    }

    if (is_dir(drive_path))
    {
        char testFile[] = "X:/test.dat";
        snprintf(testFile, sizeof(testFile), "%s:/test.dat", card->drive_letter);
        FILE* f = FIO_CreateFile(testFile);
        int fail = 0;
        for (int i = 0; i < 100; i++)
        {
            bmp_fill(COLOR_BLACK, 0, 0, 400, 38);
            char msg[50];
            snprintf(msg, sizeof(msg), "%s card test (%d%%)...", card->type, i+1);
            bmp_printf(FONT_CANON, 0, 0, msg);
            int r = FIO_WriteFile(f, (void*)YUV422_LV_BUFFER_1, 1025);
            if (r != 1025) { fail = 1; break; }
        }
        FIO_CloseFile(f);
        FIO_RemoveFile(testFile);
        bmp_fill(COLOR_BLACK, 0, 0, 400, 38);
        
        if (fail) // fsck!
        {
            int warning_enabling_workaround = (cf_card_workaround==0 && card->drive_letter[0] == 'A');
            while(1)
            {
                bmp_fill(COLOR_BLACK, 0, 0, 550, 80);
                if (warning_enabling_workaround)
                {
                    bmp_printf(FONT_CANON, 0,  0, "CF test fail, enabling workaround.");
                    bmp_printf(FONT_CANON, 0, 40, "Restart the camera to try again.");
                    cf_card_workaround = 1;
                }
                else
                {
                    bmp_printf(FONT_CANON, 0,  0, "%s card test failed!", card->type);
                    bmp_printf(FONT_CANON, 0, 40, "Do not use this card on 5D3!");
                }
                beep();
                info_led_blink(1, 1000, 1000);
            }
        }
        
        if (!cf_card_workaround)
        {
            /* test OK, save config once again to make sure we won't end up with the compatibility flag enabled by mistake */
            /* (might happen on a crash, or if you take the battery out) */
            config_save();
        }
    }
}
#endif

/** 
 * Called from debug_init_stuff
 */
void _card_tweaks()
{
#ifdef CONFIG_5D3
    if (card_test_enabled)
    {
        if (available_cards[CARD_A].free_space_raw > 10) card_test(&available_cards[CARD_A]);
        if (available_cards[CARD_B].free_space_raw > 10) card_test(&available_cards[CARD_B]);
        
        /* if it reaches this point, the cards are OK */
        card_test_enabled = 0;
    }
#endif
    
#ifdef CONFIG_DUAL_SLOT
    /* on startup enforce selected card.
       if that card type is not available, canon will ignore this change */
    if (card_force_type)
    {
        uint32_t value = card_force_type;
        
        /* ensure valid property value (side effect safe) */
        if ((value == 1 && is_dir("A:/")) ||
            (value == 2 && is_dir("B:/")))
        {
            prop_request_change(PROP_CARD_SELECT, &value, 4);
        }
    }
#endif
}
#endif  /* CONFIG_INSTALLER */

#ifdef CONFIG_5D3
static MENU_SELECT_FUNC(card_test_toggle)
{
    card_test_enabled = !card_test_enabled;
}

static MENU_UPDATE_FUNC(card_test_update)
{
    MENU_SET_VALUE(card_test_enabled ? "ON" : "OFF");
    MENU_SET_ICON(MNI_BOOL(card_test_enabled), 0);
    MENU_SET_ENABLED(card_test_enabled);
}
#endif

static void startup_warning(char* msg)
{
    /* note: this function is called before load_fonts, so in order to print something, we need to load them */
    _load_fonts();
    
    if (!DISPLAY_IS_ON)
    {
        /* force playback mode if we start with display off */
        SetGUIRequestMode(1);
        msleep(1000);
    }
    
    bmp_printf(FONT_LARGE, 0, 0, msg);
    redraw_after(5000);
}

void _find_ml_card()
{
    int ml_cf = is_dir("A:/ML");
    int ml_sd = is_dir("B:/ML");

    if (ml_cf && ml_sd)
    {
        /* ambiguity? let's look for autoexec.bin */
        ml_cf = is_file("A:/AUTOEXEC.BIN");
        ml_sd = is_file("B:/AUTOEXEC.BIN");
    }
    
    /* maybe todo: if both cards have autoexec, check which one is bootable? important? */

    if (ml_cf && !ml_sd)
    {
        ML_CARD = &available_cards[CARD_A];
    }
    else if (!ml_cf && ml_sd)
    {
        ML_CARD = &available_cards[CARD_B];
    }
    else if (ml_cf && ml_sd)
    {
        /* still ambiguity? */
        /* we know Canon loads autoexec.bin from the SD card first */
        ML_CARD = &available_cards[CARD_B];
        startup_warning("ML on both cards, loading from SD.");
    }
    else
    {
        startup_warning("Could not find ML files.");
    }
}

PROP_HANDLER(PROP_CARD_SELECT)
{
    int card_select = buf[0] - 1;
    if (card_select >= 0 && card_select < COUNT(available_cards))
    {
        SHOOTING_CARD = &available_cards[card_select];
        return;
    }
    ASSERT(0);
}

PROP_HANDLER(PROP_CLUSTER_SIZE_A)
{
    available_cards[CARD_A].cluster_size = buf[0];
}

PROP_HANDLER(PROP_CLUSTER_SIZE_B)
{
    available_cards[CARD_B].cluster_size = buf[0];
}

PROP_HANDLER(PROP_CLUSTER_SIZE_C)
{
    available_cards[CARD_C].cluster_size = buf[0];
}

PROP_HANDLER(PROP_FREE_SPACE_A)
{
    available_cards[CARD_A].free_space_raw = buf[0];
}

PROP_HANDLER(PROP_FREE_SPACE_B)
{
    available_cards[CARD_B].free_space_raw = buf[0];
}

PROP_HANDLER(PROP_FREE_SPACE_C)
{
    available_cards[CARD_C].free_space_raw = buf[0];
}

PROP_HANDLER(PROP_FILE_NUMBER_A)
{
    available_cards[CARD_A].file_number = buf[0];
}

PROP_HANDLER(PROP_FILE_NUMBER_B)
{
    available_cards[CARD_B].file_number = buf[0];
}

PROP_HANDLER(PROP_FILE_NUMBER_C)
{
    available_cards[CARD_C].file_number = buf[0];
}

PROP_HANDLER(PROP_FOLDER_NUMBER_A)
{
    available_cards[CARD_A].folder_number = buf[0];
}

PROP_HANDLER(PROP_FOLDER_NUMBER_B)
{
    available_cards[CARD_B].folder_number = buf[0];
}

PROP_HANDLER(PROP_FOLDER_NUMBER_C)
{
    available_cards[CARD_C].folder_number = buf[0];
}

PROP_HANDLER(PROP_DCIM_DIR_SUFFIX)
{
    snprintf(dcim_dir_suffix, sizeof(dcim_dir_suffix), (const char *)buf);
}

const char * get_dcim_dir_suffix()
{
    return dcim_dir_suffix;
}

const char* get_dcim_dir()
{
    snprintf(dcim_dir, sizeof(dcim_dir), "%s:/DCIM/%03d%s", SHOOTING_CARD->drive_letter, SHOOTING_CARD->folder_number, dcim_dir_suffix);
    return dcim_dir;
}

static void fixup_filename(char* new_filename, const char* old_filename, int size)
{
#define IS_IN_ML_DIR(filename)   (strncmp("ML/", filename, 3) == 0)
#define IS_IN_ROOT_DIR(filename) (filename[0] == '/' || !strchr(filename, '/'))
#define IS_DRV_PATH(filename)    (filename[1] == ':')
    char* drive_letter = ML_CARD->drive_letter;

    if (IS_DRV_PATH(old_filename))
    {
        strncpy(new_filename, old_filename, size-1);
        new_filename[size-1] = '\0';
        return;
    }

    if (!(IS_IN_ML_DIR(old_filename) || IS_IN_ROOT_DIR(old_filename)))
    {
        drive_letter = SHOOTING_CARD->drive_letter;
    }
    snprintf(new_filename, size, "%s:/%s", drive_letter, old_filename);
#undef IS_IN_ML_DIR
#undef IS_IN_ROOT_DIR
#undef IS_DRV_PATH
}

/* Canon stub */
/* note: it returns -1 on error, unlike fopen from plain C */
FILE* _FIO_OpenFile(const char* filename, unsigned mode );

/* this one returns 0 on error, just like in plain C */
FILE* FIO_OpenFile(const char* filename, unsigned mode )
{
    char new_filename[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_filename, filename, sizeof(new_filename));
    
    FILE* f = _FIO_OpenFile(new_filename, mode);
    
    if (f != PTR_INVALID)
    {
        return f;
    }
    
    return 0;
}

int _FIO_GetFileSize(const char * filename, uint32_t * size);
int FIO_GetFileSize(const char * filename, uint32_t * size)
{
    char new_filename[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_filename, filename, sizeof(new_filename));
    return _FIO_GetFileSize(new_filename, size);
}

int _FIO_RemoveFile(const char * filename);
int FIO_RemoveFile(const char * filename)
{
    char new_filename[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_filename, filename, sizeof(new_filename));
    return _FIO_RemoveFile(new_filename);
}

struct fio_dirent * _FIO_FindFirstEx(const char * dirname, struct fio_file * file);
struct fio_dirent * FIO_FindFirstEx(const char * dirname, struct fio_file * file)
{
    char new_dirname[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_dirname, dirname, sizeof(new_dirname));
    return _FIO_FindFirstEx(new_dirname, file);
}

int _FIO_CreateDirectory(const char * dirname);
int FIO_CreateDirectory(const char * dirname)
{
    char new_dirname[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_dirname, dirname, sizeof(new_dirname));
    if (is_dir(new_dirname)) return 0;
    return _FIO_CreateDirectory(new_dirname);
}

#if defined(CONFIG_FIO_RENAMEFILE_WORKS)
int _FIO_RenameFile(const char * src, const char * dst);
int FIO_RenameFile(const char * src, const char * dst)
{
    char newSrc[FIO_MAX_PATH_LENGTH];
    char newDst[FIO_MAX_PATH_LENGTH];
    fixup_filename(newSrc, src, FIO_MAX_PATH_LENGTH);
    fixup_filename(newDst, dst, FIO_MAX_PATH_LENGTH);
    return _FIO_RenameFile(newSrc, newDst);
}
#else
int FIO_RenameFile(const char * src, const char * dst)
{
    // FIO_RenameFile not known, or doesn't work
    // emulate it by copy + erase (poor man's rename :P )
    return FIO_MoveFile(src, dst);
}
#endif

static unsigned _GetFileSize(char* filename)
{
    uint32_t size;
    if( _FIO_GetFileSize( filename, &size ) != 0 )
        return 0xFFFFFFFF;
    return size;
}

uint32_t FIO_GetFileSize_direct(const char* filename)
{
    char new_filename[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_filename, filename, sizeof(new_filename));
    return _GetFileSize(new_filename);
}

static void _FIO_CreateDir_recursive(char* path)
{
    //~ NotifyBox(2000, "create dir: %s ", path); msleep(2000);
    // B:/ML/something
    
    if (is_dir(path)) return;

    int n = strlen(path);
    for (int i = n-1; i > 2; i--)
    {
        if (path[i] == '/')
        {
            path[i] = '\0';
            if (!is_dir(path))
                _FIO_CreateDir_recursive(path);
            path[i] = '/';
        }
    }

    _FIO_CreateDirectory(path);
}

/* Canon stub */
/* note: it returns -1 on error, unlike fopen from plain C */
FILE* _FIO_CreateFile(const char* filename );

/* a wrapper that also creates missing dirs and removes existing file */
/* this one returns 0 on error, just like in plain C */
static FILE* _FIO_CreateFileEx(const char* name)
{
    // first assume the path is alright
    _FIO_RemoveFile(name);
    FILE* f = _FIO_CreateFile(name);

    if (f != PTR_INVALID)
    {
        return f;
    }

    // if we are here, the path may be inexistent => create it
    int n = strlen(name);
    char* namae = (char*) name; // trick to ignore the const declaration and split the path easily
    for (int i = n-1; i > 2; i--)
    {
         if (namae[i] == '/')
         {
             namae[i] = '\0';
             _FIO_CreateDir_recursive(namae);
             namae[i] = '/';
         }
    }

    f = _FIO_CreateFile(name);

    if (f != PTR_INVALID)
    {
        return f;
    }
    
    /* return 0 on error, just like in plain C */
    return 0;
}

FILE* FIO_CreateFile(const char* name)
{
    char new_name[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_name, name, sizeof(new_name));
    return _FIO_CreateFileEx(new_name);
}

/* Canon stubs */
extern int _FIO_ReadFile( FILE* stream, void* ptr, size_t count );
extern int _FIO_WriteFile( FILE* stream, const void* ptr, size_t count );

int FIO_ReadFile( FILE* stream, void* ptr, size_t count )
{
    if (ptr == CACHEABLE(ptr))
    {
        /* there's a lot of existing code (e.g. mlv_play) that's hard to refactor */
        /* workaround: allocate DMA memory here (for small buffers only)
         * code that operates on large buffers should be already correct */
        if (!streq(get_current_task_name(), "run_test"))
        {
            printf("fixme: please use fio_malloc (in %s)\n", get_current_task_name());
        }
        ASSERT(count <= 8192);
        void * ubuf = fio_malloc(count);
        if (!ubuf)
        {
            ASSERT(0);
            return 0;
        }
        int ans = _FIO_ReadFile(stream, ubuf, count);
        memcpy(ptr, ubuf, count);
        free(ubuf);
        return ans;
    }

    return _FIO_ReadFile(stream, ptr, count);
}

int FIO_WriteFile( FILE* stream, const void* ptr, size_t count )
{
    /* we often assumed that the FIO routines will somehow care for buffers being still in cache.
       proven by the fact that even canon calls FIO_WriteFile with cached memory as source.
       that was simply incorrect. force cache flush if the buffer is a cachable one.
    */
    if (ptr == CACHEABLE(ptr))
    {
        /* write back all data to RAM */
        /* overhead is minimal (see selftest.mo for benchmark) */
        sync_caches();
    }

    return _FIO_WriteFile(stream, ptr, count);
}

FILE* FIO_CreateFileOrAppend(const char* name)
{
    /* credits: https://bitbucket.org/dmilligan/magic-lantern/commits/d7e0245b1c62c26231799e9be3b54dd77d51a283 */
    FILE * f = FIO_OpenFile(name, O_RDWR | O_SYNC);
    if (!f)
    {
        f = FIO_CreateFile(name);
    }
    else
    {
        FIO_SeekSkipFile(f,0,SEEK_END);
    }
    return f;
}

int FIO_CopyFile(const char * src, const char * dst)
{
    FILE* f = FIO_OpenFile(src, O_RDONLY | O_SYNC);
    if (!f) return -1;

    FILE* g = FIO_CreateFile(dst);
    if (!g) { FIO_CloseFile(f); return -1; }

    const int bufsize = MIN(FIO_GetFileSize_direct(src), 128*1024);
    void* buf = fio_malloc(bufsize);
    if (!buf) return -1;

    int err = 0;
    int r = 0;
    while ((r = FIO_ReadFile(f, buf, bufsize)))
    {
        int w = FIO_WriteFile(g, buf, r);
        if (w != r)
        {
            /* copy failed; abort and delete the incomplete file */
            err = 1;
            break;
        }
    }

    FIO_CloseFile(f);
    FIO_CloseFile(g);
    fio_free(buf);
    
    if (err)
    {
        FIO_RemoveFile(dst);
        return -1;
    }
    
    /* all OK */
    return 0;
}

int FIO_MoveFile(const char * src, const char * dst)
{
    int err = FIO_CopyFile(src,dst);
    if (!err)
    {
        /* file copied, we can remove the old one */
        FIO_RemoveFile(src);
        return 0;
    }
    else
    {
        /* something went wrong; keep the old file and return error code */
        return err;
    }
}

int is_file(const char* path)
{
    uint32_t file_size = 0;
    return !FIO_GetFileSize(path, &file_size);
}

int is_dir(const char* path)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( path, &file );
    if( IS_ERROR(dirent) )
    {
        return 0; // this dir does not exist
    }
    else 
    {
        FIO_FindClose(dirent);
        return 1; // dir found
    }
}

int get_numbered_file_name(const char* pattern, int nmax, char* filename, int maxlen)
{
    for (int num = 0; num <= nmax; num++)
    {
        snprintf(filename, maxlen, pattern, num);
        uint32_t size;
        if( FIO_GetFileSize( filename, &size ) != 0 ) return num;
        if (size == 0) return num;
    }

    snprintf(filename, maxlen, pattern, 0);
    return -1;
}

void dump_seg(void* start, uint32_t size, char* filename)
{
    DEBUG();
    FILE* f = FIO_CreateFile(filename);
    if (f)
    {
        FIO_WriteFile( f, start, size );
        FIO_CloseFile(f);
    }
    DEBUG();
}

void dump_big_seg(int k, char* filename)
{
    DEBUG();
    FILE* f = FIO_CreateFile(filename);
    if (f)
    {
        for (int i = 0; i < 16; i++)
        {
            DEBUG();
            uint32_t start = (k << 28 | i << 24);
            bmp_printf(FONT_LARGE, 50, 50, "Saving %8x...", start);
            FIO_WriteFile( f, (const void *) start, 0x1000000 );
        }
        FIO_CloseFile(f);
    }
    DEBUG();
}


size_t
read_file(
    const char *        filename,
    void *            buf,
    size_t            size
)
{
    FILE * file = FIO_OpenFile( filename, O_RDONLY | O_SYNC );
    if (!file)
        return -1;
    unsigned rc = FIO_ReadFile( file, buf, size );
    FIO_CloseFile( file );
    return rc;
}

uint8_t* read_entire_file(const char * filename, int* buf_size)
{
    *buf_size = 0;
    uint32_t size;
    if( FIO_GetFileSize( filename, &size ) != 0 )
        goto getfilesize_fail;

    DEBUG("File '%s' size %d bytes", filename, size);

    uint8_t * buf = fio_malloc( size + 1);
    if( !buf )
    {
        DebugMsg( DM_MAGIC, 3, "%s: fio_malloc failed", filename );
        goto malloc_fail;
    }
    size_t rc = read_file( filename, buf, size );
    if( rc != size )
        goto read_fail;

    *buf_size = size;

    buf[size] = 0; // null-terminate text files

    return buf;

//~ fail_buf_copy:
read_fail:
    fio_free( buf );
malloc_fail:
getfilesize_fail:
    DEBUG("failed");
    return NULL;
}

// sometimes gcc likes very much the default fprintf and uses that one
// => renamed to my_fprintf to force it to use this one
int
my_fprintf(
    FILE *          file,
    const char *        fmt,
    ...
)
{
    va_list         ap;
    int len = 0;
    
    const int maxlen = 512;
    char buf[maxlen];

    va_start( ap, fmt );
    len = vsnprintf( buf, maxlen-1, fmt, ap );
    va_end( ap );
    FIO_WriteFile( file, buf, len );
    
    return len;
}

#ifdef CONFIG_DUAL_SLOT
struct menu_entry card_menus[] = {
    {
        .name = "Card settings",
        .select = menu_open_submenu,
        .help = "Preferences related to SD/CF card operation.",
        .children =  (struct menu_entry[]) {
            /*
            {
                .name = "CF card", 
                .update = &card_info_display,
                .help = "CF card info: make and model."
            },*/
#ifdef CONFIG_5D3
            {
                .name = "Card test at startup", 
                //~ .priv = &card_test_enabled, /* don't use priv, so it doesn't get displayed in the modified settings menu */
                .select = card_test_toggle,
                .update = card_test_update,
                .help = "File write test. Some cards may have compatibility issues.",
            },
            {
                .name = "CF card workaround",
                .priv = &cf_card_workaround,
                .max = 1,
                .help = "Slows down the CF write speed to let you use certain cards.",
                .help2 = "(e.g. Kingston 16GB 266x is known to require this)"
            },
#endif
            {
                .name = "Preferred card", 
                .priv = &card_force_type,
                .min = 0,
                .max = 2,
                .choices = CHOICES("OFF", "CF", "SD"),
                .help = "Make sure your preferred card is selected at startup."
            },
            MENU_EOL,
        }
    }
};
#endif

static void fio_init()
{
    #ifdef CONFIG_DUAL_SLOT
    menu_add( "Prefs", card_menus, COUNT(card_menus) );
    #endif
    
    #ifdef CARD_A_MAKER
    available_cards[CARD_A].maker = (char*) CARD_A_MAKER;
    available_cards[CARD_A].model = (char*) CARD_A_MODEL;
    #endif
}


INIT_FUNC(__FILE__, fio_init);
