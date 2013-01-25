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
