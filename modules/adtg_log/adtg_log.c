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
static int cmos_delta[8];

static void cmos_reg_update(unsigned short cmos_data)
{
    unsigned short reg = (cmos_data >> 12) & 0x07;
    unsigned short data = cmos_data & 0x0FFF;

    cmos_regs[reg] = data;
}

static void cmos_reg_manipulate(unsigned short *cmos_data)
{
    unsigned short reg = (*cmos_data >> 12) & 0x07;
    unsigned short data = *cmos_data & 0x0FFF;

    data += cmos_delta[reg];
    *cmos_data = (data & 0x0FFF) | (reg << 12);
}

static unsigned int adtg_log_vsync_cbr(unsigned int unused)
{
    if(!adtg_buf)
    {
        return;
    }
    
    if(adtg_buf_pos + 2 >= adtg_buf_pos_max)
    {
        return;
    }
    
    adtg_buf[adtg_buf_pos] = 0xFFFFFFFF;
    adtg_buf_pos++;    
    adtg_buf[adtg_buf_pos] = 0xFFFFFFFF;
    adtg_buf_pos++;
}

static void adtg_log(breakpoint_t *bkpt)
{
    if(!adtg_buf)
    {
        return;
    }
    
    unsigned int cs = bkpt->ctx[0];
    unsigned int *data_buf = bkpt->ctx[1];
    
    /* log all ADTG writes */
    while(*data_buf != 0xFFFFFFFF)
    {
        if(adtg_buf_pos + 2 >= adtg_buf_pos_max)
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
    static int loops = 0;
    
    loops++;
    loops &= 0xFFF;
    
    if(!adtg_buf)
    {
        return;
    }
    
    unsigned short *data_buf = bkpt->ctx[0];
    
    /* log all CMOS writes */
    while(*data_buf != 0xFFFF)
    {
        if(adtg_buf_pos + 2 >= adtg_buf_pos_max)
        {
            return;
        }
        adtg_buf[adtg_buf_pos] = 0x00FF0000;
        adtg_buf_pos++;
        adtg_buf[adtg_buf_pos] = *data_buf;
        adtg_buf_pos++;
        
        cmos_reg_update(*data_buf);
        cmos_reg_manipulate(data_buf);
        
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
    
    /* log all CMOS writes */
    while(*data_buf != 0xFFFF)
    {
        if(adtg_buf_pos + 2 >= adtg_buf_pos_max)
        {
            return;
        }
        adtg_buf[adtg_buf_pos] = 0xFF000000;
        adtg_buf_pos++;
        adtg_buf[adtg_buf_pos] = *data_buf;
        adtg_buf_pos++;
        
        cmos_reg_update(*data_buf);
        cmos_reg_manipulate(data_buf);
        data_buf++;
    }
}

void adtg_log_task()
{
    adtg_buf_pos_max = 128 * 1024;
    adtg_buf = shoot_malloc(adtg_buf_pos_max * 4 + 0x100);
    
    if(!adtg_buf)
    {   
        return;
    }
    
    /* set watchpoints at ADTG and CMOS writes */
    gdb_setup();
    breakpoint_t *bkpt1 = gdb_add_watchpoint(ADTG_WRITE_FUNC, 0, &adtg_log);
    breakpoint_t *bkpt2 = gdb_add_watchpoint(CMOS_WRITE_FUNC, 0, &cmos_log);
    breakpoint_t *bkpt3 = gdb_add_watchpoint(CMOS16_WRITE_FUNC, 0, &cmos16_log);
    
    /* wait for buffer being filled */
    while(adtg_buf_pos + 2 < adtg_buf_pos_max)
    {
        bmp_printf(FONT_MED, 30, 60, "cmos:");
        for(int pos = 0; pos < 8; pos+=2)
        {
            bmp_printf(FONT_MED, 30, 80 + pos / 2 * font_med.height, "[%d] 0x%03X [%d] 0x%03X", pos, cmos_regs[pos], pos + 1, cmos_regs[pos+1]);
        }
        msleep(100);
    }
    
    beep();
    
    /* uninstall watchpoints */
    gdb_delete_bkpt(bkpt1);
    gdb_delete_bkpt(bkpt2);
    gdb_delete_bkpt(bkpt3);
    
    /* dump all stuff */
    char filename[100];
    snprintf(filename, sizeof(filename), "%s/adtg.bin", get_dcim_dir());
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
        .priv = &adtg_buf,
        .max = 1,
        .help = "Log ADTG writes",
        .children =  (struct menu_entry[]) {
            {
                .name = "cmos[0]",
                .priv = cmos_delta,
                .edit_mode = EM_MANY_VALUES_LV,
                .min = -1000,
                .max = 1000,
            },
            {
                .name = "cmos[1]",
                .priv = cmos_delta+1,
                .edit_mode = EM_MANY_VALUES_LV,
                .min = -1000,
                .max = 1000,
            },
            {
                .name = "cmos[2]",
                .priv = cmos_delta+2,
                .edit_mode = EM_MANY_VALUES_LV,
                .min = -1000,
                .max = 1000,
            },
            {
                .name = "cmos[3]",
                .priv = cmos_delta+3,
                .edit_mode = EM_MANY_VALUES_LV,
                .min = -1000,
                .max = 1000,
            },
            {
                .name = "cmos[4]",
                .priv = cmos_delta+4,
                .edit_mode = EM_MANY_VALUES_LV,
                .min = -1000,
                .max = 1000,
            },
            {
                .name = "cmos[5]",
                .priv = cmos_delta+5,
                .edit_mode = EM_MANY_VALUES_LV,
                .min = -1000,
                .max = 1000,
            },
            {
                .name = "cmos[6]",
                .priv = cmos_delta+6,
                .edit_mode = EM_MANY_VALUES_LV,
                .min = -1000,
                .max = 1000,
            },
            {
                .name = "cmos[7]",
                .priv = cmos_delta+7,
                .edit_mode = EM_MANY_VALUES_LV,
                .min = -1000,
                .max = 1000,
            },
            MENU_EOL,
        },
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
