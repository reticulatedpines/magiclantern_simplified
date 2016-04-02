#include <dryos.h>
#include <module.h>
#include <config.h>
#include <menu.h>
#include <beep.h>
#include <property.h>
#include <patch.h>
#include <bmp.h>

#undef CROP_DEBUG

#ifdef CROP_DEBUG
#define dbg_printf(fmt,...) { printf(fmt, ## __VA_ARGS__); }
#else
#define dbg_printf(fmt,...) {}
#endif

static CONFIG_INT("crop.enabled", crop_enabled, 0);

/* camera-specific parameters */
static uint32_t CMOS_WRITE      = 0;
static uint32_t MEM_CMOS_WRITE  = 0;
static uint32_t ADTG_WRITE      = 0;
static uint32_t MEM_ADTG_WRITE  = 0;

static int is_5D3 = 0;

static int cmos_vidmode_ok = 0;

/* return value:
 *  1: registers checked and appear OK (1080p/720p video mode)
 *  0: registers checked and they are not OK (other video mode)
 * -1: registers not checked
 */
static int FAST check_cmos_vidmode(uint16_t* data_buf)
{
    int ok = 1;
    int found = 1;
    while (*data_buf != 0xFFFF)
    {
        int reg = (*data_buf) >> 12;
        int value = (*data_buf) & 0xFFF;
        
        if (is_5D3)
        {
            if (reg == 1)
            {
                found = 1;
                if (value != 0x800 &&   /* not 1080p? */
                    value != 0xBC2)     /* not 720p? */
                {
                    ok = 0;
                }
            }
        }
        
        data_buf++;
    }
    
    if (found) return ok;
    
    return -1;
}


static void FAST cmos_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    /* make sure we are in 1080p/720p mode */
    if (video_mode_resolution > 1)
    {
        /* looks like checking properties works fine for detecting
         * changes in video mode, but not for detecting the zoom change */
        return;
    }
    
    /* also check CMOS registers; in zoom mode, we get different values
     * and this check is instant (no delays).
     * 
     * on 5D3, the 640x480 acts like 1080p during standby,
     * so properties are our only option for that one.
     */
     
    uint16_t* data_buf = (uint16_t*) regs[0];
    int ret = check_cmos_vidmode(data_buf);
    
    if (ret >= 0)
    {
        cmos_vidmode_ok = ret;
    }
    
    if (ret != 1)
    {
        return;
    }

    int cmos_new[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    
    if (is_5D3)
    {
        cmos_new[1] = (video_mode_resolution)
            ? 0xE8E     /* 50/60fps, almost centered */
            : 0xECB ;   /* 24/25/30fps, almost centered */
        
        cmos_new[2] = 0x10E;
        cmos_new[6] = 0x170;
    }
    
    /* copy data into a buffer, to make the override temporary */
    /* that means: as soon as we stop executing the hooks, values are back to normal */
    static uint16_t copy[512];
    uint16_t* copy_end = &copy[COUNT(copy)];
    uint16_t* copy_ptr = copy;

    while (*data_buf != 0xFFFF)
    {
        *copy_ptr = *data_buf;

        int reg = (*data_buf) >> 12;
        if (cmos_new[reg] != -1)
        {
            *copy_ptr = (reg << 12) | cmos_new[reg];
            dbg_printf("CMOS[%x] = %x\n", reg, cmos_new[reg]);
        }
        
        data_buf++;
        copy_ptr++;
        if (copy_ptr > copy_end) while(1);
    }
    *copy_ptr = 0xFFFF;

    /* pass our modified register list to cmos_write */
    uint32_t* out_regs = stack - 14;
    out_regs[0] = (uint32_t) copy;
}

static void FAST adtg_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    if (video_mode_resolution > 1 || !cmos_vidmode_ok)
    {
        /* don't patch other video modes */
        return;
    }

    uint32_t cs = regs[0];
    uint32_t *data_buf = (uint32_t *) regs[1];
    int dst = cs & 0xF;
    
    /* copy data into a buffer, to make the override temporary */
    /* that means: as soon as we stop executing the hooks, values are back to normal */
    static uint32_t copy[512];
    uint32_t* copy_end = &copy[COUNT(copy)];
    uint32_t* copy_ptr = copy;
    
    struct adtg_new
    {
        int dst;
        int reg;
        int val;
    };
    
    /* expand this as required */
    struct adtg_new adtg_new[2] = {{0}};
    
    if (is_5D3)
    {
        /* ADTG2/4[0x8000] = 5 (set in one call) */
        /* ADTG2[0x8806] = 0x6088 */
        adtg_new[0] = (struct adtg_new) {6, 0x8000, 5};
        adtg_new[1] = (struct adtg_new) {2, 0x8806, 0x6088};
    }
    
    while(*data_buf != 0xFFFFFFFF)
    {
        *copy_ptr = *data_buf;
        int reg = (*data_buf) >> 16;
        for (int i = 0; i < COUNT(adtg_new); i++)
        {
            if ((reg == adtg_new[i].reg) && (dst & adtg_new[i].dst))
            {
                int new_value = adtg_new[i].val;
                dbg_printf("ADTG%x[%x] = %x\n", dst, reg, new_value);
                *(uint16_t*)copy_ptr = new_value;
            }
        }
        data_buf++;
        copy_ptr++;
        if (copy_ptr >= copy_end) while(1);
    }
    *copy_ptr = 0xFFFFFFFF;
    
    /* pass our modified register list to adtg_write */
    uint32_t* out_regs = stack - 14;
    out_regs[1] = (uint32_t) copy;
}

static int patch_active = 0;

static void update_patch()
{
    if (crop_enabled && is_movie_mode())
    {
        /* install our hooks, if we haven't already do so */
        if (!patch_active)
        {
            patch_hook_function(CMOS_WRITE, MEM_CMOS_WRITE, &cmos_hook, "crop_rec: CMOS[1,2,6] parameters hook");
            patch_hook_function(ADTG_WRITE, MEM_ADTG_WRITE, &adtg_hook, "crop_rec: ADTG[8000,8806] parameters hook");
            patch_active = 1;
        }
    }
    else
    {
        /* undo active patches, if any */
        if (patch_active)
        {
            unpatch_memory(CMOS_WRITE);
            unpatch_memory(ADTG_WRITE);
            patch_active = 0;
        }
    }
}

/* enable patch when switching LiveView (not in the middle of LiveView) */
/* otherwise you will end up with a halfway configured video mode that looks weird */
PROP_HANDLER(PROP_LV_ACTION)
{
    update_patch();
}

static MENU_UPDATE_FUNC(crop_update)
{
    if (!is_movie_mode())
    {
        return;
    }
    
    if (crop_enabled)
    {
        if (video_mode_resolution <= 1)
        {
            if (!patch_active)
            {
                MENU_SET_WARNING(MENU_WARN_ADVICE, "After leaving ML menu, press PLAY twice to enable crop mode.");
                MENU_SET_RINFO(SYM_WARNING);
            }
        }
        else
        {
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This feature only works in 1080p and 720p video modes.");
        }
    }
    else /* crop disabled */
    {
        if (patch_active)
        {
            MENU_SET_WARNING(MENU_WARN_ADVICE, "After leaving ML menu, press PLAY twice to return to normal mode.");
            MENU_SET_RINFO(SYM_WARNING);
        }
    }
}

static struct menu_entry crop_rec_menu[] =
{
    {
        .name = "Crop mode (3x)",
        .priv = &crop_enabled,
        .update = crop_update,
        .max = 1,
        .depends_on = DEP_MOVIE_MODE,
        .help = "Change 1080p and 720p movie modes into a 3x (1:1) crop mode.",
    },
};

static unsigned int crop_rec_init()
{
    if (is_camera("5D3",  "1.1.3") || is_camera("5D3", "1.2.3"))
    {
        /* same addresses on both 1.1.3 and 1.2.3 */
        CMOS_WRITE = 0x119CC;
        MEM_CMOS_WRITE = 0xE92D47F0;
        
        ADTG_WRITE = 0x11640;
        MEM_ADTG_WRITE = 0xE92D47F0;
        
        is_5D3 = 1;
    }
    
    menu_add("Movie", crop_rec_menu, COUNT(crop_rec_menu));
    return 0;
}

static unsigned int crop_rec_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(crop_rec_init)
    MODULE_DEINIT(crop_rec_deinit)
MODULE_INFO_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(crop_enabled)
MODULE_CONFIGS_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_LV_ACTION)
MODULE_PROPHANDLERS_END()
