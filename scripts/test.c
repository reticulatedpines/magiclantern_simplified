printf("PicoC testing...\n");

/* Some basic functions */
beep();
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

/* Dumping memory */
for (unsigned a = 0xFF010000; a < 0xFF010010; a+=4)
    printf("%x: %x\n", a, *(int*)a);

/* Key press test */
sleep(2);
console_hide();
click(MENU);
sleep(1);
click(RIGHT);
sleep(1);
click(MENU);
sleep(1);
console_show();

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
printf("White balance: %d K, %d G/M\n", get_kelvin(), get_green());
set_green(0);
printf("Kelvin from 2000 to 10000...\n");
sleep(1);
console_hide();
sleep(2);
for (int k = 2000; k < 10000; k += 50)
{
    set_kelvin(k);
    sleep(0.01);
}
sleep(1);
console_show();


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

/* Movie recording test */
sleep(1);
printf("Recording... ");
movie_start();
sleep(3);
movie_end();
printf("done.\n");
sleep(1);

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

printf("Done :)\n");
