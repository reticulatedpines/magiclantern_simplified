/*
@title PicoC testing script
@param a Misc small tests
@range a 0 1
@param b Expo settings tests
@range b 0 1
@param c Powersave tests
@range c 0 1
@param d Movie recording tests
@range d 0 1
@param e Picture taking tests
@range e 0 1
@param f Focus tests
@range f 0 1
@param g Menu interaction tests
@range g 0 1
@param h Graphics tests
@range h 0 1
*/

printf("PicoC testing...\n");

if (a) // misc tests
{
    /* Some basic functions */
    beep();
    printf("get_model()    '%s'\n", get_model());
    printf("get_firmware() '%s'\n", get_firmware());
    sleep(2);

    printf("LiveView:%d Recording:%d\n", lv, recording);

    /* Date/time  */
    struct tm * t = get_time();
    printf("Time: %02d:%02d:%02d\n", t->hour, t->minute, t->second);
    printf("Date: %02d/%02d/%02d\n", t->year, t->month, t->day);

    /* Some math */
    float pi = 3.14;
    float twopi = 2 * pi;
    printf("2*pi = %f\n", twopi);
    printf("some float values: %f %f %f %f %f %f %f %f %f %f %f %f \n", 0.0000001, 0.001, 0.123, 10000, 200000, 300000000, 5433546.45262432, 5.450267432, 0, 42, 555555.5555555, 1.0/0.0 );
    
    /* LED blinking */
    printf("LED blinking...\n");
    for (int k = 0; k < 5; k++)
    {
        set_led(1,1);
        sleep(0.2);
        set_led(1,0);
        sleep(0.2);
    }

    /* Dumping memory */
    for (unsigned addr = 0xFF010000; addr < 0xFF010010; addr += 4)
        printf("%x: %x\n", addr, *(int*)addr);
    
    /* Key press test */
    sleep(2);
    console_hide();
    notify_box(3, "Walking through Canon menu...");
    click(MENU);
    sleep(1);
    click(RIGHT);
    sleep(1);
    click(MENU);
    sleep(1);
    console_show();
    
    printf("Press any key to continue.\n");
    wait_key();
}

if (b) // expo tests
{
    /* Exposure settings test (read/write) */
    printf("ISO %d 1/%f f/%f\n", get_iso(), 1/get_shutter(), get_aperture());
    printf("Sv%f Tv%f Av%f\n", get_sv(), get_tv(), get_av());
    printf("Raw ISO %d shutter %d aperture %d\n \n", get_rawiso(), get_rawshutter(), get_rawaperture());
    sleep(2);

    printf("setting ISO 200 1/2000 f/2.8...\n");
    set_iso(200);
    set_shutter(1./2000);
    set_aperture(2.8);
    printf(" => got ISO %d 1/%f f/%f\n\n", get_iso(), 1/get_shutter(), get_aperture());
    sleep(2);

    printf("setting ISO 400 1/128 f/11 (APEX)...\n");
    set_tv(7);
    set_sv(7);
    set_av(7);
    printf(" => got ISO %d 1/%f f/%f\n\n", get_iso(), 1/get_shutter(), get_aperture());
    sleep(2);

    printf("setting ISO 200 1/32 f/8 (raw units)...\n");
    set_rawiso(80);
    set_rawshutter(96);
    set_rawaperture(56);
    printf(" => got ISO %d 1/%f f/%f\n\n", get_iso(), 1/get_shutter(), get_aperture());
    sleep(2);

    /* AE tests */
    printf("AE: %d; flash AE: %d\n", get_ae(), get_flash_ae());
    set_ae(1);
    set_flash_ae(-2);

    /* Let's go into LiveView */
    if (!lv) { click(LV); sleep(1); }

    /* Kelvin tests */
    int kelvin_0 = get_kelvin();
    printf("White balance: %d K, %d G/M\n", get_kelvin(), get_green());
    set_green(0);
    printf("Kelvin from 2000 to 10000...\n");
    sleep(1);
    console_hide();
    sleep(2);
    for (int K = 2000; K < 10000; K += 50)
    {
        set_kelvin(K);
        sleep(0.01);
    }
    sleep(1);
    set_kelvin(kelvin_0);
    console_show();
    
    printf("Press any key to continue.\n");
    wait_key();
}

if (c) // powersave tests
{
    /* Let's go into LiveView */
    if (!lv) { click(LV); sleep(1); }

    /* Powersave tests */
    printf("Display off... ");
    sleep(1);
    display_off();
    sleep(2);
    display_on();
    sleep(2);
    printf("and back on.\n");

    sleep(1);

    printf("LiveView paused... ");
    sleep(1);
    lv_pause();
    sleep(2);
    lv_resume();
    sleep(2);
    printf("and resumed.\n");

    console_show();
    printf("Press any key to continue.\n");
    wait_key();
}

if (d) // movie test
{
    /* Movie recording test */
    sleep(1);
    printf("Recording... ");
    movie_start();
    sleep(3);
    movie_end();
    printf("done.\n");
    sleep(1);

    console_show();
    printf("Press any key to continue.\n");
    wait_key();
}

if (e) // photo tests
{
    /* Picture taking test */
    int n = 2;
    for (int i = 0; i < n; i++)
    {
        printf("Taking pic %d of %d...\n", i+1, n);
        sleep(1);
        takepic();
    }

    printf("Taking bulb picture...\n");
    sleep(1);
    bulbpic(2.5);

    console_show();
    printf("Press any key to continue.\n");
    wait_key();
}


if (f) // focus tests
{
    /* Lens focusing test */

    /* We need LiveView */
    if (!lv) { click(LV); sleep(1); }

    struct dof * D = get_dof();
    printf("Lens name      : %s\n",    D->lens_name);
    printf("Aperture       : f/%f\n",  get_aperture());
    printf("Focal length   : %d mm\n", D->focal_len);
    printf("Focus distance : %d mm\n", D->focus_dist);
    printf("DOF near       : %d mm\n", D->near);
    printf("DOF far        : %d mm\n", D->far);
    printf("Hyperfocal     : %d mm\n", D->hyperfocal);

    printf("\n");
    
    printf("AFMA           : %d\n", get_afma(-1));

    printf("\n");
        
    focus_setup(3, 200, 1); // step size 3, delay 200ms, wait
    
    printf("5 steps forward...\n");
    focus(5);

    printf("and 5 steps backward...\n");
    focus(-5);

    printf("Done.\n");
    console_show();

    sleep(1);
    printf("Press any key to continue.\n");
    wait_key();
}

if (g)
{
    /* ML menu test */
    menu_set("Shoot", "Advanced Bracket", 1);
    menu_set("Expo", "ISO", 320);
    menu_set_str("Expo", "PictureStyle", "Faithful");
    
    // for debugging
    //~ menu_set("Expo", "WhiteBalance", 3500);             // would take many iterations, but works
    //~ menu_set_str("Debug", "Don't click me!", "foo");    // should try only once and fail gracefully
    
    printf("Bracketing   : %d\n", menu_get("Shoot", "Advanced Bracket"));
    printf("Expo.Lock    : %s\n", menu_get_str("Expo", "Expo.Lock"));
    printf("ISO          : %s\n", menu_get_str("Expo", "ISO"));
    printf("PictureStyle : %s\n", menu_get_str("Expo", "PictureStyle"));
    
    sleep(2);

    menu_open();
    menu_select("Shoot", "Intervalometer");
    sleep(2);
    menu_select("Expo", "ISO");
    sleep(2);
    menu_select("Focus", "Follow Focus");
    sleep(2);
    menu_close();

    menu_set("Shoot", "Advanced Bracket", 0);

    printf("Press any key to continue.\n");
    wait_key();
}

if (h)
{
    console_hide();
    display_on();
    
    // disable GlobalDraw so it doesn't interfere with our graphics
    int old_gdr = menu_get("Overlay", "Global Draw");
    menu_set("Overlay", "Global Draw", 0);

    // also, prevent Canon code from drawing on the screen
    set_canon_gui(0);
    
    clrscr();
    
    fill_rect(50, 50, 720-100, 480-100, COLOR_GRAY(30));
    draw_rect(50, 50, 720-100, 480-100, COLOR_YELLOW);
    
    bmp_printf(FONT_LARGE, 100, 100, "Hi there!");
    bmp_printf(SHADOW_FONT(FONT_LARGE), 400, 100, "Hi there!");
    bmp_printf(FONT(FONT_LARGE, COLOR_YELLOW, COLOR_BLACK), 300, 200, "Hi there!");
    
    sleep(1);
    bmp_printf(FONT_MED, 0, 0, "Press any key to continue.\n");
    wait_key();
    
    // restore things back to their original state
    menu_set("Overlay", "Global Draw", old_gdr);
    set_canon_gui(1);
}


printf("Done :)\n");

