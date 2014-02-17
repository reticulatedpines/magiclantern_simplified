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
static char dcim_dir[100];

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
static CONFIG_INT("card.force_type", card_force_type, 0);

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
        FILE* f = FIO_CreateFileEx(testFile);
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
                    bmp_printf(FONT_CANON, 0, 40, "Restart the camera and try again.");
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
    }
}
#endif

/** 
 * Called from debug_init_stuff
 */
void card_tweaks()
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

void find_ml_card()
{
    int ml_cf = is_dir("A:/ML");
    int ml_sd = is_dir("B:/ML");
    
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
        clrscr();
        for (int i = 0; i < 5; i++)
        {
            bmp_printf(FONT_CANON, 0, 0, "ML is on both cards, format one of them!");
            msleep(1000);
            beep();
        }
        redraw_after(2000);
    }
    else
    {
        clrscr();
        for (int i = 0; i < 5; i++)
        {
            bmp_printf(FONT_CANON, 0, 0, "Could not find ML files.");
            msleep(1000);
            beep();
        }
        redraw_after(2000);
    }
}

PROP_HANDLER(PROP_CARD_SELECT)
{
    int card_select = buf[0] - 1;
    ASSERT(card_select >= 0 && card_select < 3)
    SHOOTING_CARD = &available_cards[buf[0]-1];
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

const char* get_dcim_dir()
{
    snprintf(dcim_dir, sizeof(dcim_dir), "%s:/DCIM/%03d%s", SHOOTING_CARD->drive_letter, SHOOTING_CARD->folder_number, dcim_dir_suffix);
    return dcim_dir;
}

static void guess_drive_letter(char* new_filename, const char* old_filename, int size)
{
    if (old_filename[1] == ':')
    {
        snprintf(new_filename, size, "%s", old_filename);
        return;
    }
    
    if ((old_filename[0] == 'M' && old_filename[1] == 'L' && old_filename[2] == '/') // something in ML dir
        || !strchr(old_filename, '/')) // something in root dir
    {
        snprintf(new_filename, 100, "%s:/%s", ML_CARD->drive_letter, old_filename);
    }
    else
    {
        snprintf(new_filename, 100, "%s:/%s", SHOOTING_CARD->drive_letter, old_filename);
    }
}

FILE* _FIO_Open(const char* filename, unsigned mode );
FILE* FIO_Open(const char* filename, unsigned mode )
{
    char new_filename[100];
    guess_drive_letter(new_filename, filename, 100);
    return _FIO_Open(new_filename, mode);
}

FILE* _FIO_CreateFile(const char* filename );
FILE* FIO_CreateFile(const char* filename )
{
    char new_filename[100];
    guess_drive_letter(new_filename, filename, 100);
    return _FIO_CreateFile(new_filename);
}

//~ int _FIO_GetFileSize(const char * filename, unsigned * size);
int FIO_GetFileSize(const char * filename, uint32_t * size)
{
    char new_filename[100];
    guess_drive_letter(new_filename, filename, 100);
    return _FIO_GetFileSize(new_filename, size);
}

int _FIO_RemoveFile(const char * filename);
int FIO_RemoveFile(const char * filename)
{
    char new_filename[100];
    guess_drive_letter(new_filename, filename, 100);
    return _FIO_RemoveFile(new_filename);
}

struct fio_dirent * _FIO_FindFirstEx(const char * dirname, struct fio_file * file);
struct fio_dirent * FIO_FindFirstEx(const char * dirname, struct fio_file * file)
{
    char new_dirname[100];
    guess_drive_letter(new_dirname, dirname, 100);
    return _FIO_FindFirstEx(new_dirname, file);
}

int _FIO_CreateDirectory(const char * dirname);
int FIO_CreateDirectory(const char * dirname)
{
    char new_dirname[100];
    guess_drive_letter(new_dirname, dirname, 100);
    return _FIO_CreateDirectory(new_dirname);
}

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
#ifdef CONFIG_DUAL_SLOT
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
#endif
            MENU_EOL,
        }
    }
};

static void fio_init()
{
    menu_add( "Prefs", card_menus, COUNT(card_menus) );
}


INIT_FUNC(__FILE__, fio_init);
