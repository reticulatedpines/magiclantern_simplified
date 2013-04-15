/** 
 * RAW overexposure warning
 * 
 * This works by underexposing the preview JPEGs by 2 EV
 * 
 **/

#include "dryos.h"
#include "bmp.h"
//#include "state-object.h"
#include "property.h"
#include "config.h"
#include "menu.h"
#include "lens.h"

#ifdef FEATURE_RAW_BLINKIES

static CONFIG_INT("raw.blinkies", raw_blinkies, 0);

CONFIG_INT("raw.blinkies.level.r", raw_blinkies_overexposure_level_R, 255);
CONFIG_INT("raw.blinkies.level.g", raw_blinkies_overexposure_level_G, 255);
CONFIG_INT("raw.blinkies.level.b", raw_blinkies_overexposure_level_B, 255);

static CONFIG_INT("raw.blinkies.ps.sig", raw_blinkies_ps_sig, 0);
static CONFIG_INT("raw.blinkies.wb.sig", raw_blinkies_wb_sig, 0);

int raw_blinkies_enabled() { return raw_blinkies; }

#ifdef CONFIG_550D
#define SCS_STATE (*(struct state_object **)0x31cc)
#elif CONFIG_5D2
#define SCS_STATE (*(struct state_object **)0x3168)
#endif

#define SHAD_GAIN      0xc0f08030

static int (*StateTransition)(void*,int,int,int,int) = 0;
static int stateobj_spy(struct state_object * self, int x, int input, int z, int t)
{
    //int old_state = self->current_state;
    int ans = StateTransition(self, x, input, z, t);
    //int new_state = self->current_state;
    
    /* fixme: only override when needed */
    
    //if (input == 5)
    {
        int gain = shamem_read(SHAD_GAIN);
        if (gain > 3000)
            MEM(SHAD_GAIN) = gain/4;
    }
    return ans;
}

static void stateobj_start_spy(struct state_object * stateobj)
{
    StateTransition = (void *)stateobj->StateTransition_maybe;
    stateobj->StateTransition_maybe = (void *)stateobj_spy;
}

static void stateobj_stop_spy(struct state_object * stateobj)
{
    if (StateTransition)
        stateobj->StateTransition_maybe = (void *)StateTransition;
}

static int get_ps_sig()
{
    return lens_info.picstyle * 123 + lens_get_contrast() + (get_htp() ? 17 : 23);
}

static int get_wb_sig()
{
    return lens_info.wb_mode + lens_info.kelvin + lens_info.WBGain_R * 314 + lens_info.WBGain_G * 111 + lens_info.WBGain_G * 747;
}


static void raw_blinkies_guess_overexposure_levels()
{
    msleep(1000);
    SetGUIRequestMode(1); // PLAY
    msleep(2000);
    
    int Y, U, V;
    get_spot_yuv(100, &Y, &U, &V);
    
    int Ychk, Uchk, Vchk;
    get_spot_yuv(30, &Ychk, &Uchk, &Vchk);
    
    if (ABS(Y - Ychk) > 1 || ABS(U - Uchk) > 1 || ABS(V - Vchk) > 1)
    {
        NotifyBox(5000, "Image not overexposed, try again.");
        return;
    }
    
    if (Y > 250)
    {
        NotifyBox(5000, "Take the pic with RAW blinkies ON.");
        return;
    }

    yuv2rgb(Y, U, V, &raw_blinkies_overexposure_level_R, &raw_blinkies_overexposure_level_G, &raw_blinkies_overexposure_level_B);
    NotifyBox(5000, "Clipping points: %d, %d, %d ", raw_blinkies_overexposure_level_R, raw_blinkies_overexposure_level_G, raw_blinkies_overexposure_level_B);
    raw_blinkies_ps_sig = get_ps_sig();
    raw_blinkies_wb_sig = get_wb_sig();
}

static MENU_SELECT_FUNC(raw_blinkies_toggle)
{
    raw_blinkies = !raw_blinkies;
    
    if (raw_blinkies)
        stateobj_start_spy(SCS_STATE);
    else
        stateobj_stop_spy(SCS_STATE);
}

static MENU_UPDATE_FUNC(raw_blinkies_update)
{
    int ps_changed = get_ps_sig() != raw_blinkies_ps_sig;
    int wb_changed = get_wb_sig() != raw_blinkies_wb_sig;

    if (ps_changed || wb_changed)
    {
        MENU_SET_WARNING(
            MENU_WARN_NOT_WORKING, 
            raw_blinkies_ps_sig == 0 ? "Please calibrate before using this (press %s)." :
            ps_changed               ? "Picture style changed, please recalibrate (press %s)." : 
            wb_changed               ? "White balance changed, please recalibrate (press %s)." : "err",
            Q_BTN_NAME
        );
    }
    
    int raw = pic_quality & 0x60000;
    if (!raw)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "You must shoot RAW in order to use this.");
    
    if (raw_blinkies)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "!!! This will underexpose all your JPEGs by 2 EV !!!");
}

static struct menu_entry raw_blink_menus[] = {
    {
        .name = "RAW Blinkies",
        .priv = &raw_blinkies,
        .max  = 1,
        .select = raw_blinkies_toggle,
        .update = raw_blinkies_update,
        .help  = "RAW overexposure warnings (for RGB zebras, histogram etc).",
        .depends_on = DEP_PHOTO_MODE,
        .children = (struct menu_entry[]) {
            {
                .name = "Calibrate",
                .priv = &raw_blinkies_guess_overexposure_levels,
                .select = (void (*)(void*,int))run_in_separate_task,
                .help  = "Autodetects clipping points for each channel (R,G,B).",
                .help2 = "Take a completely overexposed picture before running this.",
            },
            {
                .name = "RED clipping point",
                .priv = &raw_blinkies_overexposure_level_R, 
                .min = 128,
                .max = 255,
                .help = "Luma level where the RED channel starts clipping.",
            },
            {
                .name = "GREEN clipping point",
                .priv = &raw_blinkies_overexposure_level_G, 
                .min = 128,
                .max = 255,
                .help = "Luma level where the GREEN channel starts clipping.",
            },
            {
                .name = "BLUE clipping point",
                .priv = &raw_blinkies_overexposure_level_B, 
                .min = 128,
                .max = 255,
                .help = "Luma level where the BLUE channel starts clipping.",
            },
            MENU_EOL,
        },
    }
};

static void shootspy_init(void* unused)
{
    menu_add("Shoot", raw_blink_menus, COUNT(raw_blink_menus));
    if (raw_blinkies)
        stateobj_start_spy(SCS_STATE);
}

TASK_CREATE( "shootspy_init", shootspy_init, 0, 0x1e, 0x2000 );

#endif
