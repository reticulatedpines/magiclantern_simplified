printf("Hello from PicoC!\n");

msleep(2000);

int n = 2;
for (int i = 0; i < 2; i++)
{
    printf("Taking pic %d of %d...\n", i+1, n);
    msleep(1000);
    takepic();
}

printf("Done :)\n");
