static void busy_wait(int n)
{
    int i,j;
    static volatile int k = 0;
    for (i = 0; i < n; i++)
       for (j = 0; j < 100000; j++)
              k++;
}

static void blink(int n)
{
    while (1)
    {
        #if defined(CARD_LED_ADDRESS) && defined(LEDON) && defined(LEDOFF)
        MEM(CARD_LED_ADDRESS) = LEDON;
        busy_wait(n);
        MEM(CARD_LED_ADDRESS) = LEDOFF;
        busy_wait(n);
        #endif
    }
}
