#include <dryos.h>
#include <bmp.h>
#include <menu.h>
#include <lens.h>
#include <module.h>

static void find_response_curve(const char* fname)
{
    char fname_real[255];
    snprintf(fname_real, sizeof(fname_real), "ML/LOGS/%s", fname);
    FILE* f = FIO_CreateFileEx(fname_real);

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

static void find_response_curve_ex(const char* fname, int iso, int dgain, int htp)
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
    int digic_iso_gain = is_movie_mode() ? get_digic_iso_gain_movie() : get_digic_iso_gain_photo();

    snprintf(name, sizeof(name), "ML/LOGS/i%d%s%s.txt",
        raw2iso(lens_info.iso_equiv_raw),
        digic_iso_gain <= 256 ? "e2" : digic_iso_gain != 1024 ? "e" : "",
        get_htp() ? "h" : "");

    find_response_curve(name);
}

static void iso_response_curve_160()
{
    msleep(2000);

    // ISO 100x/160x/80x series

    find_response_curve_ex("iso160e.txt",    200,   790  , 0);
    find_response_curve_ex("iso320e.txt",    400,   790  , 0);
    find_response_curve_ex("iso640e.txt",    800,   790  , 0);
    find_response_curve_ex("iso1250e.txt",   1600,  790  , 0);
    find_response_curve_ex("iso2500e.txt",   3200,  790  , 0);

    find_response_curve_ex("iso160.txt",    160,     0   , 0);
    find_response_curve_ex("iso320.txt",    320,     0   , 0);
    find_response_curve_ex("iso640.txt",    640,     0   , 0);
    find_response_curve_ex("iso1250.txt",   1250,    0   , 0);
    find_response_curve_ex("iso2500.txt",   2500,    0   , 0);

    find_response_curve_ex("iso100.txt",    100,     0   , 0);
    find_response_curve_ex("iso200.txt",    200,     0   , 0);
    find_response_curve_ex("iso400.txt",    400,     0   , 0);
    find_response_curve_ex("iso800.txt",    800,     0   , 0);
    find_response_curve_ex("iso1600.txt",   1600,    0   , 0);
    find_response_curve_ex("iso3200.txt",   3200,    0   , 0);
}

static void iso_response_curve_logain()
{
    msleep(2000);
    find_response_curve_ex("iso70e.txt",     100,   724   , 0);
    find_response_curve_ex("iso140e.txt",    200,   724   , 0);
    find_response_curve_ex("iso280e.txt",    400,   724   , 0);
    find_response_curve_ex("iso560e.txt",    800,   724   , 0);
    find_response_curve_ex("iso1100e.txt",   1600,  724   , 0);
    find_response_curve_ex("iso2200e.txt",   3200,  724   , 0);

    find_response_curve_ex("iso65e.txt",     100,   664   , 0);
    find_response_curve_ex("iso130e.txt",    200,   664   , 0);
    find_response_curve_ex("iso260e.txt",    400,   664   , 0);
    find_response_curve_ex("iso520e.txt",    800,   664   , 0);
    find_response_curve_ex("iso1000e.txt",   1600,  664   , 0);
    find_response_curve_ex("iso2000e.txt",   3200,  664   , 0);

    find_response_curve_ex("iso50e.txt",     100,   512   , 0);
    find_response_curve_ex("iso100e.txt",    200,   512   , 0);
    find_response_curve_ex("iso200e.txt",    400,   512   , 0);
    find_response_curve_ex("iso400e.txt",    800,   512   , 0);
    find_response_curve_ex("iso800e.txt",    1600,  512   , 0);
    find_response_curve_ex("iso1600e.txt",   3200,  512   , 0);
}

static void iso_response_curve_htp()
{
    msleep(2000);
    find_response_curve_ex("iso200h.txt",      200,   0   , 1);
    find_response_curve_ex("iso400h.txt",      400,   0   , 1);
    find_response_curve_ex("iso800h.txt",      800,   0   , 1);
    find_response_curve_ex("iso1600h.txt",    1600,   0   , 1);
    find_response_curve_ex("iso3200h.txt",    3200,   0   , 1);
    find_response_curve_ex("iso6400h.txt",    6400,   0   , 1);

    find_response_curve_ex("iso140eh.txt",      200,   724   , 1);
    find_response_curve_ex("iso280eh.txt",      400,   724   , 1);
    find_response_curve_ex("iso560eh.txt",      800,   724   , 1);
    find_response_curve_ex("iso1100eh.txt",     1600,   724   , 1);
    find_response_curve_ex("iso2200eh.txt",     3200,   724   , 1);
    find_response_curve_ex("iso4500eh.txt",     6400,   724   , 1);

    find_response_curve_ex("iso100eh.txt",      200,   512   , 1);
    find_response_curve_ex("iso200eh.txt",      400,   512   , 1);
    find_response_curve_ex("iso400eh.txt",      800,   512   , 1);
    find_response_curve_ex("iso800eh.txt",     1600,   512   , 1);
    find_response_curve_ex("iso1600eh.txt",     3200,   512   , 1);
    find_response_curve_ex("iso3200eh.txt",     6400,   512   , 1);
}

static void iso_movie_change_setting(int iso, int dgain, int shutter)
{
    lens_set_rawiso(iso);
    lens_set_rawshutter(shutter);
    set_display_gain_equiv(dgain);
    msleep(2000);
    take_fast_pictures(1);
}

static void iso_movie_test()
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

static struct menu_entry iso_test_menus[] = {
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
};


static unsigned int iso_tests_init() {
    menu_add( "Debug", iso_test_menus, COUNT(iso_test_menus) );
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(iso_tests_init)
MODULE_INFO_END()
