
/* Simple module to force either 480p or 1080i output by patching ROM function which set HDMI video code.
 * This method is more reliable compared to prop_request_change in FEATURE_FORCE_HDMI_VGA.
 * FEATURE_FORCE_HDMI_VGA doesn't work on some models:
 * https://www.magiclantern.fm/forum/index.php?topic=26108.0                               */

#include <module.h>
#include <dryos.h>
#include <menu.h>
#include <config.h>
#include <patch.h>

#define OUTPUT_480p  0
#define OUTPUT_1080i_50Hz 1
#define OUTPUT_1080i_60Hz 2
#define OUTPUT_1080p_24Hz 3

static CONFIG_INT("hdmi.patch.enabled", hdmi_patch_enabled, 0);
static CONFIG_INT("hdmi.output_resolution", output_resolution, 0);

static int hdmi_output_patch_status = 0;

enum {
    HDMI_NOT_PATCHED = 0,
    HDMI_PATCHED = 1,
    HDMI_PATCHED_480p = 2,
    HDMI_PATCHED_1080i_50Hz = 20,
    HDMI_PATCHED_1080i_60Hz = 5,
    HDMI_PATCHED_1080p_24Hz = 32, // Only on 5D3 1.2.3
};

static uint32_t Set_HDMI_Code = 0;

/* These appear to hold the selected HDMI configuration before applying it
 * we can detect LCD, 480p or 1080i outputs
 *
 * This LOG from 700D when connecting HDMI to 480p ouput:
 *DisplayMgr:ff330104:88:16: [EDID] dwVideoCode = 2
 *DisplayMgr:ff330118:88:16: [EDID] dwHsize = 720
 *DisplayMgr:ff33012c:88:16: [EDID] dwVsize = 480
 *DisplayMgr:ff330148:88:16: [EDID] ScaningMode = EDID_NON_INTERLACE(p)
 *DisplayMgr:ff330194:88:16: [EDID] VerticalFreq = EDID_FREQ_60Hz
 *DisplayMgr:ff3301b0:88:16: [EDID] AspectRatio = EDID_ASPECT_4x3
 *DisplayMgr:ff3301cc:88:16: [EDID] AudioMode = EDID_AUDIO_LINEAR_PCM
 *DisplayMgr:ff331580:88:16: [EDID] ColorMode = EDID_COLOR_RGB */
const struct EDID_HDMI_INFO
{
    uint32_t dwVideoCode;   /* 0 LCD, 2 480p, 5 1080i */
    uint32_t dwHsize;       /* LCD = 0, 480p = 720, 1080i = 1920 */
    uint32_t dwVsize;       /* LCD = 0, 480p = 480, 1080i = 1080 */
    uint32_t ScaningMode;   /* 0 = EDID_NON_INTERLACE(p), 1 = EDID_INTERLACE(i) */
    uint32_t VerticalFreq;
    uint32_t AspectRatio;   /* 0 = EDID_ASPECT_4x3, 1 = EDID_ASPECT_16x9 */
    uint32_t AudioMode;
    uint32_t ColorMode;
} * EDID_HDMI_INFO = 0;

static void Set_HDMI_Code_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    /* regs[0] holds HDMI code value before applying it, we can override it from here at the start of Set_HDMI_Code function */
    /* also, only override regs[0] when HDMI code value isn't 0 (Canon might do some stuff before setting the final HDMI code value) */
    if (regs[0] != 0)
    {
        if (output_resolution == OUTPUT_480p) //  if ML menu selection is set to 480p
        {
            // force 480p HDMI code which is 2
            regs[0] = 2; 
            hdmi_output_patch_status = HDMI_PATCHED_480p;
        }
        if (output_resolution == OUTPUT_1080i_50Hz) // if ML menu selection is set to 1080i 50 Hz
        {
            // force 1080i 50Hz HDMI code which is 20
            regs[0] = 20; 
            hdmi_output_patch_status = HDMI_PATCHED_1080i_50Hz;
        }
        if (output_resolution == OUTPUT_1080i_60Hz) // if ML menu selection is set to 1080i 60 Hz
        {
            // force 1080i 60Hz HDMI code which is 5
            regs[0] = 5; 
            hdmi_output_patch_status = HDMI_PATCHED_1080i_60Hz;
        }

        /* ony works on 5D3 1.2.3 */
        if (output_resolution == OUTPUT_1080p_24Hz) // if ML menu selection is set to 1080p 24 Hz
        {
            // force 1080p 24Hz HDMI code which is 32
            regs[0] = 32; 
            hdmi_output_patch_status = HDMI_PATCHED_1080p_24Hz;
        }
    }
}

void patch_HDMI_output()
{
    if (hdmi_output_patch_status == HDMI_NOT_PATCHED)
    {
        patch_hook_function(Set_HDMI_Code, MEM(Set_HDMI_Code), Set_HDMI_Code_hook, "HDMI");
        hdmi_output_patch_status = HDMI_PATCHED;
    }
}

void unpatch_HDMI_output()
{
    if (hdmi_output_patch_status != HDMI_NOT_PATCHED)
    {
        unpatch_memory(Set_HDMI_Code);
        hdmi_output_patch_status = HDMI_NOT_PATCHED;
    }
}

static void hdmi_output_toggle(void* priv, int sign)
{
    if (hdmi_patch_enabled) 
    {
        hdmi_patch_enabled = 0;
        unpatch_HDMI_output();
    }
    else
    {
        hdmi_patch_enabled = 1;
        patch_HDMI_output();
    }
}

static MENU_UPDATE_FUNC(hdmi_update)
{
    if (EDID_HDMI_INFO->dwVideoCode == 0) // LCD, HDMI isn't connected
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "HDMI isn't connected.");
    }

    if (hdmi_output_patch_status != HDMI_NOT_PATCHED)
    {
        if (EDID_HDMI_INFO->dwVideoCode != 0) // Not LCD, HDMI is connected
        {
            if ((output_resolution == OUTPUT_480p       && EDID_HDMI_INFO->dwVideoCode != 2)  ||
                (output_resolution == OUTPUT_1080i_50Hz && EDID_HDMI_INFO->dwVideoCode != 20) ||
                (output_resolution == OUTPUT_1080i_60Hz && EDID_HDMI_INFO->dwVideoCode != 5)  || 
                (output_resolution == OUTPUT_1080p_24Hz && EDID_HDMI_INFO->dwVideoCode != 32)  )
            {
                MENU_SET_WARNING(MENU_WARN_ADVICE, "Reconnect HDMI cable or restart camera to apply output setting.");
            }
        }
    }
}

static MENU_UPDATE_FUNC(output_resolution_update)
{
    if (hdmi_output_patch_status != HDMI_NOT_PATCHED)
    {
        if (EDID_HDMI_INFO->dwVideoCode != 0) // Not LCD, HDMI is connected
        {
            if ((output_resolution == OUTPUT_480p       && EDID_HDMI_INFO->dwVideoCode != 2)  ||
                (output_resolution == OUTPUT_1080i_50Hz && EDID_HDMI_INFO->dwVideoCode != 20) ||
                (output_resolution == OUTPUT_1080i_60Hz && EDID_HDMI_INFO->dwVideoCode != 5)  || 
                (output_resolution == OUTPUT_1080p_24Hz && EDID_HDMI_INFO->dwVideoCode != 32)  )
            {
                MENU_SET_WARNING(MENU_WARN_ADVICE, "Reconnect HDMI cable or restart camera to apply output setting.");
            }
        }
    }
}

static struct menu_entry hdmi_out_menu[] =
{
    {
        .name       = "HDMI output",
        .select     = hdmi_output_toggle,
        .update     = hdmi_update,
        .max        = 1,
        .priv       = &hdmi_patch_enabled,
        .help       = "Change HDMI output settings.",
        .children =  (struct menu_entry[]) {
            {
                .name       = "Output resolution",
                .update     = output_resolution_update,
                .choices    = CHOICES("480p", "1080i 50Hz", "1080i 60Hz", "1080p 24Hz"),
                .max        = 3,
                .priv       = &output_resolution,
                .help       = "Select an output resolution for HDMI displays.",
                .help2      = "480p: 720x480 output.\n"
                              "1080i 50Hz: 1920x1080i @ 50Hz output.\n"
                              "1080i 60Hz: 1920x1080i @ 60Hz output.\n"
                              "1080p 24Hz: 1920x1080p @ 24Hz output.",
            },
            MENU_EOL,
        },
    },
};

static unsigned int hdmi_out_init()
{    
    if (is_camera("700D", "1.1.5"))
    {
        Set_HDMI_Code = 0xFF3303F8;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x648B0;
    }
    
    else if (is_camera("650D", "1.0.4"))
    {
        Set_HDMI_Code = 0xFF32D9D4;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x63F7C;
    }
    
    else if (is_camera("600D", "1.0.2"))
    {
        Set_HDMI_Code = 0xFF1ED008;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x2C4C0; 
    }

    else if (is_camera("550D", "1.0.9"))
    {
        Set_HDMI_Code = 0xFF1CD0D0;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x3BC60; 
    }

    else if (is_camera("500D", "1.1.1"))
    {
        Set_HDMI_Code = 0xFF19C840;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x32474; 
    }

    else if (is_camera("100D", "1.0.1"))
    {
        Set_HDMI_Code = 0xFF32374C;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0xA3C0C;
    }

    else if (is_camera("1200D", "1.0.2"))
    {
        Set_HDMI_Code = 0xFF2A7CB0;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x2E148;
    }
    
    else if (is_camera("EOSM", "2.0.2"))
    {
        Set_HDMI_Code = 0xFF331838;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x821CC;
    }

    else if (is_camera("EOSM2", "1.0.4"))
    {
        Set_HDMI_Code = 0xFF3418F4;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0xD75C4;
    }
    
    else if (is_camera("6D", "1.1.6"))
    {
        Set_HDMI_Code = 0xFF3223C0;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0xAECA4;
    }
    
    else if (is_camera("5D2", "2.1.2"))
    {
        Set_HDMI_Code = 0xFF1B027C;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x34384;
    }
    
    else if (is_camera("5D3", "1.1.3"))
    {
        Set_HDMI_Code = 0xFF2F7740;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x5198C; 
    }

    else if (is_camera("5D3", "1.2.3"))
    {
        Set_HDMI_Code = 0xFF2FBFC8;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x519E4; 
    }

    else if (is_camera("50D", "1.0.9"))
    {
        Set_HDMI_Code = 0xFF19C840;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x32474;
    }

    else if (is_camera("60D", "1.1.1"))
    {
        Set_HDMI_Code = 0xFF1D0B98;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0x49D08;
    }

    else if (is_camera("70D", "1.1.2"))
    {
        Set_HDMI_Code = 0xFF3369DC;
        EDID_HDMI_INFO = (struct EDID_HDMI_INFO *) 0xD2264; 
    }

    /* hide 1080p24 for not supported models */
    if (!is_camera("5D3", "1.2.3"))
    {
        hdmi_out_menu[0].children[0].max = 2;
        if (output_resolution == 3) output_resolution = 0;
    }

    if (Set_HDMI_Code)
    {
        menu_add("Display", hdmi_out_menu, COUNT(hdmi_out_menu));
    }
    else
    {
        hdmi_patch_enabled = 0;
        return 1;
    }

    /* patch on startup if "HDMI output" was enabled */
    if (hdmi_patch_enabled)
    {
        if (hdmi_output_patch_status == HDMI_NOT_PATCHED)
        {
            patch_HDMI_output();
        }
    }

    return 0;
}

static unsigned int hdmi_out_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(hdmi_out_init)
    MODULE_DEINIT(hdmi_out_init)
MODULE_INFO_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(hdmi_patch_enabled)
    MODULE_CONFIG(output_resolution)
MODULE_CONFIGS_END()