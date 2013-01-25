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
