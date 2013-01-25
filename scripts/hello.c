printf("Hello from PicoC!\n");

sleep(2);

int n = 2;
for (int i = 0; i < 2; i++)
{
    printf("Taking pic %d of %d...\n", i+1, n);
    sleep(1);
    takepic();
}

printf("Done :)\n");
