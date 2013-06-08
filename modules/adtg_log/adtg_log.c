/**
 * 
 */

#define ADTG_WRITE_FUNC   0x11644
#define CMOS_WRITE_FUNC   0x119CC
#define CMOS16_WRITE_FUNC 0x11AB8

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>

#include <gdb.h>
#include <cache_hacks.h>

unsigned int hook_calls = 0;

unsigned int *adtg_buf = NULL;
unsigned int adtg_buf_pos = 0;
unsigned int adtg_buf_pos_max = 0;

unsigned short cmos_regs[8];

static void cmos_reg_update(unsigned short cmos_data)
{
    unsigned short reg = (cmos_data >> 12) & 0x07;
    unsigned short data = cmos_data & 0x0FFF;
    
    cmos_regs[reg] = data;
}

static unsigned int adtg_log_vsync_cbr(unsigned int unused)
{
    if(!adtg_buf)
    {
        return;
    }
    uint32_t old_int = cli();
    
    if(adtg_buf_pos + 2 > adtg_buf_pos_max)
    {
        return;
    }
    
    adtg_buf[adtg_buf_pos] = 0xFFFFFFFF;
    adtg_buf_pos++;    
    adtg_buf[adtg_buf_pos] = 0xFFFFFFFF;
    adtg_buf_pos++;
    
    sei(old_int);
}

static void adtg_log(breakpoint_t *bkpt)
{
    if(!adtg_buf)
    {
        return;
    }
    
    //hook_calls++;
    
    unsigned int cs = bkpt->ctx[0];
    unsigned int *data_buf = bkpt->ctx[1];
    
    while(*data_buf != 0xFFFFFFFF)
    {
        if(adtg_buf_pos + 2 > adtg_buf_pos_max)
        {
            return;
        }
        adtg_buf[adtg_buf_pos] = cs;
        adtg_buf_pos++;
        adtg_buf[adtg_buf_pos] = *data_buf;
        adtg_buf_pos++;
        data_buf++;
    }
}

static void cmos_log(breakpoint_t *bkpt)
{
    if(!adtg_buf)
    {
        return;
    }
    
    unsigned short *data_buf = bkpt->ctx[0];
    
    while(*data_buf != 0xFFFF)
    {
        if(adtg_buf_pos + 2 > adtg_buf_pos_max)
        {
            return;
        }
        adtg_buf[adtg_buf_pos] = 0x00FF0000;
        adtg_buf_pos++;
        adtg_buf[adtg_buf_pos] = *data_buf;
        
        cmos_reg_update(*data_buf);
        adtg_buf_pos++;
        data_buf++;
    }
}


static void cmos16_log(breakpoint_t *bkpt)
{
    if(!adtg_buf)
    {
        return;
    }
    
    hook_calls++;
    
    unsigned short *data_buf = bkpt->ctx[0];
    
    while(*data_buf != 0xFFFF)
    {
        if(adtg_buf_pos + 2 > adtg_buf_pos_max)
        {
            return;
        }
        adtg_buf[adtg_buf_pos] = 0xFF000000;
        adtg_buf_pos++;
        adtg_buf[adtg_buf_pos] = *data_buf;
        adtg_buf_pos++;
        data_buf++;
    }
}

void adtg_log_task()
{
    adtg_buf_pos_max = 128 * 1024;
    adtg_buf = shoot_malloc(adtg_buf_pos_max * 4);
    
    if(!adtg_buf)
    {   
        return;
    }
    
    gdb_setup();
    breakpoint_t *bkpt1 = gdb_add_watchpoint(ADTG_WRITE_FUNC, 0, &adtg_log);
    breakpoint_t *bkpt2 = gdb_add_watchpoint(CMOS_WRITE_FUNC, 0, &cmos_log);
    breakpoint_t *bkpt3 = gdb_add_watchpoint(CMOS16_WRITE_FUNC, 0, &cmos16_log);
    
    while(adtg_buf_pos + 2 < adtg_buf_pos_max)
    {
        bmp_printf(FONT_MED, 30, 60, "cmos:");
        for(int pos = 0; pos < 8; pos+=2)
        {
            bmp_printf(FONT_MED, 30, 80 + pos / 2 * font_med.height, "[%d] 0x%03X [%d] 0x%03X", pos, cmos_regs[pos], pos + 1, cmos_regs[pos+1]);
        }
        msleep(100);
    }
    
    gdb_delete_bkpt(bkpt1);
    gdb_delete_bkpt(bkpt2);
    gdb_delete_bkpt(bkpt3);
    
    char filename[100];
    snprintf(filename, sizeof(filename), "%s/test.bin", get_dcim_dir());
    FILE* f = FIO_CreateFileEx(filename);
    FIO_WriteFile(f, adtg_buf, adtg_buf_pos * 4); 
    FIO_CloseFile(f);
    
    /* free buffer */
    void *buf = adtg_buf;
    adtg_buf = NULL;
    shoot_free(buf);
}

static MENU_SELECT_FUNC(adtg_log_toggle)
{
    if(!adtg_buf)
    {
        task_create("adtg_log_task", 0x1e, 0x1000, adtg_log_task, (void*)0);
    }
    else
    {
        adtg_buf_pos_max = adtg_buf_pos;
    }
}


static struct menu_entry adtg_log_menu[] =
{
    {
        .name = "ADTG Logging",
        .select = &adtg_log_toggle,
        .help = "Log ADTG writes",
    }
};

static unsigned int adtg_log_init()
{
    menu_add("Movie", adtg_log_menu, COUNT(adtg_log_menu));
    return 0;
}

static unsigned int adtg_log_deinit()
{
    return 0;
}



MODULE_INFO_START()
    MODULE_INIT(adtg_log_init)
    MODULE_DEINIT(adtg_log_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Description", "ADTG Logging")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Author", "g3gg0")
MODULE_STRINGS_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_VSYNC, adtg_log_vsync_cbr, 0)
MODULE_CBRS_END()
