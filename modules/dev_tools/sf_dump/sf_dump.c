/* Dumper for Serial Flash, indended for 100D firmwares. / nkls 2015 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <console.h>

#define BUF_SIZE    64*1024
#define OUT_FILE    "ML/LOGS/SFDATA.BIN"

static int (*SF_CreateSerial)() = NULL;
static int (*SF_readSerialFlash)(int src, void* dest, int size) = NULL;
static int (*SF_Destroy)() = NULL;

/* optional; dumping more will just repeat the contents */
static int SF_flash_size = 0x1000000;

/* crash breadcrumb: rewrite a marker file after each step so a crash leaves the
 * last-reached step on the card (ML/LOGS/SFSTEP.TXT) */
static void sfmark(const char* m)
{
    FILE* g = FIO_CreateFile("ML/LOGS/SFSTEP.TXT");
    if (g) { FIO_WriteFile(g, (void*)m, strlen(m)); FIO_CloseFile(g); }
    msleep(50);
}

static void sf_dump_task()
{
    gui_stop_menu();
    msleep(1000);
    console_show();

    sfmark("1: task started\n");

    uint8_t* buffer = 0;
    FILE* f = 0;

    buffer = fio_malloc(BUF_SIZE);
    if (!buffer) { sfmark("X: fio_malloc failed\n"); goto cleanup; }
    f = FIO_CreateFile(OUT_FILE);
    if (!f) { sfmark("X: create OUT_FILE failed\n"); goto cleanup; }

    sfmark("2: about to SF_CreateSerial\n");
    printf("Opening serial flash...\n");
    SF_CreateSerial();
    sfmark("3: SF_CreateSerial returned\n");

    printf("Dumping serial flash...     ");
    sfmark("4: about to first SF_readSerialFlash\n");
    SF_readSerialFlash(0, buffer, BUF_SIZE);
    sfmark("5: first read returned, writing rest\n");
    FIO_WriteFile(f, buffer, BUF_SIZE);

    for (int i = BUF_SIZE; i < SF_flash_size; i += BUF_SIZE) {
        SF_readSerialFlash(i, buffer, BUF_SIZE);
        FIO_WriteFile(f, buffer, BUF_SIZE);
        printf("\b\b\b\b%3d%%", (i + BUF_SIZE) * 100 / SF_flash_size);
    }

    sfmark("6: dump loop done, about to SF_Destroy\n");
    printf("\nClosing serial flash...\n");
    SF_Destroy();
    sfmark("7: SF_Destroy returned - DONE\n");

    printf("Done!\n");

cleanup:
    if (f) FIO_CloseFile(f);
    if (buffer) free(buffer);
}

static struct menu_entry sf_dump_menu[] =
{
    {
        .name   = "Dump serial flash",
        .select = run_in_separate_task,
        .priv   = sf_dump_task,
        .icon_type = IT_ACTION,
    }
};

static unsigned int sf_dump_init()
{
    if (is_camera("5D3", "1.1.3"))
    {
        /* not working */
        SF_CreateSerial     = (void*) 0xFF302BAC;
        SF_readSerialFlash  = (void*) 0xFF302B54;
        SF_Destroy          = (void*) 0xFF305488;
    }

    if (is_camera("100D", "1.0.1"))
    {
        /* 1.0.0 4.3.7 60(23): 0xFF144064, 0xFF14400C, 0xFF146A54 */
        SF_CreateSerial     = (void*) 0xFF143D7C;
        SF_readSerialFlash  = (void*) 0xFF143D24;
        SF_Destroy          = (void*) 0xFF14676C;
        SF_flash_size       = 0x1000000;
    }

    if (is_camera("650D", "1.0.4"))
    {
        SF_CreateSerial     = (void*) 0xFF1389C0;
        SF_readSerialFlash  = (void*) 0xFF138968;
        SF_Destroy          = (void*) 0xFF13B370;
        SF_flash_size       = 0x8000000;
    }

    if (is_camera("700D", "1.1.5"))
    {
        SF_CreateSerial     = (void*) 0xFF139578;
        SF_readSerialFlash  = (void*) 0xFF139520;
        SF_Destroy          = (void*) 0xFF13BF28;
        SF_flash_size       = 0x800000;
    }

    if (is_camera("EOSM", "2.0.2"))
    {
        SF_CreateSerial     = (void*) 0xFF1385AC;
        SF_readSerialFlash  = (void*) 0xFF138554;
        SF_Destroy          = (void*) 0xFF13AF5C;
        SF_flash_size       = 0x800000;
    }

    if (is_camera("6D", "1.1.6"))
    {
        SF_CreateSerial     = (void*) 0xFF1471CC;
        SF_readSerialFlash  = (void*) 0xFF147174;
        SF_Destroy          = (void*) 0xFF149BB8;
        SF_flash_size       = 0x800000;
    }

    if (is_camera("70D", "1.1.2"))
    {
        SF_CreateSerial     = (void*) 0xFF144D2C;
        SF_readSerialFlash  = (void*) 0xFF144CD4;
        SF_Destroy          = (void*) 0xFF14771C;
    }

    if (is_camera("R", "1.8.0"))
    {
        /* RE'd from ROM0 7.3.9 via Ghidra; Thumb fns -> bit0 set (|1) */
        SF_CreateSerial     = (void*) 0xE03C002B;  /* FUN_e03c002a "[SF] CreateSerial" (works) */
        SF_readSerialFlash  = (void*) 0xE03C066B;  /* FUN_e03c066a readSerialFlashWithQuad (src,dest,size) - handles large reads; plain 0xE03C04EE only does <0x200 */
        SF_Destroy          = (void*) 0xE03C1EE5;  /* FUN_e03c1ee4 teardown */
        SF_flash_size       = 0x1000000;           /* 16MB */
    }

    if (!SF_CreateSerial || !SF_readSerialFlash || !SF_Destroy)
    {
        console_show();
        printf("Serial flash stubs not set for your camera.\n");
        return CBR_RET_ERROR;
    }
    
    menu_add("Debug", sf_dump_menu, COUNT(sf_dump_menu));
    return 0;
}

static unsigned int sf_dump_deinit()
{
    menu_remove("Debug", sf_dump_menu, COUNT(sf_dump_menu));
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(sf_dump_init)
    MODULE_DEINIT(sf_dump_deinit)
MODULE_INFO_END()

