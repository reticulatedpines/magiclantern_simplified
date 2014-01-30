/**\file
 * EyeFi Tricks (EyeFi confirmed working only on 600D-60D)
 */

#include "tweaks-eyefi.h"

#ifdef FEATURE_EYEFI_TRICKS

int check_eyefi()
{
    FILE * f = FIO_Open(CARD_DRIVE "EYEFI/REQC", 0);
    if (f != (void*) -1)
    {
        FIO_CloseFile(f);
        return 1;
    }
    return 0;
}

void EyeFi_RenameCR2toAVI(char* dir)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.mode & 0x10) continue; // is a directory
        if (file.name[0] == '.') continue;
        if (!streq(file.name + 8, ".CR2")) continue;

        static char oldname[50];
        static char newname[50];
        snprintf(oldname, sizeof(oldname), "%s/%s", dir, file.name);
        strcpy(newname, oldname);
        newname[strlen(newname) - 4] = 0;
        STR_APPEND(newname, ".AVI");
        bmp_printf(FONT_LARGE, 0, 0, "%s...", newname);
        FIO_RenameFile(oldname, newname);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);
    beep();
    redraw();
}

void EyeFi_RenameAVItoCR2(char* dir)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.mode & 0x10) continue; // is a directory
        if (file.name[0] == '.') continue;
        if (!streq(file.name + 8, ".AVI")) continue;

        static char oldname[50];
        static char newname[50];
        snprintf(oldname, sizeof(oldname), "%s/%s", dir, file.name);
        strcpy(newname, oldname);
        newname[strlen(newname) - 4] = 0;
        STR_APPEND(newname, ".CR2");
        bmp_printf(FONT_LARGE, 0, 0, "%s...", newname);
        FIO_RenameFile(oldname, newname);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);
    beep();
    redraw();
}

static void CR2toAVI(void* priv, int delta)
{
    EyeFi_RenameCR2toAVI((char*)get_dcim_dir());
}

static void AVItoCR2(void* priv, int delta)
{
    EyeFi_RenameAVItoCR2((char*)get_dcim_dir());
}

#ifdef FEATURE_EYEFI_RENAME_422_MP4
void EyeFi_Rename422toMP4(char* dir)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.mode & 0x10) continue; // is a directory
        if (file.name[0] == '.') continue;
        if (!streq(file.name + 8, ".422")) continue;

        static char oldname[50];
        static char newname[50];
        snprintf(oldname, sizeof(oldname), "%s/%s", dir, file.name);
        strcpy(newname, oldname);
        newname[strlen(newname) - 4] = 0;
        STR_APPEND(newname, ".MP4");
        bmp_printf(FONT_LARGE, 0, 0, "%s...", newname);
        FIO_RenameFile(oldname, newname);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);
    beep();
    redraw();
}

void EyeFi_RenameMP4to422(char* dir)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.mode & 0x10) continue; // is a directory
        if (file.name[0] == '.') continue;
        if (!streq(file.name + 8, ".MP4")) continue;

        static char oldname[50];
        static char newname[50];
        snprintf(oldname, sizeof(oldname), "%s/%s", dir, file.name);
        strcpy(newname, oldname);
        newname[strlen(newname) - 4] = 0;
        STR_APPEND(newname, ".422");
        bmp_printf(FONT_LARGE, 0, 0, "%s...", newname);
        FIO_RenameFile(oldname, newname);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);
    beep();
    redraw();
}
static void f422toMP4(void* priv, int delta)
{
    EyeFi_Rename422toMP4(get_dcim_dir());
}

static void MP4to422(void* priv, int delta)
{
    EyeFi_RenameMP4to422(get_dcim_dir());
}

#endif // FEATURE_EYEFI_RENAME_422_MP4

static struct menu_entry eyefi_menus[] = {
    {
        .name        = "EyeFi Trick",
        .select        = menu_open_submenu,
        .help = "Rename CR2 files to AVI (trick for EyeFi cards).",
        .children =  (struct menu_entry[]) {
            {
            	.name        = "Rename CR2 to AVI",
            	.select        = CR2toAVI,
            	.help = "Rename CR2 files to AVI (trick for EyeFi cards)."
         	},
            {
            	.name        = "Rename AVI to CR2",
            	.select        = AVItoCR2,
            	.help = "Rename back AVI files to CR2 (trick for EyeFi cards)."
         	},
            #ifdef FEATURE_EYEFI_RENAME_422_MP4
            {
            	.name        = "Rename 422 to MP4",
            	.select        = f422toMP4,
            	.help = "Rename 422 files to MP4 (trick for EyeFi cards)."
         	},
            {
            	.name        = "Rename MP4 to 422",
            	.select        = MP4to422,
            	.help = "Rename back MP4 files to 422 (trick for EyeFi cards)."
         	},
            #endif
            MENU_EOL
        },
    },
};

static void eyefi_tweak_init()
{
    if (check_eyefi()) {
        menu_add( "Shoot", eyefi_menus, COUNT(eyefi_menus) );
    }
}

INIT_FUNC(__FILE__, eyefi_tweak_init);
#endif // FEATURE_EYEFI_TRICKS
