//TODO: save prefix config

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <fileprefix.h>
#include <config.h>

#include "../ime_base/ime_base.h"

static char default_prefix[8];
static char prefix[8];
static int key = -1;

/* prefix has 4 characters - encode it as 32-bit integer */
static CONFIG_INT("image.prefix", prefix_enc, 0);

static IME_DONE_FUNC(file_prefix_enter_done)
{
    if (status == IME_OK)
    {
        /* if user entered less than 4 characters, fill with _ */
        prefix[4] = 0;
        int len = strlen(prefix);
        for (int i = len; i < 4; i++)
        {
            prefix[i] = '_';
        }

        /* any previously set prefix? */
        if (key != -1 && key != 0)
        {
            file_prefix_reset(key);
            key = -1;
        }

        /* set the new prefix, if different from default */
        if (!streq(prefix, default_prefix))
        {
            key = file_prefix_set(prefix);

            /* update encoded prefix (this goes into config file) */
            prefix_enc = *(int*)prefix;
        }
        else
        {
            /* disable user-set prefix */
            prefix_enc = 0;
        }
    }
    return IME_OK;
}

static IME_UPDATE_FUNC(file_prefix_enter_upd)
{
    return IME_OK;
}

static MENU_SELECT_FUNC(file_prefix_enter)
{
    snprintf(prefix, sizeof(prefix), "%s", get_file_prefix());

    ime_base_start(
        "Enter file prefix",
        prefix,
        5,
        IME_UTF8,
        IME_CHARSET_FILENAME,
        file_prefix_enter_upd,
        file_prefix_enter_done,
        0, 0, 0, 0
    );
}

static MENU_UPDATE_FUNC(file_prefix_upd)
{
    /* fixme: update menu backend so it doesn't require so many workarounds */
    switch (key)
    {
        case -1:
            MENU_SET_VALUE("OFF");
            MENU_SET_RINFO(default_prefix);
            MENU_SET_ENABLED(0);
            MENU_SET_ICON(MNI_DICE_OFF, 0);
            break;
        case 0:
            MENU_SET_VALUE("FAILED");
            MENU_SET_RINFO(get_file_prefix());
            MENU_SET_ICON(MNI_RECORD, 0);
            break;
        default:
            MENU_SET_ENABLED(1);
            MENU_SET_VALUE("%s", get_file_prefix());
            MENU_SET_RINFO("");
            MENU_SET_ICON(MNI_DICE_OFF, 1);
            break;
    }

    if ((void *) &ime_base_start == (void *) &ret_0)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Please load the IME modules (ime_base.mo and one ime_*.mo)");
    }
}

static int file_num = -1;
static int file_num_canon = -1;
static int file_num_requested = -1;
static int file_num_dirty = 0;

static int folder_num = -1;
static int folder_num_canon = -1;
static int folder_num_requested = -1;
static int folder_num_dirty = 0;

static void file_folder_number_refresh()
{
    int file_num_canon_latest = get_shooting_card()->file_number;
    int folder_num_canon_latest = get_shooting_card()->folder_number;
    if (file_num_canon != file_num_canon_latest)
    {
        /* Canon code updated their file number */
        if (file_num_canon_latest != file_num_requested)
        {
            /* maybe an image was taken? */
            /* fixme: move this logic into property handlers */
            file_num_dirty = 0;
            folder_num_dirty = 0;
            file_num = file_num_canon = file_num_canon_latest;
        }
    }

    ASSERT(file_num != -1);

    if (folder_num_canon != folder_num_canon_latest)
    {
        /* Canon code updated their folder number (e.g. file_num reached 9999) */
        if (folder_num_canon_latest != folder_num_requested)
        {
            folder_num = folder_num_canon = folder_num_canon_latest;
        }
    }

    ASSERT(folder_num != -1);

    char shooting_drive_letter = get_shooting_card()->drive_letter[0];

    if (folder_num != folder_num_canon_latest)
    {
        folder_num_requested = folder_num;
        printf("Changing folder number from %d to %d...\n", folder_num_canon_latest, folder_num_requested);
        int folder_number_prop = shooting_drive_letter == 'A' ? PROP_FOLDER_NUMBER_A : PROP_FOLDER_NUMBER_B;
        prop_request_change_wait(folder_number_prop, &folder_num_requested, 4, 1000);
        folder_num_dirty = 1;
    }

    if (file_num != file_num_canon_latest)
    {
        file_num_requested = file_num;
        printf("Changing file number from %d to %d...\n", file_num_canon_latest, file_num_requested);
        int file_number_prop = shooting_drive_letter == 'A' ? PROP_FILE_NUMBER_A : PROP_FILE_NUMBER_B;
        prop_request_change_wait(PROP_NUMBER_OF_CONTINUOUS_MODE, &file_num_requested, 4, 1000);
        prop_request_change_wait(file_number_prop, &file_num_requested, 4, 1000);
        file_num_dirty = 1;
    }

    /* re-read the (possibly updated) file and folder number from Canon */
    file_num = get_shooting_card()->file_number;
    folder_num = get_shooting_card()->folder_number;
}

static MENU_UPDATE_FUNC(file_number_upd)
{
    file_folder_number_refresh();

    MENU_SET_VALUE("%04d", file_num);

    if (file_num_dirty)
    {
        MENU_SET_RINFO("Restart");
        MENU_SET_WARNING(MENU_WARN_INFO, "Please restart your camera to apply the changes.");
    }
    else
    {
        MENU_SET_ENABLED(0);
    }
}

static const char * next_image_filename(int newline)
{
    static char buf[32];

    /* also print extension(s) based on picture quality setting */
    int raw = pic_quality & 0x60000;
    int jpg = pic_quality & 0x10000;

    snprintf(buf, sizeof(buf), 
        "%03d%s/%s%s%04d.%s%s%s",
        get_shooting_card()->folder_number + (get_shooting_card()->file_number == 9999 ? 1 : 0),
        get_dcim_dir_suffix(),
        newline ? "\n" : "",
        get_file_prefix(),
        get_shooting_card()->file_number < 9999 ? get_shooting_card()->file_number + 1 : 1,
        raw ? "CR2" : "",
        raw && jpg ? "/" : "",
        jpg ? "JPG" : ""
    );

    return buf;
}

static MENU_UPDATE_FUNC(folder_number_upd)
{
    file_folder_number_refresh();

    /* fixme: hackish */
    bmp_printf(FONT(FONT_LARGE, COLOR_GRAY(50), 0), 50, 350, "Next image: %s", next_image_filename(0));

    if (folder_num_dirty)
    {
        MENU_SET_RINFO("Take pic");
        MENU_SET_WARNING(MENU_WARN_INFO, "Please take a picture to apply the changes.");
    }
    else
    {
        MENU_SET_ENABLED(0);
    }
}

/* main entry */
static MENU_UPDATE_FUNC(file_name_upd)
{
    file_folder_number_refresh();

    MENU_SET_RINFO(next_image_filename(1));

    if (file_num_dirty)
    {
        MENU_SET_RINFO("Restart");
        MENU_SET_WARNING(MENU_WARN_INFO, "Please restart your camera to apply the changes.");
    }
    else if (folder_num_dirty)
    {
        MENU_SET_RINFO("Take pic");
        MENU_SET_WARNING(MENU_WARN_INFO, "Please take a picture to apply the changes.");
    }

    /* fixme: you can't change both at the same time */
}

static struct menu_entry img_name_menu[] =
{
    {
        .name           = "Image file naming",
        .select         = menu_open_submenu,
        .update         = file_name_upd,
        .help           = "Customize image file naming for still pictures. Experimental.", 
        .help2          = "Examples: IMG_1234.JPG -> ABCD1234.JPG, IMG_1234.JPG -> IMG_0001.JPG.",
        .submenu_width  = 720,
        .children       = (struct menu_entry[]) {
            {
                .name       = "Image file prefix",
                .select     = file_prefix_enter,
                .update     = file_prefix_upd,
                .icon_type  = IT_ACTION,
                .help       = "Custom image file prefix (e.g. IMG_1234.JPG -> ABCD1234.JPG).",
                .help2      = "Might conflict with Dual ISO prefixes (to be tested).",
            },
            {
                .name       = "Image file number",
                .priv       = &file_num,
                .min        = 0,            /* we can set to 0 and the next image will be IMG_0001 */
                .max        = 9999,         /* setting it to 9999 => next image will be IMG_0001, too */
                .unit       = UNIT_DEC,
                .icon_type  = IT_DICE,
                .update     = file_number_upd,
                .help       = "Custom image file number (e.g. IMG_1234.JPG -> IMG_5678.JPG).",
                .help2      = "You will need to restart the camera for the changes to take effect.",
            },
            {
                .name       = "Image folder number",
                .priv       = &folder_num,
                .update     = folder_number_upd,
                .min        = 100,
                .max        = 999,
                .unit       = UNIT_DEC,
                .icon_type  = IT_DICE,
                .help       = "Custom image folder num. You must take a picture to apply this setting.",
                .help2      = "DCIM/100CANON/IMG_1234.JPG -> DCIM/123CANON/IMG_1234.JPG",
            },
            MENU_EOL,
        },
    },
};

static unsigned int img_name_init()
{
    /* get default prefix at startup */
    /* fixme: what if user changes it from Canon menu? */
    snprintf(default_prefix, sizeof(default_prefix), "%s", get_file_prefix());

    /* set image prefix from config file, if any saved */
    if (prefix_enc)
    {
        /* note: CONFIG_INT's are of type int, which is int32_t on our target */
        /* fixme: compile time assert to check sizeof(int) */
        *(int*)prefix = prefix_enc; prefix[4] = 0;
        key = file_prefix_set(prefix);
    }

    /* setup menus */
    menu_add("Shoot", img_name_menu, COUNT(img_name_menu));

    return 0;
}

static unsigned int img_name_deinit()
{
    return 0;
}
    
MODULE_INFO_START()
    MODULE_INIT(img_name_init)
    MODULE_DEINIT(img_name_deinit)
MODULE_INFO_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(prefix_enc)
MODULE_CONFIGS_END()
