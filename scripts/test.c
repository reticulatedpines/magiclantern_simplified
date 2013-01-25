printf("PicoC testing...\n");

/* Some basic functions */
beep();
msleep(2000);

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
msleep(2000);
console_hide();
click(MENU);
msleep(1000);
click(RIGHT);
msleep(1000);
click(MENU);
msleep(1000);
console_show();

/* Movie recording test */
msleep(1000);
printf("Recording... ");
movie_start();
msleep(3000);
movie_end();
printf("done.\n");
msleep(1000);

/* Picture taking test */
int n = 2;
for (int i = 0; i < n; i++)
{
    printf("Taking pic %d of %d...\n", i+1, n);
    msleep(1000);
    takepic();
}

printf("Taking bulb picture...\n");
msleep(1000);
bulbpic(2500);

printf("Done :)\n");
