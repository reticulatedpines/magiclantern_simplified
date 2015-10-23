/* Dumper for Serial Flash, indended for 100D firmwares. / nkls 2015 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>


// Dummy function for testing
void dummy_call(void) { msleep(1); }
#define DUMMY_CALL ((uint32_t)dummy_call)


// Lookup table for functions
const struct serial_functions {
    const struct device_desc {
        const char *model, *firmware, *subv;
    } device;
    const unsigned int flash_size; // Must be multiple of BUF_SIZE
    const uint32_t create;
    const uint32_t destroy;
    const uint32_t read;
} serial_functions[] = {
/*   Model and firmware               Flash size  SF_CreateSerial  SF_DestroySerial  SF_Read     */
//  {{"100D","1.0.0","4.3.7 60(23)"}, 0x1000000,  DUMMY_CALL,      DUMMY_CALL,       DUMMY_CALL},
    {{"100D","1.0.0","4.3.7 60(23)"}, 0x1000000,  0xFF144064,      0xFF146A54,       0xFF14400C}
};

#define BUF_SIZE    1024
#define NUM_MODELS (int)(sizeof(serial_functions) / sizeof(struct serial_functions))
#define OUT_FILE   "ML/SFDATA.bin"


void sf_dump_task()
{
    int y = 1;
    int next_update = -1;

    // Local variables
    int32_t i, size;

    // Function addresses
    void (*SF_CreateSerial)(void) = NULL;
    void (*SF_DestroySerial)(void) = NULL;
    void (*SF_Read)(uint32_t src, void * dest, size_t size) = NULL;


    // Prepare drawing (copied from other module)
    msleep(1000);
    canon_gui_disable_front_buffer();
    clrscr();
    bmp_printf(FONT_MED, 0, font_med.height * y++, "Starting SF_DUMP!");

    // Find addresses in lookup table
    for (i = 0; i < NUM_MODELS; i++) {
        const struct serial_functions * sfs = &serial_functions[i];
        if (is_camera(sfs->device.model, sfs->device.firmware) && is_subversion(sfs->device.subv)) {
            SF_CreateSerial  = (void(*)(void))                 sfs->create;
            SF_DestroySerial = (void(*)(void))                 sfs->destroy;
            SF_Read          = (void(*)(uint32_t,void*,size_t))sfs->read;
            size = sfs->flash_size;
            break;
        }
    }

    // Did we find addresses to use?
    if (i == NUM_MODELS) {
        // Not found!
        bmp_printf(FONT_MED, 0, font_med.height * y++, "This won't work, I have no function addresses for this model");
        msleep(3000);
        return;
    }

    bmp_printf(FONT_MED, 0, font_med.height * y++, "Dumping to file: %s", OUT_FILE);
    msleep(3000);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "Dumping serial flash now...");

    // This is where the magic happens
    {
        uint8_t buffer[BUF_SIZE];
        FILE * f = FIO_CreateFile(OUT_FILE);

        SF_CreateSerial();
        for (i = 0; i < size; i += BUF_SIZE) {
            int p;
            SF_Read(i, buffer, BUF_SIZE);
            FIO_WriteFile(f, buffer, BUF_SIZE);

            if (i > next_update) {
                p = ( (i * 100) / size ); // Current percentage
                next_update = (p + 1)*size / 100;
                bmp_printf(FONT_MED, 0, font_med.height * y, "%d%%   ", p);
            }
        }
        SF_DestroySerial();

        FIO_CloseFile(f);
    }

    bmp_printf(FONT_MED, 0, font_med.height * y++, "Done!");
    
    // Copied from other module...
    call("dispcheck");
    msleep(3000);
    canon_gui_enable_front_buffer(0);
}

static MENU_UPDATE_FUNC(sf_dump_update)
{
}

static MENU_SELECT_FUNC(sf_dump_select)
{
    gui_stop_menu();
    task_create("sf_dump_task", 0x1e, 0x1000, sf_dump_task, (void*)0);
}

static struct menu_entry sf_dump_menu[] =
{
    {
        .name = "Dump serial flash",
        .update = &sf_dump_update,
        .select = &sf_dump_select,
        .priv = NULL,
        .icon_type = IT_ACTION,
    }
};

unsigned int sf_dump_init()
{
    menu_add("Debug", sf_dump_menu, COUNT(sf_dump_menu));
    return 0;
}

unsigned int sf_dump_deinit()
{
    menu_remove("Debug", sf_dump_menu, COUNT(sf_dump_menu));
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(sf_dump_init)
    MODULE_DEINIT(sf_dump_deinit)
MODULE_INFO_END()

