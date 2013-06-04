#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

#ifdef CONFIG_GDB
#error ssss
#endif


#define MEM(x) (*(volatile unsigned int *)(x))
#define CF_REG_B(x) *(volatile unsigned char *)(0xC0620000 | x)
#define CF_REG_W(x) *(volatile unsigned short *)(0xC0620000 | x)
#define CF_REG_D(x) *(volatile unsigned int *)(0xC0620000 | x)

#define CF_REG_DATA_IN   0x2000
#define CF_REG_FEATURE   0x2001
#define CF_REG_SEC_COUNT 0x2002
#define CF_REG_SEC_NUM   0x2003
#define CF_REG_CYL_HI    0x2004
#define CF_REG_CYL_LO    0x2005
#define CF_REG_CDH       0x2006
#define CF_REG_COMMAND   0x2007

unsigned char *cf_acc_get_string(unsigned char *buf, unsigned short *data, int chars)
{
    for(int pos = 0; pos < chars; pos++)
    {
        unsigned char character = (data[pos/2] >> (8*(1-(pos&1)))) & 0xFF;
        buf[pos] = character;
    }
    buf[chars] = 0;
    
    return buf;
}

void cf_acc_read_task()
{
    int y = 1;
    unsigned char str_buf[64];
    unsigned short data_buf[256];
    msleep(1000);
    
    canon_gui_disable_front_buffer();
    clrscr();
    
    /* disable card interrupt. not sure if needed. */
    unsigned char control = CF_REG_B(0x200E);
    CF_REG_B(0x200E) = (control & ~2);
    
    /* disable controller interrupts for this command */
    CF_REG_D(0x8044) = 0;
    CF_REG_D(0x8040) = 0;
    
    /* issue IDENTIFY_DRIVE command */
    CF_REG_B(CF_REG_CDH) = 0;
    CF_REG_B(CF_REG_COMMAND) = 0xEC;
    
    /* wait until card is ready again */
    while((CF_REG_B(CF_REG_COMMAND) ^ 0x40) & 0xC0)
    {
        msleep(20);
    }
    
    /* read received data */
    for(int word_num = 0; word_num < 256; word_num++)
    {
        data_buf[word_num] = CF_REG_W(CF_REG_DATA_IN);
    }
    
    /* reenable card interrupts */
    CF_REG_B(0x200E) = control;
    
    bmp_printf(FONT_MED, 0, font_med.height * y++, "Model:     %s", cf_acc_get_string(str_buf, &data_buf[27], 40));
    bmp_printf(FONT_MED, 0, font_med.height * y++, "Firmware:  %s", cf_acc_get_string(str_buf, &data_buf[23], 8));
    bmp_printf(FONT_MED, 0, font_med.height * y++, "Serial:    %s", cf_acc_get_string(str_buf, &data_buf[10], 20));
    bmp_printf(FONT_MED, 0, font_med.height * y++, "C/H/S:     %d/%d/%d", data_buf[1], data_buf[3], data_buf[6]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "Sectors:   0x%04X%04X", data_buf[7], data_buf[8]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "SectLBA:   0x%04X%04X", data_buf[61], data_buf[60]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "ECCBytes:  0x%04X  SectWrMax: %d", data_buf[22], data_buf[47]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "Caps:      0x%04X  FeatsSup:  0x%04X%04X%04X", data_buf[49], data_buf[82], data_buf[83], data_buf[84]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "MWTrans:   0x%04X  FeatsEn:   0x%04X%04X%04X", data_buf[63], data_buf[85], data_buf[86], data_buf[87]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "MinMW:     0x%04X  MaxMW:     0x%04X", data_buf[65], data_buf[66]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "UDMA:      0x%04X  AdvPIO:    0x%04X", data_buf[88], data_buf[64]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "IDECap:    0x%04X  PCMCIACap: 0x%04X", data_buf[163], data_buf[164]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "Security:  0x%04X  VendorDat: 0x%04X%04X", data_buf[128], data_buf[158], data_buf[159]);
    bmp_printf(FONT_MED, 0, font_med.height * y++, "Power:     0x%04X  PowerReq:  0x%04X", data_buf[91], data_buf[160]);
    call("dispcheck");
    msleep(3000);
    canon_gui_enable_front_buffer(0);
}

static MENU_UPDATE_FUNC(cf_acc_read_update)
{
}

static MENU_SELECT_FUNC(cf_acc_read_select)
{
    gui_stop_menu();
    task_create("cf_acc_read_task", 0x1e, 0x1000, cf_acc_read_task, (void*)0);
}

static struct menu_entry cf_acc_menu[] =
{
    {
        .name = "Read CF details (MAY CAUSE ERR)",
        .update = &cf_acc_read_update,
        .select = &cf_acc_read_select,
        .priv = NULL,
        .icon_type = IT_ACTION,
    }
};

unsigned int cf_acc_init()
{
    menu_add("Debug", cf_acc_menu, COUNT(cf_acc_menu));
    return 0;
}

unsigned int cf_acc_deinit()
{
    menu_remove("Debug", cf_acc_menu, COUNT(cf_acc_menu));
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(cf_acc_init)
    MODULE_DEINIT(cf_acc_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Author", "g3gg0")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Description", "Direct CF access")
MODULE_STRINGS_END()

