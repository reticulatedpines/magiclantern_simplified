/*
@title Hello, World!
@param n Pics to take
@range n 0 5
*/

printf("Hello from PicoC!\n");

sleep(2);

for (int i = 0; i < n; i++)
{
    printf("Taking pic %d of %d...\n", i+1, n);
    sleep(1);
    takepic();
}

printf("Done :)\n");
