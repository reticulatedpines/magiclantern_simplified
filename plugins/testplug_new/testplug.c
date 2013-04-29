


int testplug_init()
{
    msleep(500);
    printf("TEST: Hiding Console\n");
    msleep(1000);
    console_hide();
    msleep(1000);
    console_show();
    printf("TEST: Finished\n");
    
    return 42;
}
