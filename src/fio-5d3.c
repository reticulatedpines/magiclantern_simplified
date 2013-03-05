#include "dryos.h"
#include "property.h"
#include "bmp.h"
#include "string.h"
#include "menu.h"
#include "config.h"

// File I/O wrappers for handling the dual card slot on 5D3

int ml_card_select = 2; // if autoexec.bin is on both cards, the one from SD is loaded
int card_select = 1;

#define SHOOTING_CARD_LETTER (card_select == 1 ? "A" : "B")
#define ML_CARD_LETTER (ml_card_select == 1 ? "A" : "B")

CONFIG_INT("card.test", card_test_enabled, 1);

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

void card_test(int type)
{
    // some cards have timing issues on 5D3
    // ML will test for this bug at startup, and refuse to run on cards that can cause trouble
    // http://www.magiclantern.fm/forum/index.php?topic=2528.0
    
    if (is_dir(type ? "B:/" : "A:/"))
    {
        FILE* f = FIO_CreateFileEx(type ? "B:/test.dat" : "A:/test.dat");
        int fail = 0;
        for (int i = 0; i < 100; i++)
        {
            bmp_fill(COLOR_BLACK, 0, 0, 400, 38);
            char msg[50];
            snprintf(msg, sizeof(msg), "%s card test (%d%%)...", type ? "SD" : "CF", i+1);
            bfnt_puts(msg, 0, 0, COLOR_WHITE, COLOR_BLACK);
            int r = FIO_WriteFile(f, (void*)YUV422_LV_BUFFER_1, 1025);
            if (r != 1025) { fail = 1; break; }
        }
        FIO_CloseFile(f);
        FIO_RemoveFile(type ? "B:/test.dat" : "A:/test.dat");
        bmp_fill(COLOR_BLACK, 0, 0, 400, 38);
        
        if (fail) // fsck!
        {
            while(1)
            {
                bmp_fill(COLOR_BLACK, 0, 0, 550, 80);
                bfnt_puts(type ? "SD card test failed!" : "CF card test failed!", 0, 0, COLOR_WHITE, COLOR_BLACK);
                bfnt_puts("Do not use this card on 5D3!", 0, 40, COLOR_WHITE, COLOR_BLACK);
                beep();
                info_led_blink(1, 1000, 1000);
            }
        }
    }
}

void card_tests()
{
    if (card_test_enabled)
    {
        card_test(0);
        card_test(1);
    }
}

void find_ml_card()
{
    int ml_cf = is_dir("A:/ML");
    int ml_sd = is_dir("B:/ML");
    
    if (ml_cf && !ml_sd) ml_card_select = 1;
    else if (!ml_cf && ml_sd) ml_card_select = 2;
    else if (ml_cf && ml_sd)
    {
        clrscr();
        for (int i = 0; i < 5; i++)
        {
            bfnt_puts("ML is on both cards, format one of them!", 0, 0, COLOR_WHITE, COLOR_BLACK);
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
            bfnt_puts("Could not find ML files.", 0, 0, COLOR_WHITE, COLOR_BLACK);
            msleep(1000);
            beep();
        }
        redraw_after(2000);
    }
    
    card_select = ml_card_select;
}

int cluster_size = 0;
int cluster_size_a = 0;
int cluster_size_b = 0;

int free_space_raw = 0;
int free_space_raw_a = 0;
int free_space_raw_b = 0;

int file_number = 0;
int file_number_a = 0;
int file_number_b = 0;

int folder_number = 0;
int folder_number_a = 0;
int folder_number_b = 0;

PROP_HANDLER(PROP_CARD_SELECT)
{
    card_select = buf[0];
    if (card_select == 1)
    {
        cluster_size = cluster_size_a;
        free_space_raw = free_space_raw_a;
        file_number = file_number_a;
        folder_number = folder_number_a;
    }
    else
    {
        cluster_size = cluster_size_b;
        free_space_raw = free_space_raw_b;
        file_number = file_number_b;
        folder_number = folder_number_b;
    }
}

PROP_HANDLER(PROP_CLUSTER_SIZE_A)
{
    cluster_size_a = buf[0];
    if (card_select == 1) cluster_size = buf[0];
}

PROP_HANDLER(PROP_CLUSTER_SIZE_B)
{
    cluster_size_b = buf[0];
    if (card_select == 2) cluster_size = buf[0];
}

PROP_HANDLER(PROP_FREE_SPACE_A)
{
    free_space_raw_a = buf[0];
    if (card_select == 1) free_space_raw = buf[0];
}

PROP_HANDLER(PROP_FREE_SPACE_B)
{
    free_space_raw_b = buf[0];
    if (card_select == 2) free_space_raw = buf[0];
}

PROP_HANDLER(PROP_FILE_NUMBER_A)
{
    file_number_a = buf[0];
    if (card_select == 1) file_number = buf[0];
}

PROP_HANDLER(PROP_FILE_NUMBER_B)
{
    file_number_b = buf[0];
    if (card_select == 2) file_number = buf[0];
}

PROP_HANDLER(PROP_FOLDER_NUMBER_A)
{
    folder_number_a = buf[0];
    if (card_select == 1) folder_number = buf[0];
}

PROP_HANDLER(PROP_FOLDER_NUMBER_B)
{
    folder_number_b = buf[0];
    if (card_select == 2) folder_number = buf[0];
}

static char dcim_dir_suffix[6];
static char dcim_dir[100];

PROP_HANDLER(PROP_DCIM_DIR_SUFFIX)
{
    snprintf(dcim_dir_suffix, sizeof(dcim_dir_suffix), (const char *)buf);
}

const char* get_dcim_dir()
{
    snprintf(dcim_dir, sizeof(dcim_dir), "%s:/DCIM/%03d%s", SHOOTING_CARD_LETTER, folder_number, dcim_dir_suffix);
    return dcim_dir;
}

static void guess_drive_letter(char* new_filename, const char* old_filename, int size)
{
    if (old_filename[1] == ':')
    {
        snprintf(new_filename, size, "%s", old_filename);
        return;
    }
    
    if (old_filename[0] == 'M' && old_filename[1] == 'L' && old_filename[2] == '/') // something in ML dir
    {
        snprintf(new_filename, 100, "%s:/%s", ML_CARD_LETTER, old_filename);
    }
    else
    {
        snprintf(new_filename, 100, "%s:/%s", SHOOTING_CARD_LETTER, old_filename);
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

INIT_FUNC("fio", find_ml_card);

struct menu_entry card_menus[] = {
    /*
    {
        .name = "CF card", 
        .update = &card_info_display,
        .help = "CF card info: make and model."
    },*/
    {
        .name = "Card test at startup", 
        .priv = &card_test_enabled,
        .max = 1,
        .help = "File write test. Disable ONLY after testing ALL your cards!"
    },
};
