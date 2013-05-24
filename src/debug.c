/** \file
 * Magic Lantern debugging and reverse engineering code
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"
#include "plugin.h"
#include "version.h"
#include "edmac.h"
#include "asm.h"

#ifdef FEATURE_DEBUG_INTERCEPT
#include "dm-spy.h"
#include "tp-spy.h"
#endif

#ifdef FEATURE_SHOW_SIGNATURE
#include "fw-signature.h"
#endif

#ifdef CONFIG_MODULES
#include "module.h"
#endif
//#include "lua.h"

#if defined(CONFIG_7D)
#include "ml_rpc.h"
#endif

#if defined(CONFIG_600D) && defined(CONFIG_AUDIO_600D_DEBUG)
void audio_reg_dump_once();
#endif

#if defined(CONFIG_EDMAC_MEMCPY)
#include "edmac-memcpy.h"
#endif

extern int config_autosave;
extern void config_autosave_toggle(void* unused, int delta);

static struct semaphore * beep_sem = 0;
static struct semaphore * config_save_sem = 0;

static void debug_init_func()
{
    beep_sem = create_named_semaphore("beep_sem",1);
    config_save_sem = create_named_semaphore("config_save_sem",1);
}
INIT_FUNC("debug", debug_init_func);

void NormalDisplay();
void MirrorDisplay();
static void HijackFormatDialogBox_main();
void config_menu_init();
void display_on();
void display_off();
void EngDrvOut(int reg, int value);
unsigned GetFileSize(char* filename);

void ui_lock(int what);

void fake_halfshutter_step();

#ifdef FEATURE_DEBUG_INTERCEPT
void j_debug_intercept() { debug_intercept(); }
void j_tp_intercept() { tp_intercept(); }
#endif

#ifdef FEATURE_SCREENSHOT

int rename_file(char* src, char* dst)
{
#if defined(CONFIG_FIO_RENAMEFILE_WORKS) // FIO_RenameFile known to work

    return FIO_RenameFile(src, dst);

#else
    // FIO_RenameFile not known, or doesn't work
    // emulate it by copy + erase (poor man's rename :P )

    const int bufsize = 128*1024;
    void* buf = alloc_dma_memory(bufsize);
    if (!buf) return 1;

    FILE* f = FIO_Open(src, O_RDONLY | O_SYNC);
    if (f == INVALID_PTR) return 1;

    FILE* g = FIO_CreateFile(dst);
    if (g == INVALID_PTR) { FIO_CloseFile(f); return 1; }

    int r = 0;
    while ((r = FIO_ReadFile(f, buf, bufsize)))
        FIO_WriteFile(g, buf, r);

    FIO_CloseFile(f);
    FIO_CloseFile(g);
    FIO_RemoveFile(src);
    msleep(1000); // this decreases the chances of getting corrupted files (figure out why!)
    free_dma_memory(buf);
    return 0;
#endif
}

void take_screenshot( int also_lv )
{
    beep();

    FIO_RemoveFile(CARD_DRIVE"TEST.BMP");

    call( "dispcheck" );
    #ifdef FEATURE_SCREENSHOT_422
    if (also_lv) silent_pic_take_lv_dbg();
    #endif

    if (GetFileSize(CARD_DRIVE"TEST.BMP") != 0xFFFFFFFF)
    { // old camera, screenshot saved as TEST.BMP => move it to VRAMxx.BMP
        msleep(300);
        for (int i = 0; i < 100; i++)
        {
            char fn[50];
            snprintf(fn, sizeof(fn), CARD_DRIVE"VRAM%d.BMP", i);
            if (GetFileSize(fn) == 0xFFFFFFFF) // this file does not exist
            {
                rename_file(CARD_DRIVE"TEST.BMP", fn);
                break;
            }
        }
    }


}
#endif

#if CONFIG_DEBUGMSG
static int draw_prop = 0;

static void
draw_prop_select( void * priv , int unused )
{
    draw_prop = !draw_prop;
}

static int dbg_propn = 0;
static void
draw_prop_reset( void * priv )
{
    dbg_propn = 0;
}
#endif

#if defined(CONFIG_7D) // pel: Checked. That's how it works in the 7D firmware
void _card_led_on()  //See sub_FF32B410 -> sub_FF0800A4
{
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = 0x800c00;
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDON); //0x138000
}
void _card_led_off()  //See sub_FF32B424 -> sub_FF0800B8
{
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = 0x800c00;
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDOFF); //0x38400
}
//TODO: Check if this is correct, because reboot.c said 0x838C00
#elif defined(CARD_LED_ADDRESS) && defined(LEDON) && defined(LEDOFF)
void _card_led_on()  { *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDON); }
void _card_led_off() { *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDOFF); }
#else
void _card_led_on()  { return; }
void _card_led_off() { return; }
#endif

void info_led_on()
{
    #ifdef CONFIG_VXWORKS
    LEDBLUE = LEDON;
    #elif defined(CONFIG_BLUE_LED)
    call("EdLedOn");
    #else
    _card_led_on();
    #endif
}
void info_led_off()
{
    #ifdef CONFIG_VXWORKS
    LEDBLUE = LEDOFF;
    #elif defined(CONFIG_BLUE_LED)
    call("EdLedOff");
    #else
    _card_led_off();
    #endif
}
void info_led_blink(int times, int delay_on, int delay_off)
{
    for (int i = 0; i < times; i++)
    {
        info_led_on();
        msleep(delay_on);
        info_led_off();
        msleep(delay_off);
    }
}


static int config_ok = 0;
static int config_deleted = 0;

// this can be called from more tasks (gui, prop handler, menu), so it needs to be thread safe
void
save_config( void * priv, int delta )
{
#ifdef CONFIG_CONFIG_FILE
    take_semaphore(config_save_sem, 0);
    update_disp_mode_bits_from_params();
    config_save_file( CARD_DRIVE "ML/SETTINGS/magic.cfg" );
    config_menu_save_flags();
    config_deleted = 0;
    give_semaphore(config_save_sem);
#endif
}

#ifdef CONFIG_CONFIG_FILE
#ifdef CONFIG_PICOC
static char last_preset_file[50] = "";
static int preset_just_saved = 0;
static int preset_scripts_dirty = 0;

static char*
find_picoc_config_filename()
{
    for (int i = 0; i < 10; i++)
    {
        snprintf(last_preset_file, sizeof(last_preset_file), CARD_DRIVE"ML/SCRIPTS/PRESET%d.C", i);

        if (GetFileSize(last_preset_file) == 0xFFFFFFFF) // this file does not exist
            return last_preset_file;
    }
    return 0;
}

// if the user tries to save more presets at a time,
// he will fill the script directory with identical files
// so.. let's calm him down :)
static int preset_user_angry = 0;

static void
save_config_as_picoc(void* priv, int delta)
{
    if (preset_just_saved)
    {
        preset_user_angry = 1;
        return;
    }

    char* fn = find_picoc_config_filename();
    if (fn)
    {
        menu_save_current_config_as_picoc_preset(fn);
        preset_just_saved = 1;
        preset_scripts_dirty = 1;
    }
}

static MENU_UPDATE_FUNC(save_config_as_picoc_update)
{
    static int last_displayed = 0;
    int t = get_ms_clock_value_fast();

    if (preset_just_saved == 2 && t - last_displayed > 2000) // if this menu was not displayed for a while, we can save a new preset
    {
        preset_just_saved = 0;
        preset_user_angry = 0;
    }

    if (preset_scripts_dirty)
    {
        MENU_SET_RINFO("Restart");
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Restart camera so the new preset appears in Scripts menu.");
    }

    if (preset_just_saved)
    {
        MENU_SET_NAME(last_preset_file + strlen(CARD_DRIVE"ML/SCRIPTS/"));
        if (preset_user_angry)
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Change some settings before saving a new preset.");
        last_displayed = t;
        preset_just_saved = 2;
    }
}

#endif // picoc

static MENU_UPDATE_FUNC(delete_config_update)
{
    if (config_deleted)
    {
        MENU_SET_RINFO("Restart");
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Restart your camera to complete the process.");
    }
}
static MENU_UPDATE_FUNC(save_config_update)
{
    if (config_deleted)
    {
        MENU_SET_RINFO("Undo");
    }
}
static void
delete_config( void * priv, int delta )
{
    FIO_RemoveFile( CARD_DRIVE "ML/SETTINGS/magic.cfg" );
    FIO_RemoveFile( CARD_DRIVE "ML/SETTINGS/MENU.CFG" );
    if (config_autosave) config_autosave_toggle(0, 0);
    config_deleted = 1;
}

static int modified_settings_page = 0;
static int modified_settings_need_more_pages;
static MENU_UPDATE_FUNC(show_modified_settings)
{
    if (!entry->selected) modified_settings_page = 0;
    if (!modified_settings_page)
    {
        MENU_SET_VALUE("");
        return;
    }

    if (!info->x) return;
    info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
    bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

    int skip = (modified_settings_page - 1) * (460 / font_med.height);
    int k = 0;
    int y = 0;
    struct menu * menu = (struct menu *) menu_get_root();
    for( ; menu ; menu = menu->next )
    {
        struct menu_entry * entry = menu->children;
        for( ; entry ; entry = entry->next )
        {
            if (config_var_was_changed(entry->priv))
            {
                k++;
                if (k <= skip) continue;

                char* value = (char*) menu_get_str_value_from_script(menu->name, entry->name);
                if (k%2) bmp_fill(COLOR_GRAY(10), 0, y, 720, font_med.height);
                bmp_printf(SHADOW_FONT(FONT_MED), 10, y, "%s - %s", menu->name, entry->name);
                bmp_printf(SHADOW_FONT(FONT(FONT_MED, COLOR_YELLOW, COLOR_BLACK)), 720 - strlen(value)*font_med.width, y, "%s", value);
                y += font_med.height;
                if (y > 450)
                {
                    modified_settings_need_more_pages = 1;
                    bmp_printf(FONT(FONT_MED, COLOR_CYAN, COLOR_BLACK), 710 - 7*font_med.width, y, "more...");
                    return;
                }
            }
        }
    }
    if (k == 0) bmp_printf(FONT_LARGE, 10, 10, "No settings changed.");
    modified_settings_need_more_pages = 0;
}

static MENU_SELECT_FUNC(toggle_modified_settings)
{
    if (delta < 0)
        modified_settings_page--;
    else if (modified_settings_need_more_pages)
        modified_settings_page++;
    else
        modified_settings_page = !modified_settings_page;
}

#endif

#if CONFIG_DEBUGMSG

static int vmax(int* x, int n)
{
    int i;
    int m = -100000;
    for (i = 0; i < n; i++)
        if (x[i] > m)
            m = x[i];
    return m;
}

static void dump_rom_task(void* priv, int unused)
{
    msleep(200);
    FILE * f = NULL;

    f = FIO_CreateFileEx(CARD_DRIVE "ML/LOGS/ROM0.BIN");
    if (f != (void*) -1)
    {
        bmp_printf(FONT_LARGE, 0, 60, "Writing ROM0");
        FIO_WriteFile(f, (void*) 0xF0000000, 0x01000000);
        FIO_CloseFile(f);
    }
    msleep(200);

    f = FIO_CreateFileEx(CARD_DRIVE "ML/LOGS/ROM1.BIN");
    if (f != (void*) -1)
    {
        bmp_printf(FONT_LARGE, 0, 60, "Writing ROM1");
        FIO_WriteFile(f, (void*) 0xF8000000, 0x01000000);
        FIO_CloseFile(f);
    }
    msleep(200);

    dump_big_seg(4, CARD_DRIVE "ML/LOGS/RAM4.BIN");
}

static void dump_rom(void* priv, int unused)
{
    gui_stop_menu();
    task_create("dump_task", 0x1e, 0, dump_rom_task, 0);
}
#endif

static void dump_logs_task(void* priv)
{
    msleep(200);
    call("dumpf");
}

static void dump_logs(void* priv)
{
    //gui_stop_menu();
    task_create("dump_logs_task", 0x1e, 0, dump_logs_task, 0);
}

// http://www.iro.umontreal.ca/~simardr/rng/lfsr113.c
int rand (void)
{
   static unsigned int z1 = 12345, z2 = 12345, z3 = 12345, z4 = 12345;
   unsigned int b;
   b  = ((z1 << 6) ^ z1) >> 13;
   z1 = ((z1 & 4294967294U) << 18) ^ b;
   b  = ((z2 << 2) ^ z2) >> 27;
   z2 = ((z2 & 4294967288U) << 2) ^ b;
   b  = ((z3 << 13) ^ z3) >> 21;
   z3 = ((z3 & 4294967280U) << 7) ^ b;
   b  = ((z4 << 3) ^ z4) >> 12;
   z4 = ((z4 & 4294967168U) << 13) ^ b;
   int ans = (z1 ^ z2 ^ z3 ^ z4);
   return ABS(ans);
}

#ifdef CONFIG_ISO_TESTS

void find_response_curve(char* fname)
{
    FILE* f = FIO_CreateFileEx(fname);

    ensure_movie_mode();
    clrscr();
    set_lv_zoom(5);

    msleep(1000);

    for (int i = 0; i < 64*2; i+=8)
        bmp_draw_rect(COLOR_BLACK,  i*5+40, 0, 8*5, 380);

    draw_line( 40,  190,  720-40,  190, COLOR_BLACK);

    extern int bv_auto;
    //int bva0 = bv_auto;
    bv_auto = 0; // make sure it won't interfere

    bv_enable(); // for enabling fine 1/8 EV increments

    int ma = (lens_info.raw_aperture_min + 7) & ~7;
    for (int i = 0; i < 64*2; i++)
    {
        int a = (i/2) & ~7;                                // change aperture in full-stop increments
        lens_set_rawaperture(ma + a);
        lens_set_rawshutter(96 + i - a);                   // shutter can be changed in finer increments
        msleep(400);
        int Y,U,V;
        get_spot_yuv(180, &Y, &U, &V);
        dot( i*5 + 40 - 16,  380 - Y*380/255 - 16, COLOR_BLUE, 3); // dot has an offset of 16px
        my_fprintf(f, "%d %d %d %d\n", i, Y, U, V);
    }
    FIO_CloseFile(f);
    beep();
    //~ call("dispcheck");
    lens_set_rawaperture(ma);
    lens_set_rawshutter(96);
}

void find_response_curve_ex(char* fname, int iso, int dgain, int htp)
{
    bmp_printf(FONT_MED, 0, 100, "ISO %d\nDGain %d\n%s", iso, dgain, htp ? "HTP" : "");
    set_htp(htp);
    msleep(100);
    lens_set_iso(iso);
    set_display_gain_equiv(dgain);

    find_response_curve(fname);

    set_display_gain_equiv(0);
    set_htp(0);
}

static void iso_response_curve_current()
{
    msleep(2000);

    static char name[100];
    extern int digic_iso_gain;

    snprintf(name, sizeof(name), CARD_DRIVE "ML/LOGS/i%d%s%s.txt",
        raw2iso(lens_info.iso_equiv_raw),
        digic_iso_gain <= 256 ? "e2" : digic_iso_gain != 1024 ? "e" : "",
        get_htp() ? "h" : "");

    find_response_curve(name);
}

void iso_response_curve_160()
{
    msleep(2000);

    // ISO 100x/160x/80x series

    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso80e.txt",     100,   790   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso160e.txt",    200,   790   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso320e.txt",    400,   790   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso640e.txt",    800,   790   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso1250e.txt",   1600,  790   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso2500e.txt",   3200,  790   , 0);

    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso160.txt",    160,     0   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso320.txt",    320,     0   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso640.txt",    640,     0   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso1250.txt",   1250,    0   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso2500.txt",   2500,    0   , 0);

    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso100.txt",    100,     0   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso200.txt",    200,     0   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso400.txt",    400,     0   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso800.txt",    800,     0   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso1600.txt",   1600,    0   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso3200.txt",   3200,    0   , 0);
}

void iso_response_curve_logain()
{
    msleep(2000);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso70e.txt",      100,   724   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso140e.txt",     200,   724   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso280e.txt",     400,   724   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso560e.txt",     800,   724   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso1100e.txt",    1600,  724   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso2200e.txt",    3200,  724   , 0);

    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso65e.txt",     100,   664   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso130e.txt",    200,   664   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso260e.txt",    400,   664   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso520e.txt",    800,   664   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso1000e.txt",   1600,  664   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso2000e.txt",   3200,  664   , 0);

    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso50e.txt",     100,   512   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso100e.txt",    200,   512   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso200e.txt",    400,   512   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso400e.txt",    800,   512   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso800e.txt",    1600,  512   , 0);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso1600e.txt",   3200,  512   , 0);
}

void iso_response_curve_htp()
{
    msleep(2000);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso200h.txt",      200,   0   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso400h.txt",      400,   0   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso800h.txt",      800,   0   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso1600h.txt",    1600,   0   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso3200h.txt",    3200,   0   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso6400h.txt",    6400,   0   , 1);

    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso140eh.txt",      200,   724   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso280eh.txt",      400,   724   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso560eh.txt",      800,   724   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/is1100eh.txt",     1600,   724   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/is2200eh.txt",     3200,   724   , 1);
    find_response_curve_ex(CARD_DRIVE "MLis4500eh.txt",     6400,   724   , 1);

    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso100eh.txt",      200,   512   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso200eh.txt",      400,   512   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso400eh.txt",      800,   512   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/iso800eh.txt",     1600,   512   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/is1600eh.txt",     3200,   512   , 1);
    find_response_curve_ex(CARD_DRIVE "ML/LOGS/is3200eh.txt",     6400,   512   , 1);
}

void iso_movie_change_setting(int iso, int dgain, int shutter)
{
    lens_set_rawiso(iso);
    lens_set_rawshutter(shutter);
    set_display_gain_equiv(dgain);
    msleep(2000);
    silent_pic_take_test();
}

void iso_movie_test()
{
    msleep(2000);
    ensure_movie_mode();

    int r = lens_info.iso_equiv_raw ? lens_info.iso_equiv_raw : lens_info.raw_iso_auto;
    int raw_iso0 = (r + 3) & ~7; // consider full-stop iso
    int tv0 = lens_info.raw_shutter;
    //int av0 = lens_info.raw_aperture;
    bv_enable(); // this enables shutter speed adjust in finer increments

    extern int bv_auto;
    int bva0 = bv_auto;
    bv_auto = 0; // make sure it won't interfere

    set_htp(0); msleep(100);
    movie_start();

    iso_movie_change_setting(raw_iso0,   0, tv0);     // fullstop ISO
    iso_movie_change_setting(raw_iso0-3, 0, tv0-3); // "native" iso, overexpose by 3/8 EV

    iso_movie_change_setting(raw_iso0, 790, tv0-3); // ML 160x equiv iso, overexpose by 3/8 EV
    iso_movie_change_setting(raw_iso0, 724, tv0-4); // ML 140x equiv iso, overexpose by 4/8 EV
    iso_movie_change_setting(raw_iso0, 664, tv0-5); // ML 130x equiv iso, overexpose by 5/8 EV

    iso_movie_change_setting(raw_iso0-3, 790, tv0-6); // 100x ISO, -3/8 Canon gain, -3/8 ML gain, overexpose by 6/8 EV
    iso_movie_change_setting(raw_iso0-3, 724, tv0-7); // 100x ISO, -3/8 Canon gain, -4/8 ML gain, overexpose by 7/8 EV
    iso_movie_change_setting(raw_iso0-3, 664, tv0-7); // 100x ISO, -3/8 Canon gain, -5/8 ML gain, overexpose by 8/8 EV

    msleep(1000);
    movie_end();
    msleep(2000);

    set_htp(1);  // this can't be set while recording

    movie_start();

    iso_movie_change_setting(raw_iso0,   0, tv0);     // fullstop ISO with HTP
    iso_movie_change_setting(raw_iso0-3, 0, tv0-3); // "native" ISO with HTP, overexpose by 3/8 EV
    iso_movie_change_setting(raw_iso0, 790, tv0-3); // ML 160x equiv iso with HTP, overexpose by 3/8 EV
    iso_movie_change_setting(raw_iso0, 724, tv0-4); // ML 140x equiv iso with HTP, overexpose by 4/8 EV
    iso_movie_change_setting(raw_iso0, 664, tv0-5); // ML 130x equiv iso with HTP, overexpose by 5/8 EV
    iso_movie_change_setting(raw_iso0, 512, tv0-8); // ML 100x equiv iso with HTP, overexpose by 8/8 EV

    iso_movie_change_setting(raw_iso0+8,   0, tv0);     // fullstop ISO + 1EV, with HTP
    iso_movie_change_setting(raw_iso0-3+8, 0, tv0-3); // "native" ISO + 1EV, with HTP, overexpose by 3/8 EV
    iso_movie_change_setting(raw_iso0+8, 790, tv0-3); // ML 160x equiv iso +1EV, with HTP, overexpose by 3/8 EV
    iso_movie_change_setting(raw_iso0+8, 724, tv0-4); // ML 140x equiv iso +1EV, with HTP, overexpose by 4/8 EV
    iso_movie_change_setting(raw_iso0+8, 664, tv0-5); // ML 130x equiv iso +1EV, with HTP, overexpose by 5/8 EV
    iso_movie_change_setting(raw_iso0+8, 512, tv0-8); // ML 100x equiv iso +1EV, with HTP, overexpose by 8/8 EV

    movie_end();

    // restore settings back
    iso_movie_change_setting(raw_iso0, 0, tv0);
    bv_auto = bva0;
}
#endif // CONFIG_ISO_TESTS


#ifdef FEATURE_GUIMODE_TEST
// beware, might be dangerous, some gui modes will give errors
void guimode_test()
{
    msleep(1000);
    for (int i = 0; i < 99; i++)
    {
        // some GUI modes may lock-up the camera or reboot
        // if this is the case, the troublesome mode will be skipped at next reboot.
        char fn[50];
        snprintf(fn, sizeof(fn), CARD_DRIVE"VRAM%d.BMP", i);

        if (GetFileSize(fn) != 0xFFFFFFFF) // this gui mode was already tested?
            continue;

        NotifyBox(500, "Trying GUI mode %d...", i);
        dump_seg(0, 0, fn); // temporary flag to indicate that this GUI mode was tried (and probably found to be troublesome)
        msleep(200);

        SetGUIRequestMode(i);

        msleep(1000);
        FIO_RemoveFile(fn);

        take_screenshot(0);

        // try to reset to initial gui mode
        SetGUIRequestMode(0);
        SetGUIRequestMode(1);
        SetGUIRequestMode(0);

        msleep(1000);
    }
}
#endif

//~ uncompressed video testing
#ifdef CONFIG_6D
FILE * movfile;
int record_uncomp = 0;
#endif

static void bsod()
{
    msleep(rand() % 20000 + 2000);

    do {
        gui_stop_menu();
        SetGUIRequestMode(1);
        msleep(1000);
    } while (CURRENT_DIALOG_MAYBE != 1);

    canon_gui_disable_front_buffer();
    ui_lock(UILOCK_EVERYTHING);
    bmp_fill(COLOR_BLUE, 0, 0, 720, 480);
    int fnt = SHADOW_FONT(FONT_MED);
    int h = font_med.height;
    int y = 50;
    bmp_printf(fnt, 0, y+=h, "   A problem has been detected and Magic Lantern has been"   );
    bmp_printf(fnt, 0, y+=h, "   shut down to prevent damage to your camera."              );
    y += h;
    bmp_printf(fnt, 0, y+=h, "   If this is the first time you've seen this Stop error"    );
    bmp_printf(fnt, 0, y+=h, "   screen, restart your camera. If this screen appears"      );
    bmp_printf(fnt, 0, y+=h, "   again, follow these steps:"                               );
    y += h;
    bmp_printf(fnt, 0, y+=h, "   Don't click things you don't know what they do."          );
    y += h;
    bmp_printf(fnt, 0, y+=h, "   Technical information:");
    bmp_printf(fnt, 0, y+=h, "   *** STOP 0x000000aa (0x1000af22, 0xdeadbeef, 0xffff)"     );
    y += h;
    bmp_printf(fnt, 0, y+=h, "   Beginning dump of physical memory"                        );
    bmp_printf(fnt, 0, y+=h, "   Physical memory dump complete. Your camera is bricked."   );
    y += h;
    bmp_printf(fnt, 0, y+=h, "   Contact the Magic Lantern guys at www.magiclantern.fm"    );
    bmp_printf(fnt, 0, y+=h, "   for further assistance and information."                  );
}

static void run_test()
{
    #ifdef CONFIG_EDMAC_MEMCPY
    msleep(2000);
    
    uint8_t* real = bmp_vram_real();
    uint8_t* idle = bmp_vram_idle();
    int xPos = 0;
    int xOff = 2;
    int yPos = 0;
    
    edmac_copy_rectangle_adv(BMP_VRAM_START(idle), BMP_VRAM_START(real), 960, 0, 0, 960, 0, 0, 960, 560);
    while(true)
    {
        edmac_copy_rectangle_adv(BMP_VRAM_START(real), BMP_VRAM_START(idle), 960, 0, 0, 960, xPos, yPos, 960-xPos, 560-yPos);
        xPos += xOff;
        
        if(xPos >= 720 || xPos < 0)
        {
            xOff *= -1;
        }
    }
    return;
    #endif
    
    call("lv_save_raw", 1);
    call("aewb_enableaewb", 0);
    return;
    
#ifdef FEATURE_SHOW_SIGNATURE
    console_show();
    console_printf("FW Signature 0x%08x", compute_signature((int*)SIG_START, SIG_LEN));
    msleep(1000);
#endif

#if 0
    void exmem_test();

    exmem_test();
    return;
#endif
/*
#ifdef CONFIG_MEMCHECK
    console_show();
    console_printf("should raise error at index=10...\n");
    char* foo = AllocateMemory(10);
    foo[9] = 0;
    foo[10] = 0;
    FreeMemory(foo);
    msleep(1000);
#endif
*/
#ifdef CONFIG_MODULES
    console_show();

    console_printf("Loading modules...\n");
    msleep(1000);
    module_load_all();
    return;

    console_printf("\n");

    console_printf("Testing TCC executable...\n");
    console_printf(" [i] this may take some time\n");
    msleep(1000);

    for(int try = 0; try < 100; try++)
    {
        void *module = NULL;
        uint32_t ret = 0;

        module = module_load(MODULE_PATH"libtcc.mex");
        if(module)
        {
            ret = module_exec(module, "tcc_new", 0);
            if(!(ret & 0x40000000))
            {
                console_printf("tcc_new() returned: 0x%08X\n", ret);
            }
            else
            {
                module_exec(module, "tcc_delete", 1, ret);
            }
            module_unload(module);
        }
        else
        {
            console_printf(" [E] load failed\n");
        }
    }

    console_printf("Done!\n");
#endif
}

void run_in_separate_task(void (*priv)(void), int delta)
{
    gui_stop_menu();
    if (!priv) return;
    task_create("run_test", 0x1a, 0x1000, priv, (void*)delta);
}


#ifdef CONFIG_BENCHMARKS

/* for 5D3, the location of the benchmark file is important;
 * if we put it in root, it will benchmark the ML card;
 * if we put it in DCIM, it will benchmark the card selected in Canon menu, which is what we want.
 */
#define CARD_BENCHMARK_FILE CARD_DRIVE"DCIM/bench.tmp"

static void card_benchmark_wr(int bufsize, int K, int N)
{
    int x = 0;
    static int y = 80;
    if (K == 1) y = 80;

    FIO_RemoveFile(CARD_BENCHMARK_FILE);
    msleep(2000);
    int filesize = 1024; // MB
    int n = filesize * 1024 * 1024 / bufsize;
    {
        FILE* f = FIO_CreateFileEx(CARD_BENCHMARK_FILE);
        int t0 = get_ms_clock_value();
        int i;
        for (i = 0; i < n; i++)
        {
            uint32_t start = (int)UNCACHEABLE(YUV422_LV_BUFFER_1);
            bmp_printf(FONT_LARGE, 0, 0, "[%d/%d] Writing: %d/100 (buf=%dK)... ", K, N, i * 100 / n, bufsize/1024);
            FIO_WriteFile( f, (const void *) start, bufsize );
        }
        FIO_CloseFile(f);
        int t1 = get_ms_clock_value();
        int speed = filesize * 1000 * 10 / (t1 - t0);
        bmp_printf(FONT_MED, x, y += font_med.height, "Write speed (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
    }

    msleep(2000);

    {
        void* buf = shoot_malloc(bufsize);
        if (buf)
        {
            FILE* f = FIO_Open(CARD_BENCHMARK_FILE, O_RDONLY | O_SYNC);
            int t0 = get_ms_clock_value();
            int i;
            for (i = 0; i < n; i++)
            {
                bmp_printf(FONT_LARGE, 0, 0, "[%d/%d] Reading: %d/100 (buf=%dK)... ", K, N, i * 100 / n, bufsize/1024);
                FIO_ReadFile(f, UNCACHEABLE(buf), bufsize );
            }
            FIO_CloseFile(f);
            shoot_free(buf);
            int t1 = get_ms_clock_value();
            int speed = filesize * 1000 * 10 / (t1 - t0);
            bmp_printf(FONT_MED, x, y += font_med.height, "Read speed  (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
        }
        else
        {
            bmp_printf(FONT_MED, x, y += font_med.height, "malloc error: buffer=%d\n", bufsize);
        }
    }

    FIO_RemoveFile(CARD_BENCHMARK_FILE);
    msleep(2000);
}

static char* print_benchmark_header()
{
    bmp_printf(FONT_MED, 0, 40, "ML %s, %s", build_version, build_id); // this includes camera name
    
    static char mode[100];
    snprintf(mode, sizeof(mode), "Mode: ");
    if (lv)
    {
        if (lv_dispsize > 1)
        {
            STR_APPEND(mode, "LV zoom x%d", lv_dispsize);
        }
        else if (is_movie_mode())
        {
            char* video_modes[] = {"1920x1080", "1280x720", "640x480"};
            STR_APPEND(mode, "movie %s%s %dp", video_modes[video_mode_resolution], video_mode_crop ? " crop" : "", video_mode_fps);
        }
        else
        {
            STR_APPEND(mode, "LV photo");
        }
    }
    else
    {
        STR_APPEND(mode, PLAY_MODE ? "playback" : display_idle() ? "photo" : "idk");
    }
    
    STR_APPEND(mode, ", Global Draw: %s", get_global_draw() ? "ON" : "OFF");
    
    bmp_printf(FONT_MED, 0, 60, mode);
    return mode;
}

static void card_benchmark_task()
{
    msleep(1000);
    if (!DISPLAY_IS_ON) { fake_simple_button(BGMT_PLAY); msleep(1000); }

    #ifdef CONFIG_5D3
    extern int card_select;
    NotifyBox(2000, "%s Benchmark (1 GB)...", card_select == 1 ? "CF" : "SD");
    #else
    NotifyBox(2000, "Card benchmark (1 GB)...");
    #endif
    msleep(3000);
    canon_gui_disable_front_buffer();
    clrscr();
    
    print_benchmark_header();

    #ifdef CARD_A_MAKER
    bmp_printf(FONT_MED, 0, 80, "CF %s %s", CARD_A_MAKER, CARD_A_MODEL);
    #endif

    card_benchmark_wr(2*1024*1024,  1, 9);
    card_benchmark_wr(2000000,      2, 9);
    card_benchmark_wr(3*1024*1024,  3, 9);
    card_benchmark_wr(3000000,      4, 9);
    card_benchmark_wr(4*1024*1024,  5, 9);
    card_benchmark_wr(4000000,      6, 9);
    card_benchmark_wr(16*1024*1024, 7, 9);
    card_benchmark_wr(16000000,     8, 9);
    card_benchmark_wr(128*1024,     9, 9);
    call("dispcheck");
    msleep(3000);
    canon_gui_enable_front_buffer(0);
}



#ifdef CONFIG_5D3

static struct msg_queue * twocard_mq = 0;
static volatile int twocard_bufsize = 0;
static volatile int twocard_done = 0;

static void twocard_init()
{
    twocard_mq = (void*)msg_queue_create("twocard", 100);
}
INIT_FUNC("twocard", twocard_init);

static void twocard_write_task(char* filename)
{
    int bufsize = twocard_bufsize;
    int t0 = get_ms_clock_value();
    int cf = filename[0] == 'A';
    int msg;
    int filesize = 0;
    FILE* f = FIO_CreateFileEx(filename);
    if (f != INVALID_PTR)
    {
        while (msg_queue_receive(twocard_mq, (struct event **) &msg, 1000) == 0)
        {
            uint32_t start = (int)UNCACHEABLE(YUV422_LV_BUFFER_1);
            bmp_printf(FONT_MED, 0, cf*20, "[%s] Writing chunk %d [total=%d MB] (buf=%dK)... ", cf ? "CF" : "SD", msg, filesize, bufsize/1024);
            int r = FIO_WriteFile( f, (const void *) start, bufsize );
            if (r != bufsize) break; // card full?
            filesize += bufsize / 1024 / 1024;
        }
        FIO_CloseFile(f);
        FIO_RemoveFile(filename);
        int t1 = get_ms_clock_value() - 1000;
        int speed = filesize * 1000 * 10 / (t1 - t0);
        bmp_printf(FONT_MED, 0, 120+cf*20, "[%s] Write speed (buffer=%dk):\t %d.%d MB/s\n", cf ? "CF" : "SD", bufsize/1024, speed/10, speed % 10);
    }
    twocard_done++;
}

static void twocard_benchmark_task()
{
    msleep(1000);
    if (!DISPLAY_IS_ON) { fake_simple_button(BGMT_PLAY); msleep(1000); }
    canon_gui_disable_front_buffer();
    clrscr();
    print_benchmark_header();

    #ifdef CARD_A_MAKER
    bmp_printf(FONT_MED, 0, 80, "CF %s %s", CARD_A_MAKER, CARD_A_MODEL);
    #endif

    uint32_t bufsize = 32*1024*1024;

    msleep(2000);
    uint32_t filesize = 2048; // MB
    uint32_t n = filesize * 1024 * 1024 / bufsize;
    twocard_bufsize = bufsize;

    for (uint32_t i = 0; i < n; i++)
        msg_queue_post(twocard_mq, i);
    
    twocard_done = 0;
    task_create("twocard_cf", 0x1d, 0x2000, twocard_write_task, "A:/bench.tmp");
    task_create("twocard_sd", 0x1d, 0x2000, twocard_write_task, "B:/bench.tmp");

    while (twocard_done < 2) msleep(100);
    
    call("dispcheck");
    msleep(3000);
    canon_gui_enable_front_buffer(0);
}

#endif

static void card_bufsize_benchmark_task()
{
    msleep(1000);
    if (!DISPLAY_IS_ON) { fake_simple_button(BGMT_PLAY); msleep(1000); }
    canon_gui_disable_front_buffer();
    clrscr();
    
    int x = 0;
    int y = 100;
    
    FILE* log = FIO_CreateFileEx(CARD_DRIVE"bench.log");
    if (log == INVALID_PTR) goto cleanup;

    my_fprintf(log, "Buffer size experiment\n");
    my_fprintf(log, "ML %s, %s\n", build_version, build_id); // this includes camera name
    char* mode = print_benchmark_header();
    my_fprintf(log, "%s\n", mode);
    
    #ifdef CARD_A_MAKER
    my_fprintf(log, "CF %s %s\n", CARD_A_MAKER, CARD_A_MODEL);
    #endif
    
    while(1)
    {
        /* random buffer size between 1K and 32M, with 1K increments */
        uint32_t bufsize = ((rand() % 32768) + 1) * 1024;

        msleep(1000);
        uint32_t filesize = 256; // MB
        uint32_t n = filesize * 1024 * 1024 / bufsize;

        FILE* f = FIO_CreateFileEx(CARD_BENCHMARK_FILE);
        int t0 = get_ms_clock_value();
        int total = 0;
        for (uint32_t i = 0; i < n; i++)
        {
            uint32_t start = (int)UNCACHEABLE(YUV422_LV_BUFFER_1);
            bmp_printf(FONT_LARGE, 0, 0, "Writing: %d/100 (buf=%dK)... ", i * 100 / n, bufsize/1024);
            uint32_t r = FIO_WriteFile( f, (const void *) start, bufsize );
            total += r;
            if (r != bufsize) break;
        }
        FIO_CloseFile(f);
        int t1 = get_ms_clock_value();
        int speed = total / 1024 * 1000 / 1024 * 10 / (t1 - t0);
        bmp_printf(FONT_MED, x, y += font_med.height, "Write speed (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
        if (y > 450) y = 100;
        
        my_fprintf(log, "%d %d\n", bufsize, speed);
        
    }
cleanup:
    if (log != INVALID_PTR) FIO_CloseFile(log);
    canon_gui_enable_front_buffer(1);
}

typedef void (*mem_bench_fun)(
    int arg0,
    int arg1,
    int arg2,
    int arg3
);


static void mem_benchmark_run(char* msg, int* y, int bufsize, mem_bench_fun bench_fun, int arg0, int arg1, int arg2, int arg3)
{
    bmp_printf(FONT_LARGE, 0, 0, "%s...", msg);

    int times = 0;
    int t0 = get_ms_clock_value();
    for (int i = 0; i < INT_MAX; i++)
    {
        bench_fun(arg0, arg1, arg2, arg3);
        if (i%2) info_led_off(); else info_led_on();
        
        /* run the benchmark for roughly 1 second */
        if (get_ms_clock_value_fast() - t0 > 1000)
        {
            times = i + 1;
            break;
        }
    }
    int t1 = get_ms_clock_value();
    int dt = t1 - t0;
    
    info_led_off();
    
    /* units: KB/s */
    int speed = bufsize * times / dt;

    /* transform in MB/s x100 */
    speed = speed * 100 / 1024;
    
    bmp_printf(FONT_MED, 0, *y += font_med.height, "%s :%4d.%02d MB/s", msg, speed/100, speed%100);
    msleep(10);
}

static void mem_test_bmp_fill(int arg0, int arg1, int arg2, int arg3)
{
    bmp_draw_to_idle(1);
    bmp_fill(COLOR_BLACK, arg0, arg1, arg2, arg3);
    bmp_draw_to_idle(0);
}

#ifdef CONFIG_EDMAC_MEMCPY
void mem_test_edmac_copy_rectangle(int arg0, int arg1, int arg2, int arg3)
{
    uint8_t* real = bmp_vram_real();
    uint8_t* idle = bmp_vram_idle();
    edmac_copy_rectangle_adv(BMP_VRAM_START(idle), BMP_VRAM_START(real), 960, 0, 0, 960, 0, 0, 720, 480);
}
#endif

static uint64_t FAST mem_test_read64(uint64_t* buf, uint32_t n)
{
    /** GCC output with -Os attribute(O3):
     * loc_7433C
     * LDMIA   R0!, {R2,R3}
     * CMP     R0, R1
     * BNE     loc_7433C
     */
    
    /* note: this kind of loops are much faster with -funroll-all-loops */
    register uint64_t tmp = 0;
    for (uint32_t i = 0; i < n/8; i++)
        tmp = buf[i];
    return tmp;
}

static uint32_t FAST mem_test_read32(uint32_t* buf, uint32_t n)
{
    /** GCC output with -Os attribute(O3):
     * loc_74310
     * LDR     R0, [R3],#4
     * CMP     R3, R2
     * BNE     loc_74310
     */
    
    register uint32_t tmp = 0;
    for (uint32_t i = 0; i < n/4; i++)
        tmp = buf[i];
    return tmp;
}


static void mem_benchmark_task()
{
    msleep(1000);
    if (!DISPLAY_IS_ON) { fake_simple_button(BGMT_PLAY); msleep(1000); }
    canon_gui_disable_front_buffer();
    clrscr();
    print_benchmark_header();

    int bufsize = 16*1024*1024;
    
    void* buf1 = 0;
    void* buf2 = 0;
    buf1 = shoot_malloc(bufsize);
    buf2 = shoot_malloc(bufsize);
    if (!buf1 || !buf2)
    {
        bmp_printf(FONT_LARGE, 0, 0, "malloc error :(");
        goto cleanup;
    }

    int y = 80;

#if 0 // need to hack the source code to run this benchmark
    extern int defish_ind;
    defish_draw_lv_color();
    void defish_draw_lv_color_loop(uint64_t* src_buf, uint64_t* dst_buf, int* ind);
    if (defish_ind)
    mem_benchmark_run("defish_draw_lv_color", &y, 720*os.y_ex, (mem_bench_fun)defish_draw_lv_color_loop, (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), defish_ind, 0);
#endif

    mem_benchmark_run("memcpy cacheable    ", &y, bufsize, (mem_bench_fun)memcpy,     (intptr_t)CACHEABLE(buf1),   (intptr_t)CACHEABLE(buf2),   bufsize, 0);
    mem_benchmark_run("memcpy uncacheable  ", &y, bufsize, (mem_bench_fun)memcpy,     (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), bufsize, 0);
    mem_benchmark_run("memcpy64 cacheable  ", &y, bufsize, (mem_bench_fun)memcpy64,   (intptr_t)CACHEABLE(buf1),   (intptr_t)CACHEABLE(buf2),   bufsize, 0);
    mem_benchmark_run("memcpy64 uncacheable", &y, bufsize, (mem_bench_fun)memcpy64,   (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), bufsize, 0);
    #ifdef CONFIG_DMA_MEMCPY
    mem_benchmark_run("dma_memcpy cacheable", &y, bufsize, (mem_bench_fun)dma_memcpy, (intptr_t)CACHEABLE(buf1),   (intptr_t)CACHEABLE(buf2),   bufsize, 0);
    mem_benchmark_run("dma_memcpy uncacheab", &y, bufsize, (mem_bench_fun)dma_memcpy, (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), bufsize, 0);
    #endif
    #ifdef CONFIG_EDMAC_MEMCPY
    mem_benchmark_run("edmac_memcpy        ", &y, bufsize, (mem_bench_fun)edmac_memcpy, (intptr_t)buf1,   (intptr_t)buf2,   bufsize, 0);
    mem_benchmark_run("edmac_copy_rectangle", &y, 720*480, (mem_bench_fun)mem_test_edmac_copy_rectangle, 0, 0, 0, 0);
    #endif
    mem_benchmark_run("memset cacheable    ", &y, bufsize, (mem_bench_fun)memset,     (intptr_t)CACHEABLE(buf1),   0,                           bufsize, 0);
    mem_benchmark_run("memset uncacheable  ", &y, bufsize, (mem_bench_fun)memset,     (intptr_t)UNCACHEABLE(buf1), 0,                           bufsize, 0);
    mem_benchmark_run("memset64 cacheable  ", &y, bufsize, (mem_bench_fun)memset64,   (intptr_t)CACHEABLE(buf1),   0,                           bufsize, 0);
    mem_benchmark_run("memset64 uncacheable", &y, bufsize, (mem_bench_fun)memset64,   (intptr_t)UNCACHEABLE(buf1), 0,                           bufsize, 0);
    mem_benchmark_run("read32 cacheable    ", &y, bufsize, (mem_bench_fun)mem_test_read32, (intptr_t)CACHEABLE(buf1),   bufsize, 0, 0);
    mem_benchmark_run("read32 uncacheable  ", &y, bufsize, (mem_bench_fun)mem_test_read32, (intptr_t)UNCACHEABLE(buf1), bufsize, 0, 0);
    mem_benchmark_run("read64 cacheable    ", &y, bufsize, (mem_bench_fun)mem_test_read64, (intptr_t)CACHEABLE(buf1),   bufsize, 0, 0);
    mem_benchmark_run("read64 uncacheable  ", &y, bufsize, (mem_bench_fun)mem_test_read64, (intptr_t)UNCACHEABLE(buf1), bufsize, 0, 0);
    mem_benchmark_run("bmp_fill to idle buf", &y, 720*480, (mem_bench_fun)mem_test_bmp_fill, 0, 0, 720, 480);

    call("dispcheck");
    msleep(3000);
    canon_gui_enable_front_buffer(0);

cleanup:
    if (buf1) shoot_free(buf1);
    if (buf2) shoot_free(buf2);
}

#endif

#ifdef CONFIG_STRESS_TEST

/*static void stress_test_long(void* priv, int delta)
{
    gui_stop_menu();
    task_create("fake_buttons", 0x1c, 0, fake_buttons, 0);
    task_create("change_colors", 0x1c, 0, change_colors_like_crazy, 0);
}*/

static void stress_test_picture(int n, int delay)
{
    if (shutter_count > 50000) { beep(); return; }
    msleep(delay);
    for (int i = 0; i < n; i++)
    {
        NotifyBox(10000, "Picture taking: %d/%d", i+1, n);
        msleep(200);
        lens_take_picture(64, 0);
    }
    lens_wait_readytotakepic(64);
    msleep(delay);
}

#define TEST_MSG(fmt, ...) { if (!silence || !ok) my_fprintf(log, fmt, ## __VA_ARGS__); bmp_printf(FONT_MED, 0, 0, fmt, ## __VA_ARGS__); }
#define TEST_TRY_VOID(x) { x; ok = 1; TEST_MSG("       %s\n", #x); }
#define TEST_TRY_FUNC(x) { int ans = (int)(x); ok = 1; TEST_MSG("       %s => 0x%x\n", #x, ans); }
#define TEST_TRY_FUNC_CHECK(x, condition) { int ans = (int)(x); ok = ans condition; TEST_MSG("[%s] %s => 0x%x\n", ok ? "Pass" : "FAIL", #x, ans); if (ok) passed_tests++; else { failed_tests++; msleep(500); } }
#define TEST_TRY_FUNC_CHECK_STR(x, expected_string) { char* ans = (char*)(x); ok = streq(ans, expected_string); TEST_MSG("[%s] %s => '%s'\n", ok ? "Pass" : "FAIL", #x, ans); if (ok) passed_tests++; else { failed_tests++; msleep(500); } }

static int test_task_created = 0;
static void test_task() { test_task_created = 1; }

static void stub_test_task(void* arg)
{
    // this test can be repeated many times, as burn-in test
    int n = (int)arg > 0 ? 1 : 100;
    msleep(1000);
    info_led_on();
    int passed_tests = 0;
    int failed_tests = 0;

    FILE* log = FIO_CreateFileEx( CARD_DRIVE "stubtest.log" );
    int silence = 0;    // if 1, only failures are logged to file
    int ok = 1;

    for (int i=0; i < n; i++)
    {

        // strlen
        TEST_TRY_FUNC_CHECK(strlen("abc"), == 3);
        TEST_TRY_FUNC_CHECK(strlen("qwertyuiop"), == 10);
        TEST_TRY_FUNC_CHECK(strlen(""), == 0);

        // strcpy
        char msg[10];
        TEST_TRY_FUNC_CHECK(strcpy(msg, "hi there"), == (int)msg);
        TEST_TRY_FUNC_CHECK_STR(msg, "hi there");

        // strcmp, snprintf
        // gcc will optimize strcmp calls with constant arguments, so use snprintf to force gcc to call strcmp
        char a[50]; char b[50];

        TEST_TRY_FUNC_CHECK(snprintf(a, sizeof(a), "foo"), == 3);
        TEST_TRY_FUNC_CHECK(snprintf(b, sizeof(b), "foo"), == 3);
        TEST_TRY_FUNC_CHECK(strcmp(a, b), == 0);

        TEST_TRY_FUNC_CHECK(snprintf(a, sizeof(a), "bar"), == 3);
        TEST_TRY_FUNC_CHECK(snprintf(b, sizeof(b), "baz"), == 3);
        TEST_TRY_FUNC_CHECK(strcmp(a, b), < 0);

        TEST_TRY_FUNC_CHECK(snprintf(a, sizeof(a), "Display"), == 7);
        TEST_TRY_FUNC_CHECK(snprintf(b, sizeof(b), "Defishing"), == 9);
        TEST_TRY_FUNC_CHECK(strcmp(a, b), > 0);

        // vsnprintf (called by snprintf)
        char buf[4];
        TEST_TRY_FUNC_CHECK(snprintf(buf, 3, "%d", 1234), == 2);
        TEST_TRY_FUNC_CHECK_STR(buf, "12");

        // memcpy, memset, bzero32
        char foo[] __attribute__((aligned(32))) = "qwertyuiop";
        char bar[] __attribute__((aligned(32))) = "asdfghjkl;";
        TEST_TRY_FUNC_CHECK(memcpy(foo, bar, 6), == (int)foo);
        TEST_TRY_FUNC_CHECK_STR(foo, "asdfghuiop");
        TEST_TRY_FUNC_CHECK(memset(bar, '*', 5), == (int)bar);
        TEST_TRY_FUNC_CHECK_STR(bar, "*****hjkl;");
        TEST_TRY_VOID(bzero32(bar + 5, 5));
        TEST_TRY_FUNC_CHECK_STR(bar, "****");

        // digic clock, msleep
        int t0, t1;
        TEST_TRY_FUNC(t0 = *(uint32_t*)0xC0242014);
        TEST_TRY_VOID(msleep(250));
        TEST_TRY_FUNC(t1 = *(uint32_t*)0xC0242014);
        TEST_TRY_FUNC_CHECK(ABS(mod(t1-t0, 1048576)/1000 - 250), < 30);

        // calendar
        struct tm now;
        int s0, s1;
        TEST_TRY_VOID(LoadCalendarFromRTC( &now ));
        TEST_TRY_FUNC(s0 = now.tm_sec);

        TEST_MSG(
            "       Date/time: %04d/%02d/%02d %02d:%02d:%02d\n",
            now.tm_year + 1900,
            now.tm_mon + 1,
            now.tm_mday,
            now.tm_hour,
            now.tm_min,
            now.tm_sec
        );

        TEST_TRY_VOID(msleep(1500));
        TEST_TRY_VOID(LoadCalendarFromRTC( &now ));
        TEST_TRY_FUNC(s1 = now.tm_sec);
        TEST_TRY_FUNC_CHECK(mod(s1-s0, 60), >= 1);
        TEST_TRY_FUNC_CHECK(mod(s1-s0, 60), <= 2);

        // mallocs
        // run this test 200 times to check for memory leaks 
        for (int i = 0; i < 200; i++)
        {
            int silence = (i > 0);
            int m0, m1, m2;
            void* p;
            TEST_TRY_FUNC(m0 = MALLOC_FREE_MEMORY);
            TEST_TRY_FUNC_CHECK(p = malloc(50*1024), != 0);
            TEST_TRY_FUNC_CHECK(CACHEABLE(p), == (int)p);
            TEST_TRY_FUNC(m1 = MALLOC_FREE_MEMORY);
            TEST_TRY_VOID(free(p));
            TEST_TRY_FUNC(m2 = MALLOC_FREE_MEMORY);
            TEST_TRY_FUNC_CHECK(ABS((m0-m1) - 50*1024), < 2048);
            TEST_TRY_FUNC_CHECK(ABS(m0-m2), < 2048);

            TEST_TRY_FUNC(m0 = GetFreeMemForAllocateMemory());
            TEST_TRY_FUNC_CHECK(p = AllocateMemory(256*1024), != 0);
            TEST_TRY_FUNC_CHECK(CACHEABLE(p), == (int)p);
            TEST_TRY_FUNC(m1 = GetFreeMemForAllocateMemory());
            TEST_TRY_VOID(FreeMemory(p));
            TEST_TRY_FUNC(m2 = GetFreeMemForAllocateMemory());
            TEST_TRY_FUNC_CHECK(ABS((m0-m1) - 256*1024), < 2048);
            TEST_TRY_FUNC_CHECK(ABS(m0-m2), < 2048);

            // these buffers may be from different memory pools, just check for leaks in main pools
            int m01, m02, m11, m12;
            TEST_TRY_FUNC(m01 = MALLOC_FREE_MEMORY);
            TEST_TRY_FUNC(m02 = GetFreeMemForAllocateMemory());
            TEST_TRY_FUNC_CHECK(p = alloc_dma_memory(256*1024), != 0);
            TEST_TRY_FUNC_CHECK(UNCACHEABLE(p), == (int)p);
            TEST_TRY_FUNC_CHECK(CACHEABLE(p), != (int)p);
            TEST_TRY_FUNC_CHECK(UNCACHEABLE(CACHEABLE(p)), == (int)p);
            TEST_TRY_VOID(free_dma_memory(p));
            TEST_TRY_FUNC_CHECK(p = (void*)shoot_malloc(24*1024*1024), != 0);
            TEST_TRY_FUNC_CHECK(UNCACHEABLE(p), == (int)p);
            TEST_TRY_VOID(shoot_free(p));
            TEST_TRY_FUNC(m11 = MALLOC_FREE_MEMORY);
            TEST_TRY_FUNC(m12 = GetFreeMemForAllocateMemory());
            TEST_TRY_FUNC_CHECK(ABS(m01-m11), < 2048);
            TEST_TRY_FUNC_CHECK(ABS(m02-m12), < 2048);
        }
        
        // exmem
        // run this test 20 times to check for memory leaks 
        for (int i = 0; i < 20; i++)
        {
            int silence = (i > 0);

            struct memSuite * suite = 0;
            struct memChunk * chunk = 0;
            void* p = 0;
            int total = 0;

            // contiguous allocation
            TEST_TRY_FUNC_CHECK(suite = shoot_malloc_suite_contig(24*1024*1024), != 0);
            TEST_TRY_FUNC_CHECK_STR(suite->signature, "MemSuite");
            TEST_TRY_FUNC_CHECK(suite->num_chunks, == 1);
            TEST_TRY_FUNC_CHECK(suite->size, == 24*1024*1024);
            TEST_TRY_FUNC_CHECK(chunk = GetFirstChunkFromSuite(suite), != 0);
            TEST_TRY_FUNC_CHECK_STR(chunk->signature, "MemChunk");
            TEST_TRY_FUNC_CHECK(chunk->size, == 24*1024*1024);
            TEST_TRY_FUNC_CHECK(p = GetMemoryAddressOfMemoryChunk(chunk), != 0);
            TEST_TRY_FUNC_CHECK(UNCACHEABLE(p), == (int)p);
            TEST_TRY_VOID(shoot_free_suite(suite); suite = 0; chunk = 0;);

            // contiguous allocation, largest block
            TEST_TRY_FUNC_CHECK(suite = shoot_malloc_suite_contig(0), != 0);
            TEST_TRY_FUNC_CHECK_STR(suite->signature, "MemSuite");
            TEST_TRY_FUNC_CHECK(suite->num_chunks, == 1);
            TEST_TRY_FUNC_CHECK(suite->size, > 24*1024*1024);
            TEST_TRY_FUNC_CHECK(chunk = GetFirstChunkFromSuite(suite), != 0);
            TEST_TRY_FUNC_CHECK_STR(chunk->signature, "MemChunk");
            TEST_TRY_FUNC_CHECK(chunk->size, == suite->size);
            TEST_TRY_FUNC_CHECK(p = GetMemoryAddressOfMemoryChunk(chunk), != 0);
            TEST_TRY_FUNC_CHECK(UNCACHEABLE(p), == (int)p);
            TEST_TRY_VOID(shoot_free_suite(suite); suite = 0; chunk = 0;);

            // fragmented allocation
            TEST_TRY_FUNC_CHECK(suite = shoot_malloc_suite(64*1024*1024), != 0);
            TEST_TRY_FUNC_CHECK_STR(suite->signature, "MemSuite");
            TEST_TRY_FUNC_CHECK(suite->num_chunks, > 1);
            TEST_TRY_FUNC_CHECK(suite->size, == 64*1024*1024);

            // iterating through chunks
            total = 0;
            TEST_TRY_FUNC_CHECK(chunk = GetFirstChunkFromSuite(suite), != 0);
            while(chunk)
            {
                TEST_TRY_FUNC_CHECK_STR(chunk->signature, "MemChunk");
                TEST_TRY_FUNC_CHECK(total += chunk->size, <= 64*1024*1024);
                TEST_TRY_FUNC_CHECK(p = GetMemoryAddressOfMemoryChunk(chunk), != 0);
                TEST_TRY_FUNC_CHECK(UNCACHEABLE(p), == (int)p);
                TEST_TRY_FUNC(chunk = GetNextMemoryChunk(suite, chunk));
            }
            TEST_TRY_FUNC_CHECK(total, == 64*1024*1024);
            TEST_TRY_VOID(shoot_free_suite(suite); suite = 0; chunk = 0; );

            // fragmented allocation, max size
            TEST_TRY_FUNC_CHECK(suite = shoot_malloc_suite(0), != 0);
            TEST_TRY_FUNC_CHECK_STR(suite->signature, "MemSuite");
            TEST_TRY_FUNC_CHECK(suite->num_chunks, > 1);
            TEST_TRY_FUNC_CHECK(suite->size, > 64*1024*1024);

            // iterating through chunks
            total = 0;
            TEST_TRY_FUNC_CHECK(chunk = GetFirstChunkFromSuite(suite), != 0);
            while(chunk)
            {
                TEST_TRY_FUNC_CHECK_STR(chunk->signature, "MemChunk");
                TEST_TRY_FUNC_CHECK(total += chunk->size, <= suite->size);
                TEST_TRY_FUNC_CHECK(p = GetMemoryAddressOfMemoryChunk(chunk), != 0);
                TEST_TRY_FUNC_CHECK(UNCACHEABLE(p), == (int)p);
                TEST_TRY_FUNC(chunk = GetNextMemoryChunk(suite, chunk));
            }
            TEST_TRY_FUNC_CHECK(total, == suite->size);
            TEST_TRY_VOID(shoot_free_suite(suite); suite = 0; chunk = 0; );
        }
        
        // engio
        TEST_TRY_VOID(EngDrvOut(0xC0F14400, 0x1234));
        TEST_TRY_FUNC_CHECK(shamem_read(0xC0F14400), == 0x1234);

        // call, DISPLAY_IS_ON
        TEST_TRY_VOID(call("TurnOnDisplay"));
        TEST_TRY_FUNC_CHECK(DISPLAY_IS_ON, != 0);
        TEST_TRY_VOID(call("TurnOffDisplay"));
        TEST_TRY_FUNC_CHECK(DISPLAY_IS_ON, == 0);
        TEST_TRY_VOID(call("TurnOnDisplay"));
        TEST_TRY_FUNC_CHECK(DISPLAY_IS_ON, != 0);

        // SetGUIRequestMode, CURRENT_DIALOG_MAYBE
        #ifdef GUIMODE_ML_MENU
        TEST_TRY_VOID(SetGUIRequestMode(1); msleep(1000););
        TEST_TRY_FUNC_CHECK(CURRENT_DIALOG_MAYBE, == 1);
        TEST_TRY_VOID(SetGUIRequestMode(2); msleep(1000););
        TEST_TRY_FUNC_CHECK(CURRENT_DIALOG_MAYBE, == 2);
        TEST_TRY_VOID(SetGUIRequestMode(0); msleep(1000););
        TEST_TRY_FUNC_CHECK(CURRENT_DIALOG_MAYBE, == 0);
        TEST_TRY_FUNC_CHECK(display_idle(), != 0);
        #endif

        // GUI_Control
        TEST_TRY_VOID(GUI_Control(BGMT_PLAY, 0, 0, 0); msleep(500););
        TEST_TRY_FUNC_CHECK(PLAY_MODE, != 0);
        TEST_TRY_FUNC_CHECK(MENU_MODE, == 0);
        TEST_TRY_VOID(GUI_Control(BGMT_MENU, 0, 0, 0); msleep(500););
        TEST_TRY_FUNC_CHECK(MENU_MODE, != 0);
        TEST_TRY_FUNC_CHECK(PLAY_MODE, == 0);

        // also check DLG_SIGNATURE here, because display is on for sure
        struct gui_task * current = gui_task_list.current;
        struct dialog * dialog = current->priv;
        TEST_TRY_FUNC_CHECK(MEM(dialog->type), == DLG_SIGNATURE);

        TEST_TRY_VOID(GUI_Control(BGMT_MENU, 0, 0, 0); msleep(500););
        TEST_TRY_FUNC_CHECK(MENU_MODE, == 0);
        TEST_TRY_FUNC_CHECK(PLAY_MODE, == 0);

        // task_create
        TEST_TRY_FUNC(task_create("test", 0x1c, 0x1000, test_task, 0));
        msleep(100);
        TEST_TRY_FUNC_CHECK(test_task_created, == 1);
        TEST_TRY_FUNC_CHECK_STR(get_task_name_from_id((unsigned int)get_current_task()), "run_test");

        // mq
        static struct msg_queue * mq = 0;
        int m = 0;
        TEST_TRY_FUNC_CHECK(mq = mq ? mq : (void*)msg_queue_create("test", 5), != 0);
        TEST_TRY_FUNC_CHECK(msg_queue_post(mq, 0x1234567), == 0);
        TEST_TRY_FUNC_CHECK(msg_queue_receive(mq, (struct event **) &m, 500), == 0);
        TEST_TRY_FUNC_CHECK(m, == 0x1234567);
        TEST_TRY_FUNC_CHECK(msg_queue_receive(mq, (struct event **) &m, 500), != 0);

        // sem
        static struct semaphore * sem = 0;
        TEST_TRY_FUNC_CHECK(sem = sem ? sem : create_named_semaphore("test", 1), != 0);
        TEST_TRY_FUNC_CHECK(take_semaphore(sem, 500), == 0);
        TEST_TRY_FUNC_CHECK(take_semaphore(sem, 500), != 0);
        TEST_TRY_FUNC_CHECK(give_semaphore(sem), == 0);
        TEST_TRY_FUNC_CHECK(take_semaphore(sem, 500), == 0);
        TEST_TRY_FUNC_CHECK(give_semaphore(sem), == 0);

        // recursive lock
        static void * rlock = 0;
        TEST_TRY_FUNC_CHECK(rlock = rlock ? rlock : CreateRecursiveLock(0), != 0);
        TEST_TRY_FUNC_CHECK(AcquireRecursiveLock(rlock, 500), == 0);
        TEST_TRY_FUNC_CHECK(AcquireRecursiveLock(rlock, 500), == 0);
        TEST_TRY_FUNC_CHECK(ReleaseRecursiveLock(rlock), == 0);
        TEST_TRY_FUNC_CHECK(ReleaseRecursiveLock(rlock), == 0);
        TEST_TRY_FUNC_CHECK(ReleaseRecursiveLock(rlock), != 0);

        // file I/O

        FILE* f;
        TEST_TRY_FUNC_CHECK(f = FIO_CreateFileEx(CARD_DRIVE"test.dat"), != (int)INVALID_PTR);
        TEST_TRY_FUNC_CHECK(FIO_WriteFile(f, (void*)ROMBASEADDR, 0x10000), == 0x10000);
        TEST_TRY_FUNC_CHECK(FIO_WriteFile(f, (void*)ROMBASEADDR, 0x10000), == 0x10000);
        TEST_TRY_VOID(FIO_CloseFile(f));
        uint32_t size;
        TEST_TRY_FUNC_CHECK(FIO_GetFileSize(CARD_DRIVE"test.dat", &size), == 0);
        TEST_TRY_FUNC_CHECK(size, == 0x20000);
        void* p;
        TEST_TRY_FUNC_CHECK(p = alloc_dma_memory(0x20000), != (int)INVALID_PTR);
        TEST_TRY_FUNC_CHECK(f = FIO_Open(CARD_DRIVE"test.dat", O_RDONLY | O_SYNC), != (int)INVALID_PTR);
        TEST_TRY_FUNC_CHECK(FIO_ReadFile(f, p, 0x20000), == 0x20000);
        TEST_TRY_VOID(FIO_CloseFile(f));
        TEST_TRY_VOID(free_dma_memory(p));

        {
        int count = 0;
        FILE* f = FIO_CreateFileEx(CARD_DRIVE"test.dat");
        for (int i = 0; i < 1000; i++)
            count += FIO_WriteFile(f, "Will it blend?\n", 15);
        FIO_CloseFile(f);
        TEST_TRY_FUNC_CHECK(count, == 1000*15);
        }

        TEST_TRY_FUNC_CHECK(FIO_RemoveFile(CARD_DRIVE"test.dat"), == 0);

        // sw1
        TEST_TRY_VOID(SW1(1,100));
        TEST_TRY_FUNC_CHECK(HALFSHUTTER_PRESSED, == 1);
        TEST_TRY_VOID(SW1(0,100));
        TEST_TRY_FUNC_CHECK(HALFSHUTTER_PRESSED, == 0);

        beep();
    }
    FIO_CloseFile(log);


    NotifyBox(10000, "Test complete.\n%d passed, %d failed.", passed_tests, failed_tests);
}

#if defined(CONFIG_7D)
static void rpc_test_task(void* unused)
{
    uint32_t loops = 0;

    ml_rpc_verbose(1);
    while(1)
    {
        msleep(50);

        ml_rpc_send(ML_RPC_PING, *(volatile uint32_t *)0xC0242014, 0, 0, 1);
        loops++;
    }
    ml_rpc_verbose(0);
}
#endif

static void stress_test_task(void* unused)
{
    NotifyBox(10000, "Stability Test..."); msleep(2000);

    msleep(2000);

    #ifndef CONFIG_50D // taking pics while REC crashes with Canon firmware too
    #ifndef CONFIG_5DC // no movie mode :)
    ensure_movie_mode();
    msleep(1000);
    for (int i = 0; i <= 5; i++)
    {
        NotifyBox(1000, "Pics while recording: %d", i);
        movie_start();
        msleep(1000);
        lens_take_picture(64, 0);
        msleep(1000);
        lens_take_picture(64, 0);
        msleep(1000);
        lens_take_picture(64, 0);
        while (lens_info.job_state) msleep(100);
        while (!lv) msleep(100);
        msleep(1000);
        movie_end();
        msleep(2000);
    }
    #endif
    #endif

    msleep(2000);

    extern struct semaphore * gui_sem;

    msleep(2000);

    for (int i = 0; i <= 1000; i++)
    {
        NotifyBox(1000, "ML menu toggle: %d", i);

        if (i == 250)
        {
            msleep(2000);
            gui_stop_menu();
            msleep(500);
            if (!lv) force_liveview();
        }

        if (i == 500)
        {
            msleep(2000);
            gui_stop_menu();
            msleep(500);
            ensure_movie_mode();
            movie_start();
        }

        if (i == 750)
        {
            msleep(2000);
            gui_stop_menu();
            msleep(500);
            movie_end();
            msleep(2000);
            fake_simple_button(BGMT_PLAY);
            msleep(1000);
        }

        give_semaphore(gui_sem);
        msleep(rand()%100);
        info_led_blink(1,50,50);

    }
    msleep(2000);
    gui_stop_menu();
    msleep(1000);
    if (!lv) force_liveview();
    msleep(2000);

#ifndef CONFIG_5DC // no cropmarks implemented
    NotifyBox(1000, "Cropmarks preview...");
    select_menu_by_name("Overlay", "Cropmarks");
    give_semaphore( gui_sem );
    msleep(500);
    menu_open_submenu();
    msleep(100);
    for (int i = 0; i <= 100; i++)
    {
        fake_simple_button(BGMT_WHEEL_RIGHT);
        msleep(rand()%500);
    }
    gui_stop_menu();
    msleep(2000);
#endif

    NotifyBox(1000, "ML menu scroll...");
    give_semaphore(gui_sem);
    msleep(1000);
    for (int i = 0; i <= 5000; i++)
    {
        static int dir = 0;
        switch(dir)
        {
            case 0: fake_simple_button(BGMT_WHEEL_LEFT); break;
            case 1: fake_simple_button(BGMT_WHEEL_RIGHT); break;
            case 2: fake_simple_button(BGMT_WHEEL_UP); break;
            case 3: fake_simple_button(BGMT_WHEEL_DOWN); break;
            case 4: fake_simple_button(BGMT_INFO); break;
            case 5: fake_simple_button(BGMT_MENU); break;
            //~ case 6: fake_simple_button(BGMT_PRESS_ZOOMIN_MAYBE); break;
        }
        dir = mod(dir + rand()%3 - 1, 7);
        msleep(MIN_MSLEEP);
    }
    gui_stop_menu();

    msleep(2000);

#ifdef FEATURE_PLAY_COMPARE_IMAGES
    beep();
    fake_simple_button(BGMT_PLAY); msleep(1000);
    for (int i = 0; i < 100; i++)
    {
        NotifyBox(1000, "PLAY: image compare: %d", i);
        playback_compare_images_task(1);
    }
    get_out_of_play_mode();
    msleep(2000);
#endif

#ifdef FEATURE_PLAY_EXPOSURE_FUSION
    fake_simple_button(BGMT_PLAY); msleep(1000);
    for (int i = 0; i < 10; i++)
    {
        NotifyBox(1000, "PLAY: exposure fusion: %d", i);
        expfuse_preview_update_task(1);
    }
    get_out_of_play_mode();
    msleep(2000);
#endif

#ifdef FEATURE_PLAY_422
    fake_simple_button(BGMT_PLAY); msleep(1000);
    for (int i = 0; i < 100; i++)
    {
        NotifyBox(1000, "PLAY: 422 scrolling: %d", i);
        play_next_422_task(1);
    }
    get_out_of_play_mode();
    msleep(2000);
#endif

    fake_simple_button(BGMT_PLAY); msleep(1000);
    for (int i = 0; i < 50; i++)
    {
        NotifyBox(1000, "PLAY scrolling: %d", i);
        next_image_in_play_mode(1);
    }
    extern int timelapse_playback;
    timelapse_playback = 1;
    for (int i = 0; i < 50; i++)
    {
        NotifyBox(1000, "PLAY scrolling: %d", i+50);
        msleep(200);
    }
    timelapse_playback = 0;
    get_out_of_play_mode();

    msleep(2000);

    if (!lv) force_liveview();

#ifdef CONFIG_LIVEVIEW
    for (int i = 0; i <= 100; i++)
    {
        int r = rand()%3;
        set_lv_zoom(r == 0 ? 1 : r == 1 ? 5 : 10);
        NotifyBox(1000, "LV zoom test: %d", i);
        msleep(rand()%200);
    }
    set_lv_zoom(1);
    msleep(2000);

#ifdef CONFIG_EXPSIM
    for (int i = 0; i <= 100; i++)
    {
        set_expsim(i%3);
        NotifyBox(1000, "ExpSim toggle: %d", i/10);
        msleep(rand()%100);
    }

    msleep(2000);
#endif
#ifdef FEATURE_EXPO_OVERRIDE
    for (int i = 0; i <= 100; i++)
    {
        bv_toggle(0, 1);
        NotifyBox(1000, "Exp.Override toggle: %d", i/10);
        msleep(rand()%100);
    }
    msleep(2000);
#endif
#endif

/*    for (int i = 0; i < 100; i++)
    {
        NotifyBox(1000, "Disabling Canon GUI (%d)...", i);
        canon_gui_disable();
        msleep(rand()%300);
        canon_gui_enable();
        msleep(rand()%300);
    } */

    msleep(2000);

    NotifyBox(10000, "LCD backlight...");
    int old_backlight_level = backlight_level;
    for (int i = 0; i < 5; i++)
    {
        for (int k = 1; k <= 7; k++)
        {
            set_backlight_level(k);
            msleep(50);
        }
        for (int k = 7; k >= 1; k--)
        {
            set_backlight_level(k);
            msleep(50);
        }
    }
    set_backlight_level(old_backlight_level);

#ifndef CONFIG_5DC // no LV
    if (!lv) force_liveview();
    for (int k = 0; k < 10; k++)
    {
        NotifyBox(1000, "LiveView / Playback (%d)...", k*10);
        fake_simple_button(BGMT_PLAY);
        msleep(rand() % 1000);
        SW1(1, rand()%100);
        SW1(0, rand()%100);
        msleep(rand() % 1000);
    }
    if (!lv) force_liveview();
    msleep(2000);
    lens_set_rawiso(0);
    for (int k = 0; k < 5; k++)
    {
        NotifyBox(1000, "LiveView gain test: %d", k*20);
        for (int i = 0; i <= 16; i++)
        {
            set_display_gain_equiv(1<<i);
            msleep(100);
        }
        for (int i = 16; i >= 0; i--)
        {
            set_display_gain_equiv(1<<i);
            msleep(100);
        }
    }
    set_display_gain_equiv(0);
#endif

    msleep(1000);

    for (int i = 0; i <= 10; i++)
    {
        NotifyBox(1000, "LED blinking: %d", i*10);
        info_led_blink(10, i*3, (10-i)*3);
    }

    msleep(2000);

    for (int i = 0; i <= 100; i++)
    {
        NotifyBox(1000, "Redraw test: %d", i);
        msleep(50);
        redraw();
        msleep(50);
    }

    msleep(2000);

    NotifyBox(10000, "Menu scrolling");
    fake_simple_button(BGMT_MENU);
    msleep(1000);
    for (int i = 0; i < 5000; i++)
        fake_simple_button(BGMT_WHEEL_LEFT);
    for (int i = 0; i < 5000; i++)
        fake_simple_button(BGMT_WHEEL_RIGHT);
    SW1(1,0);
    SW1(0,0);

    stress_test_picture(2, 2000); // make sure we have at least 2 pictures for scrolling :)

    msleep(2000);

#if 0 // unsafe
    for (int i = 0; i <= 10; i++)
    {
        NotifyBox(1000, "Mode switching: %d", i*10);
        set_shooting_mode(SHOOTMODE_AUTO);    msleep(100);
        set_shooting_mode(SHOOTMODE_MOVIE);    msleep(2000);
        set_shooting_mode(SHOOTMODE_SPORTS);    msleep(100);
        set_shooting_mode(SHOOTMODE_NIGHT);    msleep(100);
        set_shooting_mode(SHOOTMODE_CA);    msleep(100);
        set_shooting_mode(SHOOTMODE_M);    msleep(100);
        ensure_bulb_mode(); msleep(100);
        set_shooting_mode(SHOOTMODE_TV);    msleep(100);
        set_shooting_mode(SHOOTMODE_AV);    msleep(100);
        set_shooting_mode(SHOOTMODE_P);    msleep(100);
    }

    stress_test_picture(2, 2000);
#endif

#ifndef CONFIG_5DC // no focus features
    if (!lv) force_liveview();
    NotifyBox(10000, "Focus tests...");
    msleep(2000);
    for (int i = 1; i <= 3; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            lens_focus( 1, i, 1, 0);
            lens_focus(-1, i, 1, 0);
        }
    }

    msleep(2000);
#endif

    NotifyBox(10000, "Expo tests...");

    if (!lv) force_liveview();
    msleep(1000);
    for (int i = KELVIN_MIN; i <= KELVIN_MAX; i += KELVIN_STEP)
    {
        NotifyBox(1000, "Kelvin: %d", i);
        lens_set_kelvin(i); msleep(200);
    }
    lens_set_kelvin(6500);

    stress_test_picture(2, 2000);

    set_shooting_mode(SHOOTMODE_M);
    msleep(1000);
    if (!lv) force_liveview();
    msleep(1000);

    for (int i = 72; i <= 136; i++)
    {
        NotifyBox(1000, "ISO: raw %d  ", i);
        lens_set_rawiso(i); msleep(200);
    }
    lens_set_iso(88);

    stress_test_picture(2, 2000);

    msleep(5000);
    if (!lv) force_liveview();
    msleep(1000);

#ifndef CONFIG_5DC // no LV
    for (int i = 0; i <= 100; i++)
    {
        NotifyBox(1000, "Pause LiveView: %d", i);
        PauseLiveView(); msleep(rand()%200);
        ResumeLiveView(); msleep(rand()%200);
    }

    stress_test_picture(2, 2000);
#endif

    msleep(2000);
    if (!lv) force_liveview();
    msleep(1000);

    for (int i = 0; i <= 100; i++)
    {
        NotifyBox(1000, "BMP overlay: %d", i);
        bmp_off(); msleep(rand()%200);
        bmp_on(); msleep(rand()%200);
    }

    stress_test_picture(2, 2000);

    msleep(2000);
    if (!lv) force_liveview();
    msleep(1000);

    for (int i = 0; i <= 100; i++)
    {
        NotifyBox(1000, "Display on/off: %d", i);
        display_off(); msleep(rand()%200);
        display_on(); msleep(rand()%200);
    }

    msleep(3000); // 60D: display on/off is slow and will continue a while after this

    stress_test_picture(2, 2000);

#ifndef CONFIG_5DC // no LV, bulb, movie
    NotifyBox(10000, "LiveView switch...");
    set_shooting_mode(SHOOTMODE_M);
    for (int i = 0; i < 21; i++)
    {
        fake_simple_button(BGMT_LV); msleep(rand()%200);
    }

    stress_test_picture(2, 2000);

    set_shooting_mode(SHOOTMODE_BULB);

    msleep(1000);
    NotifyBox(10000, "Bulb picture taking");
    bulb_take_pic(2000);
    bulb_take_pic(100);
    bulb_take_pic(1500);
    bulb_take_pic(10);
    bulb_take_pic(1000);
    bulb_take_pic(1);

    NotifyBox(10000, "Movie recording");
    ensure_movie_mode();
    msleep(1000);
    for (int i = 0; i <= 5; i++)
    {
        NotifyBox(10000, "Movie recording: %d", i);
        movie_start();
        msleep(5000);
        movie_end();
        msleep(5000);
    }

    stress_test_picture(2, 2000);
#endif

    NotifyBox(2000, "Test complete."); msleep(2000);
    NotifyBox(2000, "Is the camera still working?"); msleep(2000);
    NotifyBox(10000, ":)");
    //~ NotifyBox(10000, "Burn-in test (will take hours!)");
    //~ set_shooting_mode(SHOOTMODE_M);
    //~ xx_test2(0);

}

/*
static void stress_test_toggle_menu_item(char* menu_name, char* item_name)
{
    extern struct semaphore * gui_sem;
    select_menu_by_name(menu_name, item_name);
    if (!gui_menu_shown()) give_semaphore( gui_sem );
    msleep(400);
    fake_simple_button(BGMT_PRESS_SET);
    msleep(200);
    give_semaphore( gui_sem );
    msleep(200);
    return;
} */

static void stress_test_toggle_random_menu_item()
{
    extern struct semaphore * gui_sem;
    if (!gui_menu_shown()) give_semaphore( gui_sem );
    msleep(400);
    int dx = rand() % 20 - 10;
    int dy = rand() % 20 - 10;
    for (int i = 0; i < ABS(dx); i++)
        fake_simple_button(dx > 0 ? BGMT_WHEEL_RIGHT : BGMT_WHEEL_LEFT);
    msleep(200);
    for (int i = 0; i < ABS(dy); i++)
        fake_simple_button(dy > 0 ? BGMT_WHEEL_UP : BGMT_WHEEL_DOWN);
    msleep(200);
    fake_simple_button(BGMT_PRESS_SET);
    msleep(200);
    give_semaphore( gui_sem );
    msleep(200);
    return;
}

static void stress_test_random_action()
{
    switch (rand() % 50)
    {
        case 0:
            lens_take_picture(64, rand() % 2);
            return;
        case 1:
            fake_simple_button(BGMT_LV);
            return;
        case 2:
            fake_simple_button(BGMT_PLAY);
            return;
        case 3:
            fake_simple_button(BGMT_MENU);
            return;
        default:
            stress_test_toggle_random_menu_item();
    }
}

static void stress_test_random_task(void* unused)
{
    config_autosave = 0; // this will make many changes in menu, don't save them
    TASK_LOOP
    {
        stress_test_random_action();
        //~ stress_test_toggle_menu_item("Play", "Zoom in PLAY mode");
        msleep(rand() % 1000);
    }
}

/*static void stress_test_random_action_simple()
{
    {
        switch (rand() % 4)
        {
            case 0:
            {
                stress_test_toggle_menu_item("Overlay", "Global Draw");
                return;
            }
            case 1:
                fake_simple_button(BGMT_PLAY);
                return;
            case 2:
                fake_simple_button(BGMT_MENU);
                return;
            case 3:
                fake_simple_button(BGMT_INFO);
                return;
        }
    }
}
*/

static void stress_test_menu_dlg_api_task(void* unused)
{
    msleep(2000);
    info_led_blink(5,50,50);
    extern struct semaphore * gui_sem;
    TASK_LOOP
    {
        give_semaphore(gui_sem);
        msleep(20);
    }
}

static void excessive_redraws_task()
{
    info_led_blink(5,50,1000);
    while(1)
    {
        if (gui_menu_shown()) menu_redraw();
        else redraw();
        msleep(10);
    }
}

static void bmp_fill_test_task()
{
    msleep(2000);
    while(1)
    {
        int x1 = rand() % 720;
        int x2 = rand() % 720;
        int y1 = rand() % 480;
        int y2 = rand() % 480;
        int xm = MIN(x1,x2); int xM = MAX(x1,x2);
        int ym = MIN(y1,y2); int yM = MAX(y1,y2);
        int w = xM-xm;
        int h = yM-ym;
        int c = rand() % 255;
        bmp_fill(c, xm, ym, w, h);
        msleep(20);
    }
}

extern void menu_self_test();

#endif // CONFIG_STRESS_TEST

void ui_lock(int x)
{
    int unlocked = UILOCK_NONE;
    prop_request_change(PROP_ICU_UILOCK, &unlocked, 4);
    msleep(50);
    prop_request_change(PROP_ICU_UILOCK, &x, 4);
    msleep(50);
}

#if CONFIG_DEBUGMSG

int mem_spy = 0;

int mem_spy_start = 0; // start from here
int mem_spy_bool = 0;           // only display booleans (0,1,-1)
int mem_spy_fixed_addresses = 0; // only look from a list of fixed addresses
const int mem_spy_addresses[] = {};//0xc0000044, 0xc0000048, 0xc0000057, 0xc00011cf, 0xc02000a8, 0xc02000ac, 0xc0201004, 0xc0201010, 0xc0201100, 0xc0201104, 0xc0201200, 0xc0203000, 0xc020301c, 0xc0203028, 0xc0203030, 0xc0203034, 0xc020303c, 0xc0203044, 0xc0203048, 0xc0210200, 0xc0210208, 0xc022001c, 0xc0220028, 0xc0220034, 0xc0220070, 0xc02200a4, 0xc02200d0, 0xc02200d4, 0xc02200d8, 0xc02200e8, 0xc02200ec, 0xc0220100, 0xc0220104, 0xc022010c, 0xc0220118, 0xc0220130, 0xc0220134, 0xc0220138, 0xc0222000, 0xc0222004, 0xc0222008, 0xc022200c, 0xc0223000, 0xc0223010, 0xc0223060, 0xc0223064, 0xc0223068, 0xc0224100, 0xc0224104, 0xc022d000, 0xc022d02c, 0xc022d074, 0xc022d1ec, 0xc022d1f0, 0xc022d1f4, 0xc022d1f8, 0xc022d1fc, 0xc022dd14, 0xc022f000, 0xc022f004, 0xc022f200, 0xc022f210, 0xc022f214, 0xc022f340, 0xc022f344, 0xc022f430, 0xc022f434, 0xc0238060, 0xc0238064, 0xc0238080, 0xc0238084, 0xc0238098, 0xc0242010, 0xc0300000, 0xc0300100, 0xc0300104, 0xc0300108, 0xc0300204, 0xc0400004, 0xc0400008, 0xc0400018, 0xc040002c, 0xc0400080, 0xc0400084, 0xc040008c, 0xc04000b4, 0xc04000c0, 0xc04000c4, 0xc04000cc, 0xc0410000, 0xc0410008, 0xc0500080, 0xc0500088, 0xc0500090, 0xc0500094, 0xc05000a0, 0xc05000a8, 0xc05000b0, 0xc05000b4, 0xc05000c0, 0xc05000c4, 0xc05000c8, 0xc05000cc, 0xc05000d0, 0xc05000d4, 0xc05000d8, 0xc0520000, 0xc0520004, 0xc0520008, 0xc052000c, 0xc0520014, 0xc0520018, 0xc0720000, 0xc0720004, 0xc0720008, 0xc072000c, 0xc0720014, 0xc0720024, 0xc07200ec, 0xc07200f0, 0xc0720100, 0xc0720104, 0xc0720108, 0xc072010c, 0xc0720110, 0xc0720114, 0xc0720118, 0xc072011c, 0xc07201c8, 0xc0720200, 0xc0720204, 0xc0720208, 0xc072020c, 0xc0720210, 0xc0800008, 0xc0800014, 0xc0800018, 0xc0820000, 0xc0820304, 0xc0820308, 0xc082030c, 0xc0820310, 0xc0820318, 0xc0920000, 0xc0920004, 0xc0920008, 0xc092000c, 0xc0920010, 0xc0920100, 0xc0920118, 0xc092011c, 0xc0920120, 0xc0920124, 0xc0920204, 0xc0920208, 0xc092020c, 0xc0920210, 0xc0920220, 0xc0920224, 0xc0920238, 0xc0920320, 0xc0920344, 0xc0920348, 0xc0920354, 0xc0920358, 0xc0a00000, 0xc0a00008, 0xc0a0000c, 0xc0a00014, 0xc0a00018, 0xc0a0001c, 0xc0a00020, 0xc0a00024, 0xc0a00044, 0xc0a10008 };
int mem_spy_len = 0x10000/4;    // look at ### int32's; use only when mem_spy_fixed_addresses = 0
//~ int mem_spy_len = COUNT(mem_spy_addresses); // use this when mem_spy_fixed_addresses = 1

int mem_spy_count_lo = 5; // how many times is a value allowed to change
int mem_spy_count_hi = 50; // (limits)
int mem_spy_freq_lo =  0;
int mem_spy_freq_hi =  0;  // or check frequecy between 2 limits (0 = disable)
int mem_spy_value_lo = 0;
int mem_spy_value_hi = 0;  // or look for a specific range of values (0 = disable)
int mem_spy_start_time = 30;  // ignore values changing early (these are noise)


static int* dbg_memmirror = 0;
static int* dbg_memchanges = 0;

static int dbg_memspy_get_addr(int i)
{
    if (mem_spy_fixed_addresses)
        return mem_spy_addresses[i];
    else
        return mem_spy_start + i*4;
}

static void
mem_spy_select( void * priv, int unused)
{
    mem_spy = !mem_spy;
}

// for debugging purpises only
int _t = 0;
static int _get_timestamp(struct tm * t)
{
    return t->tm_sec + t->tm_min * 60 + t->tm_hour * 3600 + t->tm_mday * 3600 * 24;
}
static void _tic()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    _t = _get_timestamp(&now);
}
static int _toc()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    return _get_timestamp(&now) - _t;
}

static void dbg_memspy_init() // initial state of the analyzed memory
{
    bmp_printf(FONT_MED, 10,10, "memspy init @ %x ... (+%x) ... %x", mem_spy_start, mem_spy_len, mem_spy_start + mem_spy_len * 4);
    //~ msleep(2000);
    //mem_spy_len is number of int32's
    if (!dbg_memmirror) dbg_memmirror = SmallAlloc(mem_spy_len*4 + 100); // local copy of mem area analyzed
    if (!dbg_memmirror) return;
    if (!dbg_memchanges) dbg_memchanges = SmallAlloc(mem_spy_len*4 + 100); // local copy of mem area analyzed
    if (!dbg_memchanges) return;
    int i;
    //~ bmp_printf(FONT_MED, 10,10, "memspy alloc");
    int crc = 0;
    for (i = 0; i < mem_spy_len; i++)
    {
        uint32_t addr = dbg_memspy_get_addr(i);
        dbg_memmirror[i] = (int) MEMX(addr);
        dbg_memchanges[i] = 0;
        crc += dbg_memmirror[i];
        //~ bmp_printf(FONT_MED, 10,10, "memspy: %8x => %8x ", addr, dbg_memmirror[i]);
        //~ msleep(1000);
    }
    bmp_printf(FONT_MED, 10,10, "memspy OK: %x", crc);
    _tic();
}

static void dbg_memspy_update()
{
    static int init_done = 0;
    if (!init_done) dbg_memspy_init();
    init_done = 1;

    if (!dbg_memmirror) return;
    if (!dbg_memchanges) return;

    int elapsed_time = _toc();
    bmp_printf(FONT_MED, 50, 400, "%d ", elapsed_time);

    int i;
    int k=0;
    for (i = 0; i < mem_spy_len; i++)
    {
#ifdef CONFIG_VXWORKS
        uint32_t fnt = FONT_MED;
#else
        uint32_t fnt = FONT_SMALL;
#endif
        uint32_t addr = dbg_memspy_get_addr(i);
        int oldval = dbg_memmirror[i];
        int newval = (int) MEMX(addr);
        if (oldval != newval)
        {
            //~ bmp_printf(FONT_MED, 10,460, "memspy: %8x: %8x => %8x", addr, oldval, newval);
            dbg_memmirror[i] = newval;
            if (dbg_memchanges[i] < 1000000) dbg_memchanges[i]++;
#ifdef CONFIG_VXWORKS
            fnt = FONT(FONT_MED, COLOR_BLUE, COLOR_BG);
#else
            fnt = FONT(FONT_SMALL, 5, COLOR_BG);
#endif
            if (elapsed_time < mem_spy_start_time) dbg_memchanges[i] = 1000000; // so it will be ignored
        }
        //~ else continue;

        if (mem_spy_bool && newval != 0 && newval != 1 && newval != -1) continue;

        if (mem_spy_value_lo && newval < mem_spy_value_lo) continue;
        if (mem_spy_value_hi && newval > mem_spy_value_hi) continue;

        if (mem_spy_count_lo && dbg_memchanges[i] < mem_spy_count_lo) continue;
        if (mem_spy_count_hi && dbg_memchanges[i] > mem_spy_count_hi) continue;

        int freq = dbg_memchanges[i] / elapsed_time;
        if (mem_spy_freq_lo && freq < mem_spy_freq_lo) continue;
        if (mem_spy_freq_hi && freq > mem_spy_freq_hi) continue;

#ifdef CONFIG_VXWORKS
        int x =  10 + 16 * 22 * (k % 2);
        int y =  10 + 20 * (k / 2);
        bmp_printf(FONT_MED, "%8x:%2d:%8x", addr, dbg_memchanges[i], newval);
        k = (k + 1) % 30;
#else
        int x =  10 + 8 * 22 * (k % 4);
        int y =  10 + 12 * (k / 4);
        bmp_printf(fnt, x, y, "%8x:%2d:%8x", addr, dbg_memchanges[i], newval);
        k = (k + 1) % 120;
#endif
    }

    for (i = 0; i < 10; i++)
    {
#ifdef CONFIG_VXWORKS
        int x =  10 + 16 * 22 * (k % 2);
        int y =  10 + 20 * (k / 2);
        bmp_printf(FONT_MED, x, y, "                    ");
        k = (k + 1) % 30;
#else
        int x =  10 + 8 * 22 * (k % 4);
        int y =  10 + 12 * (k / 4);
        bmp_printf(FONT_SMALL, x, y, "                    ");
        k = (k + 1) % 120;
#endif
    }
}
#endif

#if CONFIG_DEBUGMSG
static void dbg_draw_props(int changed);
static unsigned dbg_last_changed_propindex = 0;

void
memfilt(void* m, void* M, int value)
{
    int k = 0;
    bmp_printf(FONT_SMALL, 0, 0, "%8x", value);
    for (void* i = m; i < M; i ++)
    {
        if ((*(uint8_t*)i) == value)
        {
            int x =  10 + 4 * 22 * (k % 8);
            int y =  10 + 12 * (k / 8);
            bmp_printf(FONT_SMALL, x, y, "%8x", i);
            k = (k + 1) % 240;
        }
    }
    int x =  10 + 4 * 22 * (k % 8);
    int y =  10 + 12 * (k / 8);
    bmp_printf(FONT_SMALL, x, y, "        ");
}
#endif

static int screenshot_sec = 0;

PROP_INT(PROP_ICU_UILOCK, uilock);

#ifdef CONFIG_ELECTRONIC_LEVEL

struct rolling_pitching
{
    uint8_t status;
    uint8_t cameraposture;
    uint8_t roll_sensor1;
    uint8_t roll_sensor2;
    uint8_t pitch_sensor1;
    uint8_t pitch_sensor2;
};
struct rolling_pitching level_data;

PROP_HANDLER(PROP_ROLLING_PITCHING_LEVEL)
{
    memcpy(&level_data, buf, 6);
}

void draw_electronic_level(int angle, int prev_angle, int force_redraw)
{
    if (!force_redraw && angle == prev_angle) return;

    int x0 = os.x0 + os.x_ex/2;
    int y0 = os.y0 + os.y_ex/2;
    int r = 200;
    draw_angled_line(x0, y0, r, prev_angle, 0);
    draw_angled_line(x0+1, y0+1, r, prev_angle, 0);
    draw_angled_line(x0, y0, r, angle, (angle % 900) ? COLOR_BLACK : COLOR_GREEN1);
    draw_angled_line(x0+1, y0+1, r, angle, (angle % 900) ? COLOR_WHITE : COLOR_GREEN2);
}

void disable_electronic_level()
{
    if (level_data.status == 2)
    {
        GUI_SetRollingPitchingLevelStatus(1);
        msleep(100);
    }
}

void show_electronic_level()
{
    static int prev_angle10 = 0;
    int force_redraw = 0;
    if (level_data.status != 2)
    {
        GUI_SetRollingPitchingLevelStatus(0);
        msleep(100);
        force_redraw = 1;
    }

    static int k = 0;
    k++;
    if (k % 10 == 0) force_redraw = 1;

    int angle100 = level_data.roll_sensor1 * 256 + level_data.roll_sensor2;
    int angle10 = angle100/10;
    draw_electronic_level(angle10, prev_angle10, force_redraw);
    draw_electronic_level(angle10 + 1800, prev_angle10 + 1800, force_redraw);
    //~ draw_line(x0, y0, x0 + r * cos(angle), y0 + r * sin(angle), COLOR_BLUE);
    prev_angle10 = angle10;

    if (angle10 > 1800) angle10 -= 3600;
    bmp_printf(FONT_MED, 0, 35, "%s%3d", angle10 < 0 ? "-" : angle10 > 0 ? "+" : " ", ABS(angle10/10));
}

#endif

#ifdef CONFIG_HEXDUMP

CONFIG_INT("hexdump", hexdump_addr, 0x5024);

int hexdump_enabled = 0;
int hexdump_digit_pos = 0; // 0...7, 8=all

static MENU_UPDATE_FUNC (hexdump_print)
{
    int fnt = MENU_FONT;
    int x = info->x;
    int y = info->y;
    bmp_printf(
        fnt,
        x, y,
        "HexDump : %8x",
        hexdump_addr
    );

    fnt = FONT(fnt, COLOR_WHITE, COLOR_RED);

    if (hexdump_digit_pos < 8)
        bmp_printf(
            fnt,
            x + font_large.width * (17 - hexdump_digit_pos), y,
            "%x",
            (hexdump_addr >> (hexdump_digit_pos * 4)) & 0xF
        );
}

static MENU_UPDATE_FUNC (hexdump_print_value_hex)
{
    MENU_SET_VALUE("%x",
        MEMX(hexdump_addr)
    );
}

static MENU_UPDATE_FUNC (hexdump_print_value_int32)
{
    MENU_SET_VALUE(
        "%d",
        MEMX(hexdump_addr)
    );
}

static MENU_UPDATE_FUNC (hexdump_print_value_int16)
{
    int value = MEMX(hexdump_addr);
    MENU_SET_VALUE(
        "%d %d",
        value & 0xFFFF, (value>>16) & 0xFFFF
    );
}

static MENU_UPDATE_FUNC (hexdump_print_value_int8)
{
    int value = MEMX(hexdump_addr);
    MENU_SET_VALUE(
        "%d %d %d %d",
        (int8_t)( value      & 0xFF),
        (int8_t)((value>>8 ) & 0xFF),
        (int8_t)((value>>16) & 0xFF),
        (int8_t)((value>>24) & 0xFF)
    );
}

static MENU_UPDATE_FUNC (hexdump_print_value_str)
{
    if (hexdump_addr & 0xF0000000) return;
    MENU_SET_VALUE(
        "%s",
        "Val string: %s",
        (char*)hexdump_addr
    );
}

static void
hexdump_toggle_value_int32(void * priv, int delta)
{
    MEM(hexdump_addr) += delta;
}

static void
hexdump_toggle_value_int16(void * priv, int delta)
{
    (*(int16_t*)(hexdump_addr+2)) += delta;
}

void hexdump_digit_toggle(void* priv, int dir)
{
    if (hexdump_digit_pos < 8)
    {
        int digit = (hexdump_addr >> (hexdump_digit_pos * 4)) & 0xF;
        digit = mod(digit + dir*(hexdump_digit_pos?1:4), 16);
        hexdump_addr &= ~(0xF << (hexdump_digit_pos * 4));
        hexdump_addr |= (digit << (hexdump_digit_pos * 4));
    }
    else
    {
        hexdump_addr += dir * 4;
    }
}

void hexdump_digit_pos_toggle(void* priv, int dir)
{
    hexdump_digit_pos = mod(hexdump_digit_pos + 1, 9);
}

int hexdump_prev = 0;
void hexdump_back(void* priv, int dir)
{
    hexdump_addr = hexdump_prev;
}
void hexdump_deref(void* priv, int dir)
{
    if (dir < 0) hexdump_back(priv, dir);
    hexdump_prev = hexdump_addr;
    hexdump_addr = MEMX(hexdump_addr);
}
#endif

static int crash_log_requested = 0;
void request_crash_log(int type)
{
    crash_log_requested = type;
}

static int core_dump_requested = 0;
static int core_dump_req_from = 0;
static int core_dump_req_size = 0;
void request_core_dump(int from, int size)
{
    core_dump_req_from = from;
    core_dump_req_size = size;
    core_dump_requested = 1;
}

int GetFreeMemForAllocateMemory()
{
    int a,b;
    GetMemoryInformation(&a,&b);
    return b;
}

#ifdef CONFIG_CRASH_LOG
static void save_crash_log()
{
    static char log_filename[100];

    int log_number = 0;
    for (log_number = 0; log_number < 100; log_number++)
    {
        snprintf(log_filename, sizeof(log_filename), crash_log_requested == 1 ? CARD_DRIVE "CRASH%02d.LOG" : CARD_DRIVE "ASSERT%02d.LOG", log_number);
        uint32_t size;
        if( FIO_GetFileSize( log_filename, &size ) != 0 ) break;
        if (size == 0) break;
    }

    FILE* f = FIO_CreateFileEx(log_filename);
    my_fprintf(f, "%s\n\n", get_assert_msg());
    my_fprintf(f,
        "Magic Lantern version : %s\n"
        "Mercurial changeset   : %s\n"
        "Built on %s by %s.\n",
        build_version,
        build_id,
        build_date,
        build_user);

    int M = GetFreeMemForAllocateMemory();
    int m = MALLOC_FREE_MEMORY;
    my_fprintf(f,
        "Free Memory  : %dK + %dK\n",
        m/1024, M/1024
    );

    FIO_CloseFile(f);

    msleep(1000);

    if (crash_log_requested == 1)
        NotifyBox(5000, "Crash detected - log file saved.\n"
                        "Pls send CRASH%02d.LOG to ML devs.\n"
                        "\n"
                        "%s", log_number, get_assert_msg());
    else
        info_led_blink(10,20,20);

}

static void crash_log_step()
{
    static int dmlog_saved = 0;
    if (crash_log_requested)
    {
        //~ beep();
        save_crash_log();
        crash_log_requested = 0;
        msleep(2000);
    }

    if (core_dump_requested)
    {
        NotifyBox(100000, "Saving core dump, please wait...\n");
        dump_seg(core_dump_req_from, core_dump_req_from + core_dump_req_size, CARD_DRIVE"COREDUMP.DAT");
        NotifyBox(10000, "Pls send COREDUMP.DAT to ML devs.\n");
        core_dump_requested = 0;
    }

    //~ bmp_printf(FONT_MED, 100, 100, "%x ", get_current_dialog_handler());
    extern thunk ErrForCamera_handler;
    if (get_current_dialog_handler() == (intptr_t)&ErrForCamera_handler)
    {
        if (!dmlog_saved)
        {
            beep();
            NotifyBox(10000, "Saving debug log...");
            call("dumpf");
        }
        dmlog_saved = 1;
    }
    else dmlog_saved = 0;
}
#endif

static void
debug_loop_task( void* unused ) // screenshot, draw_prop
{
    TASK_LOOP
    {
#ifdef CONFIG_HEXDUMP
        if (hexdump_enabled)
            bmp_hexdump(FONT_SMALL, 0, 480-120, hexdump_addr, 32*10);
#endif

        #ifdef FEATURE_SCREENSHOT
        if (screenshot_sec)
        {
            info_led_blink(1, 20, 1000-20-200);
            screenshot_sec--;
            if (!screenshot_sec)
                take_screenshot(1);
        }
        #endif

        #ifdef CONFIG_RESTORE_AFTER_FORMAT
        if (MENU_MODE)
        {
            HijackFormatDialogBox_main();
        }
        #endif

        #if CONFIG_DEBUGMSG
        if (draw_prop)
        {
            dbg_draw_props(dbg_last_changed_propindex);
            continue;
        }
        else if (mem_spy)
        {
            dbg_memspy_update();
            continue;
        }
        #endif

        #ifdef CONFIG_CRASH_LOG
        crash_log_step();
        #endif

        msleep(200);
    }
}

static void screenshot_start(void* priv, int delta)
{
    screenshot_sec = 10;
}

/*void screenshots_for_menu()
{
    msleep(1000);
    extern struct semaphore * gui_sem;
    give_semaphore(gui_sem);

    select_menu_by_name("Audio", "AGC");
    msleep(1000); call("dispcheck");

    select_menu_by_name("Expo", "ISO");
    msleep(1000); call("dispcheck");

    select_menu_by_name("Overlay", "Magic Zoom");
    msleep(1000); call("dispcheck");

    select_menu_by_name("Movie", "FPS override");
    msleep(1000); call("dispcheck");

    select_menu_by_name("Shoot", "Motion Detect");
    msleep(1000); call("dispcheck");

    select_menu_by_name("Focus", "Follow Focus");
    msleep(1000); call("dispcheck");

    select_menu_by_name("Display", "LV saturation");
    msleep(1000); call("dispcheck");

    select_menu_by_name("Prefs", "Powersave settings...");
    msleep(1000); call("dispcheck");

    select_menu_by_name("Debug", "Free Memory");
    msleep(1000); call("dispcheck");

    select_menu_by_name("Help", "About Magic Lantern");
    msleep(1000); call("dispcheck");
}
*/

static int draw_event = 0;

#if CONFIG_DEBUGMSG
static void
spy_print(
          void *            priv,
          int            x,
          int            y,
          int            selected
          )
{
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Spy %s/%s (s/q)",
               draw_prop ? "PROP" : "prop",
               mem_spy ? "MEM" : "mem"
               );
    menu_draw_icon(x, y, MNI_BOOL(draw_prop || draw_event || mem_spy), 0);
}

static void
lvbuf_display(
              void *            priv,
              int            x,
              int            y,
              int            selected
              )
{
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Dump Live View Buffers"
               );
}

static void lvbuf_select()
{
    if (lv)
    {
        call("lv_vram_dump");
        call("lv_ssdev_dump");
        //~ call("lv_yuv_dump");
        //~ call("lv_raw_dump2");
        //~ call("lv_faceyuv_dump");
    }
    else
        NotifyBox(5000, "Only Works In Live View!!!");
}
#endif

#ifdef FEATURE_SHOW_IMAGE_BUFFERS_INFO
static MENU_UPDATE_FUNC(image_buf_display)
{
    MENU_SET_VALUE(
        "%dx%d, %dx%d",
        vram_lv.width, vram_lv.height,
        vram_hd.width, vram_hd.height
    );
}
#endif

#ifdef FEATURE_SHOW_FREE_MEMORY

static volatile int max_stack_ack = 0;

static void max_stack_try(void* size) { max_stack_ack = (int) size; }

static int stack_size_crit(int x)
{
    int size = x * 1024;
    task_create("stack_try", 0x1e, size, max_stack_try, (void*) size);
    msleep(50);
    if (max_stack_ack == size) return 1;
    return -1;
}

static int max_shoot_malloc_mem = 0;
static int max_shoot_malloc_frag_mem = 0;
static char shoot_malloc_frag_desc[70] = "";

/* fixme: find a way to read the free stack memory from DryOS */
/* current workaround: compute it by trial and error when you press SET on Free Memory menu item */
static volatile int guess_mem_running = 0;
static void guess_free_mem_task(void* priv, int delta)
{
    /* reset values */
    max_stack_ack = 0;
    max_shoot_malloc_mem = 0;
    max_shoot_malloc_frag_mem = 0;

    bin_search(1, 1024, stack_size_crit);
    
    {
        struct memSuite * hSuite = shoot_malloc_suite_contig(0);
        if (!hSuite)
        {
            beep();
            guess_mem_running = 0;
            return;
        }
        ASSERT(hSuite->num_chunks == 1);
        max_shoot_malloc_mem = hSuite->size;
        shoot_free_suite(hSuite);
    }

    struct memSuite * hSuite = shoot_malloc_suite(0);
    if (!hSuite)
    {
        beep();
        guess_mem_running = 0;
        return;
    }
    max_shoot_malloc_frag_mem = hSuite->size;
    
    struct memChunk *currentChunk;
    int chunkAvail;
    int total = 0;
    
    currentChunk = GetFirstChunkFromSuite(hSuite);
    
    snprintf(shoot_malloc_frag_desc, sizeof(shoot_malloc_frag_desc), "");
    
    while(currentChunk)
    {
        chunkAvail = GetSizeOfMemoryChunk(currentChunk);
    
        int mb = 10*chunkAvail/1024/1024;
        STR_APPEND(shoot_malloc_frag_desc, mb%10 ? "%s%d.%d" : "%s%d", total ? "+" : "", mb/10, mb%10);
        total += chunkAvail;

        currentChunk = GetNextMemoryChunk(hSuite, currentChunk);
    }
    STR_APPEND(shoot_malloc_frag_desc, " MB.");
    ASSERT(max_shoot_malloc_frag_mem == total);
    
    shoot_free_suite(hSuite);

    menu_redraw();
    guess_mem_running = 0;
}

static void guess_free_mem()
{
    task_create("guess_mem", 0x1e, 0x4000, guess_free_mem_task, 0);
}

static MENU_UPDATE_FUNC(meminfo_display)
{
    int M = GetFreeMemForAllocateMemory();
    int m = MALLOC_FREE_MEMORY;

#ifdef CONFIG_VXWORKS
    MENU_SET_VALUE(
        "%dK",
        M/1024
    );
    if (M < 1024*1024) MENU_SET_WARNING(MENU_WARN_ADVICE, "Not enough free memory.");
#else

    int guess_needed = 0;
    int info_type = (int) entry->priv;
    switch (info_type)
    {
        case 0: // main entry
            MENU_SET_VALUE(
                "%dK + %dK",
                m/1024, M/1024
            );
            if (M < 1024*1024 || m < 128*1024) MENU_SET_WARNING(MENU_WARN_ADVICE, "Not enough free memory.");
            MENU_SET_ENABLED(1);
            MENU_SET_ICON(MNI_DICE, 0);
            break;

        case 1: // malloc
            MENU_SET_VALUE("%d K", m/1024);
            if (m < 128*1024) MENU_SET_WARNING(MENU_WARN_ADVICE, "Would be nice to have at least 128K free here.");
            break;

        case 2: // AllocateMemory
            MENU_SET_VALUE("%d K", M/1024);
            if (M < 1024*1024) MENU_SET_WARNING(MENU_WARN_ADVICE, "Canon code requires around 1 MB from here.");
            break;

        case 3: // task stack
            MENU_SET_VALUE("%d K", max_stack_ack/1024);
            guess_needed = 1;
            break;

        case 4: // shoot_malloc contig
            MENU_SET_VALUE("%d M", max_shoot_malloc_mem/1024/1024);
            guess_needed = 1;
            break;

        case 5: // shoot_malloc fragmented
            MENU_SET_VALUE("%d M", max_shoot_malloc_frag_mem/1024/1024);
            MENU_SET_HELP(shoot_malloc_frag_desc);
            guess_needed = 1;
            break;

        #if defined(CONFIG_MEMPATCH_CHECK)
        case 6: // autoexec size
        {
            extern uint32_t ml_reserved_mem;
            extern uint32_t ml_used_mem;

            if (ABS(ml_used_mem - ml_reserved_mem) < 1024) MENU_SET_VALUE(
                "%dK",
                ml_used_mem/1024
            );
            else MENU_SET_VALUE(
                "%dK of %dK",
                ml_used_mem/1024, ml_reserved_mem/1024
            );
            if (ml_reserved_mem < ml_used_mem)
                MENU_SET_WARNING(MENU_WARN_ADVICE, "ML uses too much memory!!");
            break;
        }
        #endif
    }

    if (guess_needed && !guess_mem_running)
    {
        /* check this once every 10 seconds (not more often) */
        static int aux = INT_MIN;
        if (should_run_polling_action(10000, &aux))
        {
            guess_mem_running = 1;
            guess_free_mem();
        }
    }

    if (guess_mem_running)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Trying to guess how much RAM we have...");
#endif
}
#endif

#ifdef FEATURE_SHOW_SHUTTER_COUNT
static MENU_UPDATE_FUNC(shuttercount_display)
{
    MENU_SET_VALUE(
        "%dK = %d+%d",
        (shutter_count_plus_lv_actuations + 500) / 1000,
        shutter_count, shutter_count_plus_lv_actuations - shutter_count
    );
    if (shutter_count_plus_lv_actuations > 50000)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Too many shutter actuations.");
}
#endif

#ifdef FEATURE_SHOW_CMOS_TEMPERATURE
static MENU_UPDATE_FUNC(efictemp_display)
{
    MENU_SET_VALUE(
        "%d deg C",
        EFIC_CELSIUS
    );
}
#endif

#if 0 // CONFIG_5D2
static void ambient_display(
    void *            priv,
    int            x,
    int            y,
    int            selected
)
{
    extern int lightsensor_raw_value;
    int ev = gain_to_ev_scaled(lightsensor_raw_value, 10);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Ambient light: %d.%d EV",
        ev/10, ev%10
    );
    menu_draw_icon(x, y, MNI_ON, 0);
}
#endif

#if CONFIG_DEBUGMSG
CONFIG_INT("prop.i", prop_i, 0);
CONFIG_INT("prop.j", prop_j, 0);
CONFIG_INT("prop.k", prop_k, 0);

static void prop_display(
    void *            priv,
    int            x,
    int            y,
    int            selected
)
{
    unsigned prop = (prop_i << 24) | (prop_j << 16) | (prop_k);
    int* data = 0;
    size_t len = 0;
    int err = prop_get_value(prop, (void **) &data, &len);
    bmp_printf(
        FONT_MED,
        x, y,
        "PROP %8x: %d: %8x %8x %8x %8x\n"
        "'%s' ",
        prop,
        len,
        len > 0x00 ? data[0] : 0,
        len > 0x04 ? data[1] : 0,
        len > 0x08 ? data[2] : 0,
        len > 0x0c ? data[3] : 0,
        strlen((const char *) data) < 100 ? (const char *) data : ""
    );
    menu_draw_icon(x, y, MNI_BOOL(!err), 0);
}

void prop_dump()
{
    FILE* f = FIO_CreateFileEx(CARD_DRIVE "ML/LOGS/PROP.LOG");
    FILE* g = FIO_CreateFileEx(CARD_DRIVE "ML/LOGS/PROP-STR.LOG");

    unsigned i, j, k;

    for( i=0 ; i<256 ; i++ )
    {
        if (i > 0x10 && i != 0x80) continue;
        for( j=0 ; j<=0xA ; j++ )
        {
            for( k=0 ; k<0x50 ; k++ )
            {
                unsigned prop = 0
                    | (i << 24)
                    | (j << 16)
                    | (k <<  0);

                bmp_printf(FONT_LARGE, 0, 0, "PROP %x...", prop);
                int* data = 0;
                size_t len = 0;
                int err = prop_get_value(prop, (void **) &data, &len);
                if (!err)
                {
                    my_fprintf(f, "\nPROP %8x: %5d:", prop, len );
                    my_fprintf(g, "\nPROP %8x: %5d:", prop, len );
                    for (unsigned int i = 0; i < (MIN(len,40)+3)/4; i++)
                    {
                        my_fprintf(f, "%8x ", data[i]);
                    }
                    if (strlen((const char *) data) < 100) my_fprintf(g, "'%s'", data);
                }
            }
        }
    }
    FIO_CloseFile(f);
    FIO_CloseFile(g);
    beep();
    redraw();
}

static void prop_toggle_i(void* priv, int unused) {prop_i = prop_i < 5 ? prop_i + 1 : prop_i == 5 ? 0xE : prop_i == 0xE ? 0x80 : 0; }
static void prop_toggle_j(void* priv, int unused) {prop_j = mod(prop_j + 1, 0x10); }
static void prop_toggle_k(void* priv, int dir) {if (dir < 0) prop_toggle_j(priv, dir); prop_k = mod(prop_k + 1, 0x51); }
#endif

#ifdef CONFIG_KILL_FLICKER
void menu_kill_flicker()
{
    gui_stop_menu();
    canon_gui_disable_front_buffer();
}
#endif


#ifdef FEATURE_SHOW_EDMAC_INFO

static int edmac_selection;

static void edmac_display_page(int i0, int x0, int y0)
{
    bmp_printf(
        FONT_MED,
        x0, y0,
        "EDM# Address  Size\n"
    );

    y0 += font_med.height * 2;
    
    for (int i = 0; i < 16; i++)
    {
        char msg[100];
        
        uint32_t base = edmac_get_base(i0+i);
        uint32_t addr = shamem_read(base + 8);
        union edmac_size_t
        {
            struct { short x, y; } size;
            uint32_t raw;
        };
        
        union edmac_size_t size = (union edmac_size_t) shamem_read(base + 0x10);
        
        int state = MEM(base + 0);
        int color = 
            state == 0 ? COLOR_GRAY(50) :   // inactive?
            state == 1 ? COLOR_GREEN1 :     // active?
            COLOR_RED;                      // no idea
        
        if (addr && size.size.x > 0 && size.size.y > 0)
        {
            snprintf(msg, sizeof(msg), "[%2d] %8x: %dx%d", i0+i, addr, size.size.x, size.size.y);
        }
        else
        {
            snprintf(msg, sizeof(msg), "[%2d] %8x: %x", i0+i, addr, size.raw);
        }
        
        if (color == COLOR_RED)
            STR_APPEND(msg, " (%x)", state);

        uint32_t conn_w  = edmac_get_connection(i0+i, EDMAC_DIR_WRITE);
        uint32_t conn_r  = edmac_get_connection(i0+i, EDMAC_DIR_READ);
        
        if (conn_r == 0xFF) { if (conn_w != 0) STR_APPEND(msg, " <w%x>", conn_w); }
        else if (conn_w == 0) { STR_APPEND(msg, " <r%x>", conn_r); }
        else { STR_APPEND(msg, " <%x,%x>", conn_w, conn_r); }

        bmp_printf(
            FONT(FONT_MED, color, COLOR_BLACK),
            x0, y0 + i * font_med.height, 
            msg
        );
    }
}

static void edmac_display_detailed(int channel)
{
    uint32_t base = edmac_get_base(channel);

    int x = 50;
    int y = 50;
    bmp_printf(
        FONT_LARGE,
        x, y,
        "EDMAC #%d - %x\n", 
        channel,
        base
    );
    y += font_large.height;
    
    /* http://magiclantern.wikia.com/wiki/Register_Map#EDMAC */

    uint32_t state = MEM(base + 0);
    uint32_t flags = shamem_read(base + 4);
    uint32_t addr = shamem_read(base + 8);

    union edmac_size_t
    {
        struct { short x, y; } size;
        uint32_t raw;
    };
    
    union edmac_size_t size_n = (union edmac_size_t) shamem_read(base + 0x0C);
    union edmac_size_t size_b = (union edmac_size_t) shamem_read(base + 0x10);
    union edmac_size_t size_a = (union edmac_size_t) shamem_read(base + 0x14);
    
    uint32_t off1b = shamem_read(base + 0x18);
    uint32_t off2b = shamem_read(base + 0x1C);
    uint32_t off1a = shamem_read(base + 0x20);
    uint32_t off2a = shamem_read(base + 0x24);
    uint32_t off3  = shamem_read(base + 0x28);

    uint32_t conn_w  = edmac_get_connection(channel, EDMAC_DIR_WRITE);
    uint32_t conn_r  = edmac_get_connection(channel, EDMAC_DIR_READ);
    
    bmp_printf(FONT_MED, 50, y += font_med.height, "Address    : %8x ", addr);
    bmp_printf(FONT_MED, 50, y += font_med.height, "State      : %8x ", state);
    bmp_printf(FONT_MED, 50, y += font_med.height, "Flags      : %8x ", flags);
    y += font_med.height;
    bmp_printf(FONT_MED, 50, y += font_med.height, "Size A     : %8x (%d x %d) ", size_a.raw, size_a.size.x, size_a.size.y);
    bmp_printf(FONT_MED, 50, y += font_med.height, "Size B     : %8x (%d x %d) ", size_b.raw, size_b.size.x, size_b.size.y);
    bmp_printf(FONT_MED, 50, y += font_med.height, "Size N     : %8x (%d x %d) ", size_n.raw, size_n.size.x, size_n.size.y);
    y += font_med.height;
    bmp_printf(FONT_MED, 50, y += font_med.height, "off1a      : %8x ", off1a);
    bmp_printf(FONT_MED, 50, y += font_med.height, "off1b      : %8x ", off1b);
    bmp_printf(FONT_MED, 50, y += font_med.height, "off2a      : %8x ", off2a);
    bmp_printf(FONT_MED, 50, y += font_med.height, "off2b      : %8x ", off2b);
    bmp_printf(FONT_MED, 50, y += font_med.height, "off3       : %8x ", off3);
    y += font_med.height;
    bmp_printf(FONT_MED, 50, y += font_med.height, "Connection : write=0x%x read=0x%x ", conn_w, conn_r);
    
    #ifdef CONFIG_5D3
    /**
     * ConnectReadEDmac(channel, conn)
     * RAM:edmac_register_interrupt(channel, cbr_handler, ...)
     * => *(8 + 32*arg0 + *0x12400) = arg1
     * and also: *(12 + 32*arg0 + *0x12400) = arg1
     */
    uint32_t cbr1 = MEM(8 + 32*(channel) + MEM(0x12400));
    uint32_t cbr2 = MEM(12 + 32*(channel) + MEM(0x12400));
    bmp_printf(FONT_MED, 50, y += font_med.height, "CBR handler: %8x %s", cbr1, asm_guess_func_name_from_string(cbr1));
    bmp_printf(FONT_MED, 50, y += font_med.height, "CBR abort  : %8x %s", cbr2, asm_guess_func_name_from_string(cbr2));
    #endif
}

static MENU_UPDATE_FUNC(edmac_display)
{
    if (!info->x) return;
    info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
    bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

    if (edmac_selection == 0) // overview
    {
        edmac_display_page(0, 0, 30);
        edmac_display_page(16, 360, 30);

        //~ int x = 20;
        bmp_printf(
            FONT_MED,
            20, 450, "EDMAC state: "
        );

        bmp_printf(
            FONT(FONT_MED, COLOR_GRAY(50), COLOR_BLACK),
            20+200, 450, "inactive"
        );

        bmp_printf(
            FONT(FONT_MED, COLOR_GREEN1, COLOR_BLACK),
            20+350, 450, "running"
        );

        bmp_printf(
            FONT_MED,
            720 - font_med.width * 13, 450, "[Scrollwheel]"
        );
    }
    else // detailed view
    {
        edmac_display_detailed(edmac_selection - 1);
    }
}
#endif

extern void menu_open_submenu();
extern MENU_UPDATE_FUNC(tasks_print);
extern MENU_UPDATE_FUNC(batt_display);
extern MENU_SELECT_FUNC(tasks_toggle_flags);
extern void peaking_benchmark();

extern int show_cpu_usage_flag;


static struct menu_entry debug_menus[] = {
#ifdef CONFIG_HEXDUMP
    {
        .name = "Memory Browser",
        .priv = &hexdump_enabled,
        .max = 1,
        .help = "Display memory contents in real-time (hexdump).",
        .children =  (struct menu_entry[]) {
            {
                .name = "HexDump",
                .priv = &hexdump_addr,
                .select = hexdump_digit_toggle,
                .update = hexdump_print,
                .help = "Address to be analyzed"
            },
            {
                .name = "Edit digit",
                .priv = &hexdump_digit_pos,
                .max = 8,
                .help = "Choose which digit to edit (0-7) or the entire nuber (8)."
            },
            {
                .name = "Pointer dereference",
                .select = hexdump_deref,
                .help = "Changes address to *(int*)addr [SET] or goes back [PLAY]."
            },
            {
                .name = "Val hex32",
                //~.display = hexdump_print_value_hex,
                .select = hexdump_toggle_value_int32,
                .help = "Value as hex."
            },
            {
                .name = "Val int32",
                //~.display = hexdump_print_value_int32,
                .select = hexdump_toggle_value_int32,
                .help = "Value as int32."
            },
            {
                .name = "Val int16",
                //~.display = hexdump_print_value_int16,
                .select = hexdump_toggle_value_int16,
                .help = "Value as 2 x int16. Toggle: changes second value."
            },
            {
                .name = "Val int8",
                //~.display = hexdump_print_value_int8,
                .help = "Value as 4 x int8."
            },
            {
                .name = "Val string",
                //~.display = hexdump_print_value_str,
                .help = "Value as string."
            },
            MENU_EOL
        },
    },
#endif
    /*{
        .name        = "Flashlight",
        .select        = flashlight_lcd,
        .select_reverse = flashlight_frontled,
        .help = "Turn on the front LED [PLAY] or make display bright [SET]."
    },*/
    #ifdef FEATURE_SCREENSHOT
    {
        .name = "Screenshot - 10s",
        .select     = screenshot_start,
        .help = "Screenshot after 10 seconds => VRAMx.BMP / VRAMx.422.",
    },
    #endif
/*    {
        .name = "Menu screenshots",
        .select     = (void (*)(void*,int))run_in_separate_task,
        .priv = screenshots_for_menu,
        .help = "Take a screenshot for each ML menu.",
    }, */
#if CONFIG_DEBUGMSG
    #ifndef CONFIG_5DC
    {
        .name = "Draw palette",
        .select        = (void(*)(void*,int))bmp_draw_palette,
        .help = "Display a test pattern to see the color palette."
    },
    #endif
    {
        .name = "Spy prop/evt/mem",
        .select        = draw_prop_select,
        .select_Q = mem_spy_select,
        //~.display    = spy_print,
        .help = "Spy properties / events / memory addresses which change."
    },
/*    {
        .name        = "Dialog test",
        .select        = dlg_test,
        .help = "Dialog templates (up/dn) and color palettes (left/right)"
    },*/
    {
        .name        = "Dump ROM and RAM",
        .select        = dump_rom,
        .help = "0.BIN:0-0FFFFFFF, ROM0.BIN:F0000000, ROM1.BIN:F8000000"
    },
#endif
#ifdef CONFIG_40D
    {
        .name        = "Dump camera logs",
        .select      = dump_logs,
        .help = "Dump camera logs to card."
    },
#endif
#ifdef FEATURE_DONT_CLICK_ME
    {
        .name        = "Don't click me!",
        .priv =         run_test,
        .select        = (void(*)(void*,int))run_in_separate_task,
        .help = "The camera may turn into a 1DX or it may explode."
    },
#endif
#ifdef FEATURE_DEBUG_INTERCEPT
    {
        .name        = "DM Log",
        .priv        = j_debug_intercept,
        .select      = (void(*)(void*,int))run_in_separate_task,
        .help = "Log DebugMessages"
    },
    {
        .name        = "TryPostEvent Log",
        .priv        = j_tp_intercept,
        .select      = (void(*)(void*,int))run_in_separate_task,
        .help = "Log TryPostEvents"
    },
#endif
#if defined(CONFIG_7D)
    {
        .name        = "LV dumping",
        .priv = (int*) 0x60D8,
        .max = 6,
        .icon_type = IT_DICE_OFF,
        .help = "Silent picture mode: simple, burst, continuous or high-resolution.",
        .choices = (const char *[]) {"Disabled", "A:/.JPEG", "B:/.JPEG", "A:/.422", "B:/.422", "A:/.JPEG/.422", "B:/.JPEG/.422"},
    },
#endif
#ifdef CONFIG_ISO_TESTS
    {
        .name        = "ISO tests...",
        .select        = menu_open_submenu,
        .help = "Computes camera response curve for certain ISO values.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Response curve @ current ISO",
                .priv = iso_response_curve_current,
                .select = (void (*)(void*,int))run_in_separate_task,
                .help = "MOV: point camera at smth bright, 1/30, f1.8. Takes 1 min.",
            },
            {
                .name = "Test ISO 100x/160x/80x series",
                .priv = iso_response_curve_160,
                .select = (void (*)(void*,int))run_in_separate_task,
                .help = "ISO 100,200..3200, 80eq,160/160eq...2500/eq. Takes 20 min.",
            },
            {
                .name = "Test 70x/65x/50x series",
                .priv = iso_response_curve_logain,
                .select = (void (*)(void*,int))run_in_separate_task,
                .help = "ISOs with -0.5/-0.7/-0.8 EV of DIGIC gain. Takes 20 mins.",
            },
            {
                .name = "Test HTP series",
                .priv = iso_response_curve_htp,
                .select = (void (*)(void*,int))run_in_separate_task,
                .help = "Full-stop ISOs with HTP on. Also with -1 EV of DIGIC gain.",
            },
            {
                .name = "Movie test",
                .priv = iso_movie_test,
                .select = (void (*)(void*,int))run_in_separate_task,
                .help = "Records two test movies, changing settings every 2 seconds.",
            },
            MENU_EOL
        },
    },
#endif
#ifdef CONFIG_STRESS_TEST
    {
        .name        = "Burn-in tests",
        .select        = menu_open_submenu,
        .help = "Tests to make sure Magic Lantern is stable and won't crash.",
        .submenu_width = 650,
        //.essential = FOR_MOVIE | FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Stubs API test",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = stub_test_task,
                .help = "Tests Canon functions called by ML. SET=once, PLAY=100x."
            },
            #ifdef CONFIG_PICOC // the tests depend on some picoc functions
            {
                .name = "Menu integrity test",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = menu_self_test,
                .help = "Internal menu tests: duplicates, wrap around etc.",
            },
            #endif
            #if defined(CONFIG_7D)
            {
                .name = "RPC reliability test (infinite loop)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = rpc_test_task,
                .help = "Flood master with RPC requests and print delay. "
            },
            #endif
            {
                .name = "Quick test (around 15 min)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = stress_test_task,
                .help = "A quick test which covers basic functionality. "
            },
            {
                .name = "Random tests (infinite loop)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = stress_test_random_task,
                .help = "A thorough test which randomly enables functions from menu. "
            },
            {
                .name = "Menu backend test (infinite)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = stress_test_menu_dlg_api_task,
                .help = "Tests proper usage of Canon API calls in ML menu backend."
            },
            {
                .name = "Redraw test (infinite)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = excessive_redraws_task,
                .help = "Causes excessive redraws for testing the graphics backend",
            },
            {
                .name = "Rectangle test (infinite)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = bmp_fill_test_task,
                .help = "Stresses graphics bandwith. Run this while recording.",
            },
            MENU_EOL,
        }
    },
#if 0
    {
        .name        = "Fault emulation...",
        .select        = menu_open_submenu,
        .help = "Causes intentionally wrong behavior to see DryOS reaction.",
        //.essential = FOR_MOVIE | FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Create a stuck task",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = frozen_task,
                .help = "Creates a task which will become stuck in an infinite loop."
            },
            {
                .name = "Freeze GUI task",
                .select = freeze_gui_task,
                .help = "Freezes main GUI task. Camera will stop reacting to buttons."
            },
            {
                .name = "Division by zero",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = divzero_task,
                .help = "Performs some math operations which will divide by zero."
            },
            {
                .name = "Allocate 1MB of RAM",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = alloc_1M_task,
                .help = "Allocates 1MB RAM from system memory, without freeing it."
            },
            MENU_EOL,
        }
    },
#endif
#endif
#ifdef CONFIG_BENCHMARKS
    {
        .name        = "Benchmarks",
        .select        = menu_open_submenu,
        .help = "Check how fast is your camera. Card, CPU, graphics...",
        .submenu_width = 650,
        //.essential = FOR_MOVIE | FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Card R/W benchmark (5 min)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = card_benchmark_task,
                .help = "Check card read/write speed. Uses a 1GB temporary file."
            },
            {
                .name = "Card buffer benchmark (inf)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = card_bufsize_benchmark_task,
                .help = "Experiment for finding optimal write buffer sizes.",
                .help2 = "Results saved in BENCH.LOG."
            },
            #ifdef CONFIG_5D3
            {
                .name = "CF+SD write benchmark (1 min)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = twocard_benchmark_task,
                .help = "Write speed on both CF and SD cards at the same time."
            },
            #endif
            {
                .name = "Memory benchmark (1 min)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = mem_benchmark_task,
                .help = "Check memory read/write speed."
            },
            #ifdef FEATURE_FOCUS_PEAK
            {
                .name = "Focus peaking benchmark (30s)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = peaking_benchmark,
                .help = "Check how fast peaking runs in PLAY mode (1000 iterations)."
            },
            #endif
            MENU_EOL,
        }
    },
#endif
    MENU_PLACEHOLDER("Mem Protection"), // module mem_prot
    MENU_PLACEHOLDER("Show MRC regs"), // module mrc_dump
#ifdef FEATURE_SHOW_TASKS
    {
        .name = "Show tasks",
        .select = menu_open_submenu,
        .help = "Displays the tasks started by Canon and Magic Lantern.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Task list",
                .update = tasks_print,
                .select = tasks_toggle_flags,
                #ifdef CONFIG_VXWORKS
                .help = "Task info: name, priority, stack memory usage.",
                #else
                .help = "Task info: ID, name, priority, wait_id, mem, state.",
                #endif
            },
            MENU_EOL
        }
    },
#endif
#ifdef FEATURE_SHOW_CPU_USAGE
#ifdef CONFIG_TSKMON
    {
        .name = "Show CPU usage",
        .priv = &show_cpu_usage_flag,
        .max = 3,
        .choices = (const char *[]) {"OFF", "Percentage", "Busy tasks (ABS)", "Busy tasks (REL)"},
        .help = "Display total CPU usage (percentage).",
    },
#endif
#endif
#ifdef FEATURE_SHOW_GUI_EVENTS
    {
        .name = "Show GUI evts",
        .priv = &draw_event,
        .max = 2,
        .choices = (const char *[]) {"OFF", "ON", "ON + delay 300ms"},
        .help = "Display GUI events (button codes).",
    },
#endif
#ifdef FEATURE_GUIMODE_TEST
    {
        .name = "Test GUI modes (DANGEROUS!!!)",
        .select = (void(*)(void*,int))run_in_separate_task,
        .priv = guimode_test,
        .help = "Cycle through all GUI modes and take screenshots.",
    },
#endif
#ifdef FEATURE_SHOW_EDMAC_INFO
    {
        .name = "Show EDMAC",
        .select = menu_open_submenu,
        .help = "Useful for finding image buffers.",
        .children =  (struct menu_entry[]) {
            {
                .name = "EDMAC display",
                .priv = &edmac_selection,
                .max = 48,
                .update = edmac_display,
            },
            MENU_EOL
        }
    },
#endif
#ifdef FEATURE_SHOW_FREE_MEMORY
#ifdef CONFIG_VXWORKS
    {
        .name = "Free Memory",
        .update = meminfo_display,
        .icon_type = IT_ALWAYS_ON,
        .help = "Free memory, shared between ML and Canon firmware.",
    },
#else // dryos
    {
        .name = "Free Memory",
        .update = meminfo_display,
        .select = menu_open_submenu,
        .help = "Free memory, shared between ML and Canon firmware.",
        .help2 = "Press SET for detailed info.",
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "malloc",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)1,
                .update = meminfo_display,
                .help = "Free memory available via malloc.",
            },
            {
                .name = "AllocateMemory",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)2,
                .update = meminfo_display,
                .help = "Free memory available via AllocateMemory.",
            },
            {
                .name = "stack space",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)3,
                .update = meminfo_display,
                .help = "Free memory available as stack space for user tasks.",
            },
            {
                .name = "shoot_malloc contig",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)4,
                .update = meminfo_display,
                .help = "Largest contiguous block from shoot memory.",
            },
            {
                .name = "shoot_malloc total",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)5,
                .update = meminfo_display,
                .help = "Largest fragmented block from shoot memory.",
            },
            #if defined(CONFIG_MEMPATCH_CHECK)
            {
                .name = "AUTOEXEC.BIN size",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)6,
                .update = meminfo_display,
                .help = "Memory reserved statically at startup for ML binary.",
            },
            #endif
            MENU_EOL
        },
    },
#endif
#endif
#ifdef FEATURE_SHOW_IMAGE_BUFFERS_INFO
    {
        .name = "Image buffers",
        .update = image_buf_display,
        .icon_type = IT_ALWAYS_ON,
        .help = "Display the image buffer sizes (LiveView and Craw).",
        //.essential = 0,
    },
#endif
#ifdef FEATURE_SHOW_SHUTTER_COUNT
    {
        .name = "Shutter Count",
        .update = shuttercount_display,
        .icon_type = IT_ALWAYS_ON,
        .help = "Number of pics taken + number of LiveView actuations",
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
#endif
#ifdef FEATURE_SHOW_CMOS_TEMPERATURE
    {
        .name = "Internal Temp",
        .update = efictemp_display,
        .icon_type = IT_ALWAYS_ON,
        .help = "EFIC chip temperature (somewhere on the mainboard).",
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
#endif
    #if 0 // CONFIG_5D2
    {
        .name = "Ambient light",
        //~.display = ambient_display,
        .help = "Ambient light from the sensor under LCD, in raw units.",
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
    #endif
#ifdef CONFIG_BATTERY_INFO
    {
        .name = "Battery level",
        .update = batt_display,
        .help = "Battery remaining. Wait for 2%% discharge before reading.",
        .icon_type = IT_ALWAYS_ON,
    },
#endif
#if CONFIG_DEBUGMSG
    {
        .name = "PROP display",
        //~.display = prop_display,
        .select = prop_toggle_k,
        // .select_reverse = prop_toggle_j,
        .select_Q = prop_toggle_i,
        .help = "Raw property display (read-only)",
    },
    {
        .name = "Dump LV Buffers",
        //~.display = lvbuf_display,
        .select = lvbuf_select,
        .help = "Dump .422 files containing LV/HD buf addrs in filenames.",
    },
#endif
};

#ifdef CONFIG_CONFIG_FILE
static struct menu_entry cfg_menus[] = {
{
    .name = "Config file",
    .select = menu_open_submenu,
    .submenu_width = 700,
    .help = "Config auto save, manual save, restore defaults...",
    .children =  (struct menu_entry[]) {
        {
            .name = "Config AutoSave",
            .priv = &config_autosave,
            .max  = 1,
            .select        = config_autosave_toggle,
            .help = "If enabled, ML settings are saved automatically at shutdown."
        },
        {
            .name = "Save config now",
            .select        = save_config,
            .update        = save_config_update,
            .help = "Save ML settings to ML/SETTINGS/MAGIC.CFG ."
        },
        {
            .name = "Restore ML defaults",
            .select        = delete_config,
            .update        = delete_config_update,
            .help  = "This restore ML default settings, by deleting MAGIC.CFG.",
        },

        {
            .name = "Show modified settings",
            .update = show_modified_settings,
            .select = toggle_modified_settings,
            .max = 1,
            .icon_type = IT_ACTION,
            .help =  "Displays the settings that were changed from ML defaults.",
        },

        #ifdef CONFIG_PICOC
        {
            .name = "Export as PicoC script",
            .select = save_config_as_picoc,
            .update = save_config_as_picoc_update,
            .help =  "Export current menu settings to ML/SCRIPTS/PRESETn.C.",
            .help2 = "The preset will appear in Scripts menu. Edit/rename on PC.",
        },
        #endif

        MENU_EOL,
    },
},
};
#endif

#if CONFIG_DEBUGMSG

static void * debug_token;

static void
debug_token_handler(
    void *            token,
    void *            arg1,
    void *            arg2,
    void *            arg3
)
{
    debug_token = token;
    DebugMsg( DM_MAGIC, 3, "token %08x arg=%08x %08x %08x",
        (unsigned) token,
        (unsigned) arg1,
        (unsigned) arg2,
        (unsigned) arg3
    );
}

//~ static int dbg_propn = 0;
#define MAXPROP 30
static unsigned dbg_props[MAXPROP] = {0};
static unsigned dbg_props_len[MAXPROP] = {0};
static unsigned dbg_props_a[MAXPROP] = {0};
static unsigned dbg_props_b[MAXPROP] = {0};
static unsigned dbg_props_c[MAXPROP] = {0};
static unsigned dbg_props_d[MAXPROP] = {0};
static unsigned dbg_props_e[MAXPROP] = {0};
static unsigned dbg_props_f[MAXPROP] = {0};
static void dbg_draw_props(int changed)
{
    dbg_last_changed_propindex = changed;
    int i;
    for (i = 0; i < dbg_propn; i++)
    {
    	int x =  80;
        unsigned property = dbg_props[i];
        unsigned len = dbg_props_len[i];
#ifdef CONFIG_VXWORKS
        uint32_t fnt = FONT_MED;
        unsigned y =  15 + i * font_med.height;
#else
        uint32_t fnt = FONT_SMALL;
        int y =  15 + i * font_small.height;
#endif
        if (i == changed) fnt = FONT(fnt, 5, COLOR_BG);
        char msg[100];
        snprintf(msg, sizeof(msg),
#ifdef CONFIG_VXWORKS
            "%08x %04x: %8lx %8lx %8lx %8lx",
#else
            "%08x %04x: %8lx %8lx %8lx %8lx %8lx %8lx",
#endif
            property,
            len,
            len > 0x00 ? dbg_props_a[i] : 0,
            len > 0x04 ? dbg_props_b[i] : 0,
            len > 0x08 ? dbg_props_c[i] : 0,
            len > 0x0c ? dbg_props_d[i] : 0
            #ifndef CONFIG_VXWORKS
           ,len > 0x10 ? dbg_props_e[i] : 0,
            len > 0x14 ? dbg_props_f[i] : 0
            #endif
        );
        bmp_puts(fnt, &x, &y, msg);
    }
}


static void *
debug_property_handler(
    unsigned        property,
    void *            UNUSED_ATTR( priv ),
    void *            buf,
    unsigned        len
)
{
    const uint32_t * const addr = buf;

    /*console_printf("Prop %08x: %2x: %08x %08x %08x %08x\n",
        property,
        len,
        len > 0x00 ? addr[0] : 0,
        len > 0x04 ? addr[1] : 0,
        len > 0x08 ? addr[2] : 0,
        len > 0x0c ? addr[3] : 0
    );*/

    if( !draw_prop )
        goto ack;

    // maybe the property is already in the array
    int i;
    for (i = 0; i < dbg_propn; i++)
    {
        if (dbg_props[i] == property)
        {
            dbg_props_len[i] = len;
            dbg_props_a[i] = addr[0];
            dbg_props_b[i] = addr[1];
            dbg_props_c[i] = addr[2];
            dbg_props_d[i] = addr[3];
            dbg_props_e[i] = addr[4];
            dbg_props_f[i] = addr[5];
            dbg_draw_props(i);
            goto ack; // return with cleanup
        }
    }
    // new property
    if (dbg_propn >= MAXPROP) dbg_propn = MAXPROP-1; // too much is bad :)
    dbg_props[dbg_propn] = property;
    dbg_props_len[dbg_propn] = len;
    dbg_props_a[dbg_propn] = addr[0];
    dbg_props_b[dbg_propn] = addr[1];
    dbg_props_c[dbg_propn] = addr[2];
    dbg_props_d[dbg_propn] = addr[3];
    dbg_props_e[dbg_propn] = addr[4];
    dbg_props_f[dbg_propn] = addr[5];
    dbg_propn++;
    dbg_draw_props(dbg_propn);

ack:
    return (void*)_prop_cleanup( debug_token, property );
}

#endif

#if defined(CONFIG_500D)
#define num_properties 2048
#elif defined(CONFIG_5DC)
#define num_properties 202
#else
#define num_properties 8192
#endif

void
debug_init( void )
{
#if CONFIG_DEBUGMSG
    draw_prop = 0;
    static unsigned* property_list = 0;
    if (!property_list) property_list = SmallAlloc(num_properties * sizeof(unsigned));
    if (!property_list) return;
    unsigned i, j, k;
    unsigned actual_num_properties = 0;

    unsigned is[] = {0x2, 0x80, 0xe, 0x5, 0x4, 0x1, 0x0};
    for( i=0 ; i<COUNT(is) ; i++ )
    {
        for( j=0 ; j<=0xA ; j++ )
        {
            for( k=0 ; k<0x50 ; k++ )
            {
                unsigned prop = 0
                    | (is[i] << 24)
                    | (j << 16)
                    | (k <<  0);

                property_list[ actual_num_properties++ ] = prop;

                if( actual_num_properties >= num_properties )
                    goto thats_all;
            }
        }
    }

thats_all:
    prop_register_slave(
        property_list,
        actual_num_properties,
        debug_property_handler,
        0,
        0
    );
#endif

}

CONFIG_INT( "debug.timed-dump",        timed_dump, 0 );

//~ CONFIG_INT( "debug.dump_prop", dump_prop, 0 );
//~ CONFIG_INT( "debug.dumpaddr", dump_addr, 0 );
//~ CONFIG_INT( "debug.dumplen", dump_len, 0 );

/*
struct bmp_file_t * logo = (void*) -1;
void load_logo()
{
    if (logo == (void*) -1)
        logo = bmp_load(CARD_DRIVE "ML/DOC/logo.bmp",0);
}
void show_logo()
{
    load_logo();
    if ((int)logo > 0)
    {
        kill_flicker(); msleep(100);
        bmp_draw_scaled_ex(logo, 360 - logo->width/2, 240 - logo->height/2, logo->width, logo->height, 0, 0);
    }
}*/

// initialization done AFTER reading the config file,
// but BEFORE starting ML tasks
void
debug_init_stuff( void )
{
    //~ set_pic_quality(PICQ_RAW);
    config_ok = 1;

    #ifdef CONFIG_WB_WORKAROUND
    if (is_movie_mode()) restore_kelvin_wb();
    #endif

    #ifdef CONFIG_5D3
    card_tests();
    #endif
}

TASK_CREATE( "debug_task", debug_loop_task, 0, 0x1e, 0x2000 );

void config_save_at_shutdown()
{
#ifdef CONFIG_CONFIG_FILE
    static int config_saved = 0;
    if (config_ok && config_autosave && !config_saved)
    {
        config_saved = 1;
        save_config(0, 0);
        msleep(100);
    }
#endif
}

#ifdef FEATURE_INTERMEDIATE_ISO_INTERCEPT_SCROLLWHEEL
    #ifndef FEATURE_EXPO_ISO
    #error This requires FEATURE_EXPO_ISO.
    #endif

int iso_intercept = 1;

void iso_adj(int prev_iso, int sign)
{
    if (sign)
    {
        lens_info.raw_iso = prev_iso;
        iso_intercept = 0;
        iso_toggle(0, sign);
        if (lens_info.iso > 6400) lens_set_rawiso(0);
        iso_intercept = 1;
    }
}

int iso_adj_flag = 0;
int iso_adj_old = 0;
int iso_adj_sign = 0;

void iso_adj_task(void* unused)
{
    TASK_LOOP
    {
        msleep(20);
        if (iso_adj_flag)
        {
            iso_adj_flag = 0;
            iso_adj(iso_adj_old, iso_adj_sign);
            lens_display_set_dirty();
        }
    }
}

TASK_CREATE("iso_adj_task", iso_adj_task, 0, 0x1a, 0);

PROP_HANDLER(PROP_ISO)
{
    static unsigned int prev_iso = 0;
    if (!prev_iso) prev_iso = lens_info.raw_iso;

    if (iso_intercept && ISO_ADJUSTMENT_ACTIVE && lv && lv_disp_mode == 0 && is_movie_mode())
    {
        if ((prev_iso && buf[0] && prev_iso < buf[0]) || // 100 -> 200 => +
            (prev_iso >= 112 && buf[0] == 0)) // 3200+ -> auto => +
        {
            //~ bmp_printf(FONT_LARGE, 50, 50, "[%d] ISO+", k++);
            iso_adj_old = prev_iso;
            iso_adj_sign = 1;
            iso_adj_flag = 1;
        }
        else if ((prev_iso && buf[0] && prev_iso > buf[0]) || // 200 -> 100 => -
            (prev_iso <= 88 && buf[0] == 0)) // 400- -> auto => -
        {
            //~ bmp_printf(FONT_LARGE, 50, 50, "[%d] ISO-", k++);
            iso_adj_old = prev_iso;
            iso_adj_sign = -1;
            iso_adj_flag = 1;
        }
    }
    prev_iso = buf[0];
}

#endif

unsigned GetFileSize(char* filename)
{
    uint32_t size;
    if( FIO_GetFileSize( filename, &size ) != 0 )
        return 0xFFFFFFFF;
    return size;
}

static int ReadFileToBuffer(char* filename, void* buf, int maxsize)
{
    int size = GetFileSize(filename);
    if (!size) return 0;

    FILE* f = FIO_Open(filename, O_RDONLY | O_SYNC);
    if (f == INVALID_PTR) return 0;
    int r = FIO_ReadFile(f, UNCACHEABLE(buf), MIN(size, maxsize));
    FIO_CloseFile(f);
    return r;
}

#ifdef CONFIG_RESTORE_AFTER_FORMAT

static int keep_ml_after_format = 1;

static void HijackFormatDialogBox()
{
    if (MEM(DIALOG_MnCardFormatBegin) == 0) return;
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    if (dialog && MEM(dialog->type) != DLG_SIGNATURE) return;

#if defined(CONFIG_50D)
    #define FORMAT_BTN "[FUNC]"
    #define STR_LOC 6
#elif defined(CONFIG_500D)
    #define FORMAT_BTN "[LV]"
    #define STR_LOC 12      //~ by the others' pattern, should this be 10 not 12?
#elif defined(CONFIG_5D2)
    #define FORMAT_BTN "[PicSty]"
    #define STR_LOC 6
#elif defined(CONFIG_EOSM)
    #define FORMAT_BTN "[Tap Screen]"
    #define STR_LOC 4
#else
    #define FORMAT_BTN "[Q]"
    #define STR_LOC 11
#endif

    if (keep_ml_after_format)
        dialog_set_property_str(dialog, 4, "Format card, keep ML " FORMAT_BTN);
    else
        dialog_set_property_str(dialog, 4, "Format card, remove ML " FORMAT_BTN);
    dialog_redraw(dialog);
}

static void HijackCurrentDialogBox(int string_id, char* msg)
{
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    if (dialog && MEM(dialog->type) != DLG_SIGNATURE) return;
    dialog_set_property_str(dialog, string_id, msg);
    dialog_redraw(dialog);
}

int handle_keep_ml_after_format_toggle()
{
    if (!MENU_MODE) return 1;
    if (MEM(DIALOG_MnCardFormatBegin) == 0) return 1;
    keep_ml_after_format = !keep_ml_after_format;
    fake_simple_button(MLEV_HIJACK_FORMAT_DIALOG_BOX);
    return 0;
}

/**
 * for testing dialogs and string IDs
 */

static void HijackDialogBox()
{
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    if (dialog && MEM(dialog->type) != DLG_SIGNATURE) return;
    int i;
    for (i = 0; i<255; i++) {
            char s[30];
            snprintf(s, sizeof(s), "%d", i);
            dialog_set_property_str(dialog, i, s);
    }
    dialog_redraw(dialog);
}

struct tmp_file {
    char name[50];
    void* buf;
    int size;
    int sig;
};

static struct tmp_file * tmp_files = 0;
static int tmp_file_index = 0;
static void* tmp_buffer = 0;
static void* tmp_buffer_ptr = 0;
#define TMP_MAX_BUF_SIZE 15000000

static int TmpMem_Init()
{
    ASSERT(!tmp_buffer);
    ASSERT(!tmp_files);
    static int retries = 0;
    tmp_file_index = 0;
    if (!tmp_files) tmp_files = SmallAlloc(200 * sizeof(struct tmp_file));
    if (!tmp_files)
    {
        retries++;
        HijackCurrentDialogBox(4,
            retries > 2 ? "Restart your camera (malloc error)." :
                          "Format: malloc error :("
            );
        beep();
        msleep(2000);
        return 0;
    }

    if (!tmp_buffer) tmp_buffer = (void*)shoot_malloc(TMP_MAX_BUF_SIZE);
    if (!tmp_buffer)
    {
        retries++;
        HijackCurrentDialogBox(4,
            retries > 2 ? "Restart your camera (shoot_malloc err)." :
                          "Format: shoot_malloc error, retrying..."
        );
        beep();
        msleep(2000);
        SmallFree(tmp_files); tmp_files = 0;
        return 0;
    }

    retries = 0;
    tmp_buffer_ptr = tmp_buffer;

    return 1;
}

static void TmpMem_Done()
{
    SmallFree(tmp_files); tmp_files = 0;
    shoot_free(tmp_buffer); tmp_buffer = 0;
}

static void TmpMem_UpdateSizeDisplay(int counting)
{
    int size = tmp_buffer_ptr - tmp_buffer;
    int size_mb = size * 10 / 1024 / 1024;

    char msg[100];
    snprintf(msg, sizeof(msg), "Format       (ML size: %s%d.%d MB%s)", counting ? "> " : "", size_mb/10, size_mb%10, counting ? "..." : "");
    HijackCurrentDialogBox(3, msg);
}

static void TmpMem_AddFile(char* filename)
{
    if (!tmp_buffer) return;
    if (!tmp_buffer_ptr) return;

    int filesize = GetFileSize(filename);
    if (filesize == -1) return;
    if (tmp_file_index >= 200) return;
    if (tmp_buffer_ptr + filesize + 10 >= tmp_buffer + TMP_MAX_BUF_SIZE) return;

    ReadFileToBuffer(filename, tmp_buffer_ptr, filesize);
    snprintf(tmp_files[tmp_file_index].name, 50, "%s", filename);
    tmp_files[tmp_file_index].buf = tmp_buffer_ptr;
    tmp_files[tmp_file_index].size = filesize;
    tmp_files[tmp_file_index].sig = compute_signature(tmp_buffer_ptr, filesize/4);
    tmp_file_index++;
    tmp_buffer_ptr += ALIGN32SUP(filesize);

    /* no not update on every file, else it takes too long (90% of time updating display) */
    static int aux = 0;
    if(should_run_polling_action(500, &aux))
    {
        char msg[100];

        snprintf(msg, sizeof(msg), "Reading %s...", filename, tmp_buffer_ptr);
        HijackCurrentDialogBox(4, msg);
        TmpMem_UpdateSizeDisplay(1);
    }
}

static void CopyMLDirectoryToRAM_BeforeFormat(char* dir, int cropmarks_flag)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.mode & 0x10) continue; // is a directory
        if (file.name[0] == '.' || file.name[0] == '_') continue;
        if (cropmarks_flag && !is_valid_cropmark_filename(file.name)) continue;

        int n = strlen(file.name);
        if ((n > 4) && (streq(file.name + n - 4, ".VRM") || streq(file.name + n - 4, ".vrm"))) continue;

        char fn[30];
        snprintf(fn, sizeof(fn), "%s%s", dir, file.name);
        TmpMem_AddFile(fn);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
}

static void CopyMLFilesToRAM_BeforeFormat()
{
    TmpMem_AddFile(CARD_DRIVE "AUTOEXEC.BIN");
    TmpMem_AddFile(CARD_DRIVE "MAGIC.FIR");
    CopyMLDirectoryToRAM_BeforeFormat(CARD_DRIVE "ML/", 0);
    CopyMLDirectoryToRAM_BeforeFormat(CARD_DRIVE "ML/SETTINGS/", 0);
    CopyMLDirectoryToRAM_BeforeFormat(CARD_DRIVE "ML/MODULES/", 0);
    CopyMLDirectoryToRAM_BeforeFormat(CARD_DRIVE "ML/SCRIPTS/", 0);
    CopyMLDirectoryToRAM_BeforeFormat(CARD_DRIVE "ML/DATA/", 0);
    CopyMLDirectoryToRAM_BeforeFormat(CARD_DRIVE "ML/CROPMKS/", 1);
    CopyMLDirectoryToRAM_BeforeFormat(CARD_DRIVE "ML/DOC/", 0);
    CopyMLDirectoryToRAM_BeforeFormat(CARD_DRIVE "ML/LOGS/", 0);
    CopyMLDirectoryToRAM_BeforeFormat(CARD_DRIVE, 0);
    TmpMem_UpdateSizeDisplay(0);
}

// check if autoexec.bin is present on the card
static int check_autoexec()
{
    FILE * f = FIO_Open(CARD_DRIVE "AUTOEXEC.BIN", 0);
    if (f != (void*) -1)
    {
        FIO_CloseFile(f);
        return 1;
    }
    return 0;
}


// check if magic.fir is present on the card
static int check_fir()
{
    FILE * f = FIO_Open(CARD_DRIVE "MAGIC.FIR", 0);
    if (f != (void*) -1)
    {
        FIO_CloseFile(f);
        return 1;
    }
    return 0;
}

static void CopyMLFilesBack_AfterFormat()
{
    int i;
    char msg[100];
    int aux = 0;
    for (i = 0; i < tmp_file_index; i++)
    {
        if(should_run_polling_action(500, &aux))
        {
            snprintf(msg, sizeof(msg), "Restoring %s...", tmp_files[i].name);
            HijackCurrentDialogBox(STR_LOC, msg);
        }
        dump_seg(tmp_files[i].buf, tmp_files[i].size, tmp_files[i].name);
        int sig = compute_signature(tmp_files[i].buf, tmp_files[i].size/4);
        if (sig != tmp_files[i].sig)
        {
            snprintf(msg, sizeof(msg), "Could not restore %s :(", tmp_files[i].name);
            HijackCurrentDialogBox(STR_LOC, msg);
            msleep(2000);
            FIO_RemoveFile(tmp_files[i].name);
            if (i <= 1) return;
            //else: if it copies AUTOEXEC.BIN and fonts, ignore the error, it's safe to run
        }
    }

    /* make sure we don't enable bootflag when there is no autoexec.bin (anymore) */
    if(check_autoexec())
    {
        HijackCurrentDialogBox(STR_LOC, "Writing bootflags...");
        bootflag_write_bootblock();
    }

    HijackCurrentDialogBox(STR_LOC, "Magic Lantern restored :)");
    msleep(1000);
    HijackCurrentDialogBox(STR_LOC, "Format");
}

static void HijackFormatDialogBox_main()
{
    if (!MENU_MODE) return;
    if (MEM(DIALOG_MnCardFormatBegin) == 0) return;
    // at this point, Format dialog box is active

    // make sure we have something to restore :)
    if (!check_autoexec() && !check_fir()) return;

    if (!TmpMem_Init()) return;

    // before user attempts to do something, copy ML files to RAM
    ui_lock(UILOCK_EVERYTHING);
    CopyMLFilesToRAM_BeforeFormat();
    ui_lock(UILOCK_NONE);

    // all files copied, we can change the message in the format box and let the user know what's going on
    fake_simple_button(MLEV_HIJACK_FORMAT_DIALOG_BOX);

    // waiting to exit the format dialog somehow
    while (MEM(DIALOG_MnCardFormatBegin))
        msleep(200);

    // and maybe to finish formatting the card
    while (MEM(DIALOG_MnCardFormatExecute))
        msleep(50);

    // card was formatted (autoexec no longer there) => restore ML
    if (keep_ml_after_format && !check_autoexec())
    {
        ui_lock(UILOCK_EVERYTHING);
        CopyMLFilesBack_AfterFormat();
        ui_lock(UILOCK_NONE);
    }

    TmpMem_Done();
}
#endif

void config_menu_init()
{
    menu_prefs_init();

    #ifdef CONFIG_CONFIG_FILE
    menu_add( "Prefs", cfg_menus, COUNT(cfg_menus) );
    #endif

    #ifdef FEATURE_LV_DISPLAY_PRESETS
    extern struct menu_entry livev_cfg_menus[];
    menu_add( "Prefs", livev_cfg_menus,  1);
    #endif

    crop_factor_menu_init();
    customize_menu_init();
    menu_add( "Debug", debug_menus, COUNT(debug_menus) );

    movie_tweak_menu_init();
}

void spy_event(struct event * event)
{
    if (draw_event)
    {
        static int kev = 0;
        static int y = 250;
        kev++;
        bmp_printf(FONT_MED, 0, y, "Ev%d: p=%8x *o=%8x/%8x/%8x a=%8x\n                                                           ",
            kev,
            event->param,
            event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj)) : 0,
            event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 4)) : 0,
            event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 8)) : 0,
            event->arg);
        y += font_med.height;
        if (y > 350) y = 250;
        if (draw_event == 2) msleep(300);
    }
}

#ifdef CONFIG_5DC
static int halfshutter_pressed;
bool get_halfshutter_pressed() { return halfshutter_pressed; }
#else
bool get_halfshutter_pressed() { return HALFSHUTTER_PRESSED && !dofpreview; }
#endif

static int zoom_in_pressed = 0;
static int zoom_out_pressed = 0;
int get_zoom_out_pressed() { return zoom_out_pressed; }
int joy_center_pressed = 0;

int handle_buttons_being_held(struct event * event)
{
    // keep track of buttons being pressed
    #ifdef CONFIG_5DC
    if (event->param == BGMT_PRESS_HALFSHUTTER) halfshutter_pressed = 1;
    if (event->param == BGMT_UNPRESS_HALFSHUTTER) halfshutter_pressed = 0;
    #endif
    #ifdef BGMT_JOY_CENTER
    if (event->param == BGMT_JOY_CENTER) joy_center_pressed = 1;
    if (event->param == BGMT_UNPRESS_UDLR) joy_center_pressed = 0;
    #endif
    #ifdef BGMT_UNPRESS_ZOOMIN_MAYBE
    if (event->param == BGMT_PRESS_ZOOMIN_MAYBE) {zoom_in_pressed = 1; zoom_out_pressed = 0; }
    if (event->param == BGMT_UNPRESS_ZOOMIN_MAYBE) {zoom_in_pressed = 0; zoom_out_pressed = 0; }
    #endif
    #ifdef BGMT_PRESS_ZOOMOUT_MAYBE
    if (event->param == BGMT_PRESS_ZOOMOUT_MAYBE) { zoom_out_pressed = 1; zoom_in_pressed = 0; }
    if (event->param == BGMT_UNPRESS_ZOOMOUT_MAYBE) { zoom_out_pressed = 0; zoom_in_pressed = 0; }
    #endif

    return 1;
}

void fake_simple_button(int bgmt_code)
{
    if ((uilock & 0xFFFF) && (bgmt_code >= 0)) return; // Canon events may not be safe to send when UI is locked; ML events are (and should be sent)

    if (ml_shutdown_requested) return;
    GUI_Control(bgmt_code, 0, FAKE_BTN, 0);
}

// those functions seem not to be thread safe
// calling them from gui_main_task seems to sync them with other Canon calls properly
int handle_tricky_canon_calls(struct event * event)
{
    // fake ML events are always negative numbers
    if (event->param >= 0) return 1;

    //~ static int k; k++;
    //~ bmp_printf(FONT_LARGE, 50, 50, "[%d] tricky call: %d ", k, event->param); msleep(1000);

    switch (event->param)
    {
        #ifdef CONFIG_RESTORE_AFTER_FORMAT
        case MLEV_HIJACK_FORMAT_DIALOG_BOX:
            HijackFormatDialogBox();
            break;
        #endif
        case MLEV_TURN_ON_DISPLAY:
            if (!DISPLAY_IS_ON) call("TurnOnDisplay");
            break;
        case MLEV_TURN_OFF_DISPLAY:
            if (DISPLAY_IS_ON) call("TurnOffDisplay");
            break;
        /*case MLEV_ChangeHDMIOutputSizeToVGA:
            ChangeHDMIOutputSizeToVGA();
            break;*/
        case MLEV_LCD_SENSOR_START:
            #ifdef CONFIG_LCD_SENSOR
            DispSensorStart();
            #endif
            break;
        case MLEV_REDRAW:
            redraw_do();
            break;
    }
    return 0;
}

void display_on()
{
    fake_simple_button(MLEV_TURN_ON_DISPLAY);
}
void display_off()
{
    fake_simple_button(MLEV_TURN_OFF_DISPLAY);
}


// engio functions may fail and lock the camera
void EngDrvOut(int reg, int value)
{
    if (ml_shutdown_requested) return;
    if (!DISPLAY_IS_ON) return; // these are normally used with display on; otherwise, they may lock-up the camera
    _EngDrvOut(reg, value);
}

#if 0 // moved to module mrc_dump?

/* snprintf(buf,max_len,"%30s : %08x <8 groups of 4 bits 1/0>",header,data,data)*/
static uint32_t dump_data(char* buf, uint32_t max_len, char* header, uint32_t data) {
        if (!buf || !header) return 0;
#define SPACE10 "          "
        //Note: %30s does not work
        uint32_t len1 = snprintf(buf, max_len, SPACE10 SPACE10 SPACE10 " : %08X ", data);
        for (uint32_t i = 0; i <= len1 && header[i]; i++) buf[i] = header[i];
        buf += len1;
        uint32_t len2 = snprintf(buf,max_len-len1,"XXXX,XXXX XXXX,XXXX XXXX,XXXX XXXX,XXXX\n");
    for (int i = MIN(39-1,len2); i >= 0; i--) {
        *(buf+i) = ((data & 0x1) != 0) ? '1' : '0';
        data >>= 1;
        if (((i)%5) == 0) i--;
    }
    return len1 + len2;
}

/* Dumps PSRs and coprocessor 15 to buf*/
static uint32_t dump_cache(char* buf, uint32_t max_len) {
        if (!buf) return 0;
    uint32_t old_int;
        uint32_t data;
    asm __volatile__ (
            "MRS %0, CPSR\n"
            "ORR r1, %0, #0xC0\n" // set I flag to disable IRQ
            "MSR CPSR_c, r1\n"
            : "=r"(data) : : "r1"
        );
    old_int = data & 0xC0; // keep just the I flag
        uint32_t len = 0;
        // 20000013 - Supervisor mode. Thumb mode.
    len += dump_data(buf+len, max_len-len, "CPSR", data);
    asm __volatile__ ("MRS %0, SPSR" : "=r"(data));
    // 00000093 - Supervisor mode. Thumb mode. IRQ disabled.
    len += dump_data(buf+len, max_len-len, "SPSR", data);

#define dump_MRC(op1, cIdx, cIdx2, op2, name) \
                {asm volatile ("MRC p15, "#op1", %0, c"#cIdx", c"#cIdx2", "#op2 : "=r"(data)); \
                len += dump_data(buf+len, max_len-len, #op1":c"#cIdx",c"#cIdx2":"#op2" "name, data);}

        // Cache = I/D Cache
        // TCM = Tightly Coupled Memory (small on-board memory)
        // BIST = Built In Self Test
        // Write Buffer != Cache.
        // Values are read from a 550D
/* General */
        // 41059461 - ARM946. Rev 1. 5TE architecture.
        dump_MRC(0,0,0,0, "ID");
        // 0F112112 - Cache type: 4 way set associative. 8KB I/D Cache. 8 words / line
        dump_MRC(0,0,0,1, "Cache Type");
        // 000C00C0 - I/D TCM preset. 4KB each.
        dump_MRC(0,0,0,2, "TCM Size");
        // 0005107D - I/D TCM Enabled. I/D TCM Load mode Disabled.
        // Load mode: At the same address: Reads from underlying memory. Writes to TCM.
        // [15] Thumb state entry enabled from data loaded in to bit 0 of PC register.
        // [14] Pseudo random cache replacement used.
        // [13] Base address for exception vectors @ 0x00000000
        // [12] ICache enable
        // [7]  Little endian
        // [2]  DCache enable
        // [0]  Protection unit enabled
        dump_MRC(0,1,0,0, "Control");

/* Cache */
        // 00000070 - I/D Cachable bit set for areas 4,5,6
        dump_MRC(0,2,0,0, "DCache Cfg");
        dump_MRC(0,2,0,1, "ICache Cfg");

        // 00000070 - Write buffer enabled for areas 4,5,6
        dump_MRC(0,3,0,0, "Wr Buf Ctl");
        // Write Buffer is a 16 entry buffer (addr + [data chunks])
        // Write back: (Cachable + Write Bufferable)
        // Self modifying code in enabled areas should flush the write buffer
        // Writes mark the cacheline as dirty but do not clean it
        // Cleans use the write buffer
        // Linefills cause the buffer to drain

        // Write only. Read = 00000000
        dump_MRC(0,7,5,0, "IC  Flush");
        dump_MRC(0,7,5,1, "IC1 Flush");
        dump_MRC(0,7,13,1,"IC Preftch");
        dump_MRC(0,7,6,0, "DC  Flush");
        dump_MRC(0,7,6,1, "DC1 Flush");
        dump_MRC(0,7,10,1,"DC  Clean");
        dump_MRC(0,7,14,1,"DC1 C/F");
        dump_MRC(0,7,10,2,"DC1 Clean");
        dump_MRC(0,7,14,2,"DC1 C/F");
        dump_MRC(0,7,10,4,"Drain");
        dump_MRC(0,7,0,4, "Sleep");
        dump_MRC(0,15,8,2,"SleepOld");

        dump_MRC(0,9,0,0, "DC Lock"); // 00000000 - Unused
        dump_MRC(0,9,0,1, "IC Lock"); // 00000000 - Unused

        // 00000000 - I/D cache streaming and linefill enabled
        dump_MRC(0,15,0,0,"Test State");

        // [31:30] Segment. [29:5] Zeros+Idx. [4:2] Word. [1:0] Zeros.
        dump_MRC(3,15,0,0,"C Dbg Idx");
        // [31:5] Tag+Idx. [4] Valid. [3:2] Dirty. [1:0] Set.
        dump_MRC(3,15,1,0,"I TAG");
        dump_MRC(3,15,2,0,"D TAG");
        dump_MRC(3,15,3,0,"I Cache");
        dump_MRC(3,15,4,0,"D Cache");

/* TCM - Tightly Coupled Memory */
        // 40000006 - D TCM located at 40000000 with a size of 4KB (no aliasing)
        dump_MRC(0,9,1,0, "DTCM");
        // 40000000 - I TCM located at 00000000 with a size of 4KB (no aliasing)
        dump_MRC(0,9,1,1, "ITCM");

/* Protection unit */
        // I/D (Privileged + User) Read/Write Access for areas 0 to 6.
        // No access for area 7.
        // Protection check failure results in branch to Data Abort or Prefetch Abort.
        dump_MRC(0,5,0,0, "AccPerm D");  // 00003FFF
        dump_MRC(0,5,0,1, "AccPerm I");  // 00003FFF
        dump_MRC(0,5,0,2, "AccPerm Dx"); // 03333333
        dump_MRC(0,5,0,3, "AccPerm Ix"); // 03333333

/* Memory Areas */
        // Definition of areas 0 to 7. Base address, Size.
        // Areas can overlap. Area 7 has the highest priority. Area 0 lowest.
        dump_MRC(0,6,0,0, "Area 0"); // 0000003F - 00000000 - 4GB
        dump_MRC(0,6,1,0, "Area 1"); // 0000003D - 00000000 - 2GB
        dump_MRC(0,6,2,0, "Area 2"); // E0000039 - E0000000 - 512MB
        dump_MRC(0,6,3,0, "Area 3"); // C0000039 - C0000000 - 512MB
        dump_MRC(0,6,4,0, "Area 4"); // FF00002F - FF000000 - 16MB
        dump_MRC(0,6,5,0, "Area 5"); // 00000039 - 00000000 - 512MB
        dump_MRC(0,6,6,0, "Area 6"); // F780002D - F7800000 - 8MB
        dump_MRC(0,6,7,0, "Area 7"); // 00000000 - Disabled

/*BIST - Built In Self Test */
        // 00100010 - BIST complete. (Invalid) size of 0.
        dump_MRC(0,15,0,1,"TAG B Ctl");
        // 00000000 - No BIST. (Invalid) size of 0.
        dump_MRC(1,15,1,1,"TCM B Ctl");
        // 00000000 - Cache RAM(CRM). No BIST. (Invalid) size of 0.
        dump_MRC(2,15,1,1,"CRM B Ctl");
        // (R)ead and (W)rite to control BIST operation.
        // Operation depends on BIST Pause. 0 or 1.
        // Address register:
        //   R0+1: Fail addr. W0: Start addr. W1: peek/poke addr.
        // General register:
        //   R0: Fail data. R1: Peek data. W0: Seed data. W1: Poke data.
        dump_MRC(0,15,0,2,"ITAG B Add"); // 00000000
        dump_MRC(0,15,0,3,"ITAG B Gen"); // 00000000
        dump_MRC(0,15,0,6,"DTAG B Add"); // 00000000
        dump_MRC(0,15,0,7,"DTAG B Gen"); // 00000000
        dump_MRC(1,15,0,2,"ITCM B Add"); // 00000000
        dump_MRC(1,15,0,3,"ITCM B Gen"); // 00000000
        dump_MRC(1,15,0,6,"DTCM B Add"); // 00000000
        dump_MRC(1,15,0,7,"DTCM B Gen"); // 00000000
        dump_MRC(2,15,0,2,"ICRM B Add"); // 00000000
        dump_MRC(2,15,0,3,"ICRM B Gen"); // 00000000
        dump_MRC(2,15,0,6,"DCRM B Add"); // 00000000
        dump_MRC(2,15,0,7,"DCRM B Gen"); // 00000000

/* Misc */
        // 00000000 - Process ID - Unused
        dump_MRC(0,13,0,1,"PID");
        dump_MRC(0,13,1,1,"PID Old"); // alias

        // 00000000 - nFIQ and nIRQ are not masked by a hardware trace.
        dump_MRC(1,15,1,0,"Trace Ctrl");

/* Debug communication channel - coprocessor 14*/
/*#undef dump_MR
#define dump_MRC(cIdx, name) \
                {asm volatile ("MRC p14, 0, %0, c"#cIdx", c0" : "=r"(data)); \
                len += dump_data(buf+len, max_len-len, "c"#cIdx" "name, data);}
        // These cause a lock on my 550D
        dump_MRC(0,"Dbg C Status");
        dump_MRC(1,"Dbg C Read");
        dump_MRC(2,"Dbg C Write"); // write only...
        dump_MRC(3,"Dbg Status"); // bit 4 = debug from Thumb ? */

#undef dump_MRC
    sei(old_int);
        return len;
}
#endif
