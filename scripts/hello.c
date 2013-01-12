printf("Hello from PicoC!\n");
msleep(3000);

int n = 3;
for (int i = 0; i < n; i++)
{
    printf("Taking pic %d of %d...\n", i+1, n);
    msleep(1000);
    shoot();
}

printf("Done :)\n");
