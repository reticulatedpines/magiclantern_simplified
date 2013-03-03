/** 
 * AF microadjustment functions: get_afma, set_afma
 * Dot-Tune AFMA tuning method by Horshack
 * http://www.magiclantern.fm/forum/index.php?topic=4648
 */

#include "dryos.h"
#include "property.h"
#include "menu.h"
#include "bmp.h"
#include "lens.h"
#include "afma.h" // camera-specific, in platform dir

#ifdef CONFIG_AFMA_EXTENDED
#define AFMA_MAX 100
#else
#define AFMA_MAX 20
#endif

// we have to wait until the AFMA change is operated into Canon firmware
static int afma_ack = 0;

static void afma_wait_ack()
{
    for (int i = 0; i < 50; i++)
    {
        msleep(10);
        if (afma_ack) break;
    }
}

PROP_HANDLER(PROP_AFMA)
{
    ASSERT(len == sizeof(afma_buf));
    my_memcpy(afma_buf, buf, sizeof(afma_buf));
    afma_ack = 1;
}

int get_afma(int mode)
{
    if (mode == AFMA_MODE_AUTODETECT) mode = AFMA_MODE;
    
#ifdef CONFIG_AFMA_WIDE_TELE
    if (mode == AFMA_MODE_PER_LENS)
        return (AFMA_PER_LENS_WIDE + AFMA_PER_LENS_TELE) / 2;

    else if (mode == AFMA_MODE_PER_LENS_WIDE)
        return AFMA_PER_LENS_WIDE;

    else if (mode == AFMA_MODE_PER_LENS_TELE)
        return AFMA_PER_LENS_TELE;
#else
    if (mode == AFMA_MODE_PER_LENS)
        return AFMA_PER_LENS;
#endif

    else if (mode == AFMA_MODE_ALL_LENSES)
        return AFMA_ALL_LENSES;

    return 0;
}

void set_afma(int value, int mode)
{
    if (ml_shutdown_requested) return;

    value = COERCE(value, -AFMA_MAX, AFMA_MAX);

    if (mode == AFMA_MODE_AUTODETECT) mode = AFMA_MODE;

    if (mode == AFMA_MODE_DISABLED)
        {} // do nothing

#ifdef CONFIG_AFMA_WIDE_TELE
    else if (mode == AFMA_MODE_PER_LENS)
        AFMA_PER_LENS_WIDE = AFMA_PER_LENS_TELE = value;
    else if (mode == AFMA_MODE_PER_LENS_WIDE)
        AFMA_PER_LENS_WIDE = value;
    else if (mode == AFMA_MODE_PER_LENS_TELE)
        AFMA_PER_LENS_TELE = value;
#else
    else if (mode == AFMA_MODE_PER_LENS)
        AFMA_PER_LENS = value;
#endif
    
    else if (mode == AFMA_MODE_ALL_LENSES)
        AFMA_ALL_LENSES = value;
    
    else return; // bad arguments
    
    AFMA_MODE = mode & 0xFF;
    
    afma_ack = 0;
    prop_request_change(PROP_AFMA, afma_buf, sizeof(afma_buf));
    afma_wait_ack();
}

void set_afma_mode(int mode)
{
    AFMA_MODE = COERCE(mode, 0, 2);
    
    afma_ack = 0;
    prop_request_change(PROP_AFMA, afma_buf, sizeof(afma_buf));
    afma_wait_ack();
}


#ifdef FEATURE_AFMA_TUNING

/**
 * Dot-Tune AFMA
 */

int afma_mode = AFMA_MODE_AUTODETECT;
int afma_mode_index = 0;

static void afma_print_status(int8_t* score, int range_expand_factor)
{
    int max = 4;
    for (int i = -20; i <= 20; i++)
        max = MAX(max, score[i+20]);
    
    bmp_fill(45, 0, 0, 720, 20);

    char msg[5];
    snprintf(msg, sizeof(msg), "-%d", 20 * range_expand_factor);
    bmp_printf(SHADOW_FONT(FONT_SMALL), 3, 4, msg);
    snprintf(msg, sizeof(msg), "+%d", 20 * range_expand_factor);
    bmp_printf(SHADOW_FONT(FONT_SMALL), 695, 4, msg);
    
    for (int i = -20; i <= 20; i++)
    {
        int x = 353 + i*16;
        int s = score[i+20];
        int h = s * 16 / max;
        if (s) h = MAX(h, 2);
        bmp_fill(COLOR_BLACK, x, 2, 14, 16-h);
        bmp_fill(COLOR_WHITE, x, 2 + 16-h, 14, h);
    }
    
    int afma = get_afma(afma_mode);
    int xc = 353 + COERCE(afma / range_expand_factor, -20, 20) * 16;
    bmp_fill(COLOR_RED, xc, 2, 14, 16);
    snprintf(msg, sizeof(msg), "%d", afma);
    bmp_printf(SHADOW_FONT(FONT_SMALL), xc + 8 - strlen(msg)*font_small.width/2, 4, msg);
}


static int wait_for_focus_confirmation_val(int maxTimeToWaitMs, int valToWaitFor)
{
    int timeWaitedMs = 0;
    for ( ;; )
    {
        int fc = FOCUS_CONFIRMATION ? 1 : 0;
        if (fc == valToWaitFor)
            return 1; // detected value
        if (timeWaitedMs >= maxTimeToWaitMs)
            return 0; // timed-out before detecting value
        msleep(10);
        timeWaitedMs += 10;
    }
}

static void afma_auto_tune(int range_expand_factor)
{
    if (AFMA_MODE == AFMA_MODE_DISABLED) return;
    
    int afma0 = get_afma(afma_mode);
    
    msleep(1000);

    if (is_movie_mode()) { NotifyBox(5000, "Switch to photo mode and try again."); beep(); return; };
    
    if (lv) { fake_simple_button(BGMT_LV); msleep(1000); }
    
    for (int i = 0; i < 5; i++)
    {
        if (!DISPLAY_IS_ON || !display_idle())
        {
            fake_simple_button(BGMT_INFO);
            msleep(500);
        }
    }

    if (lv) { NotifyBox(5000, "Turn off LiveView and try again."); beep(); return; }
    if (!DISPLAY_IS_ON || !display_idle()) { NotifyBox(5000, "Press " INFO_BTN_NAME " and try again."); beep(); return; }
    if (!is_manual_focus()) { NotifyBox(5000, "Switch to MF and try again."); beep(); return; }
    if (!lens_info.name[0]) { NotifyBox(5000, "Attach a chipped lens and try again."); beep(); return; }
    NotifyBox(5000, "You should have perfect focus.");
    msleep(2000);
    NotifyBox(5000, "Leave the camera still...");
    msleep(2000);
    NotifyBoxHide();
    msleep(100);
    
    int8_t score[41];
    for (int i = -20; i <= 20; i++) score[i+20] = 0;
    afma_print_status(score, range_expand_factor);
    msleep(1000);
    
    set_afma(-20 * range_expand_factor, afma_mode);
    assign_af_button_to_halfshutter();
    msleep(100);

    SW1(1,100);

    for (int k = 0; k < 2; k++)
    {
        for (int c = 0; c <= 80; c++) // -20 ... 20 and back to -20
        {
            int i = (c <= 40) ? c-20 : 60-c;
            set_afma(i * range_expand_factor, afma_mode);
            msleep(100);
            
            int fc;
            // initial focus must occur within 200ms
            fc = wait_for_focus_confirmation_val(200, 1);
           
            if (fc)
            {
                // weak or strong confirmation? use a higher score if strong
                // focus must sustain for 500ms to be considered strong
                if (!wait_for_focus_confirmation_val(500, 0))
                    // focus sustained
                    fc = 3;
            }            
            
            score[i+20] += fc;
            afma_print_status(score, range_expand_factor);
            
            if (!HALFSHUTTER_PRESSED)
            {
                beep();
                set_afma(afma0, afma_mode);
                NotifyBox(2000, "Canceled by user.");
                return;
            }
        }
    }
    SW1(0,100);
    
    beep();

    restore_af_button_assignment();

    int s = 0;
    int w = 0;
    for (int i = -20; i <= 20; i++)
    {
        // values confirmed multiple times, and/or strongly confirmed, are weighted a lot more
        int wi = score[i+20] * score[i+20] * score[i+20] * score[i+20];
        s += (wi * i * range_expand_factor);
        w += wi;
    }
    if (w < 10)
    {
        NotifyBox(5000, "OOF, check focus and contrast.");
        set_afma(afma0, afma_mode);
    }
    else
    {
        int afma = s / w;
        NotifyBox(10000, "New AFMA: %d", afma);
        set_afma(afma, afma_mode);
        msleep(300);
        afma_print_status(score, range_expand_factor);
        //~ call("dispcheck"); // screenshot
        
        if (score[0] > 5 || score[40] > 5) // focus confirmed for extreme values
        {
            msleep(3000);
            beep();
            if (range_expand_factor == 1)
                NotifyBox(5000, "Try scanning from -40 to +40.");
            else if (range_expand_factor == 2)
                NotifyBox(5000, "Try scanning from -100 to +100.");
            else
                NotifyBox(5000, "Double-check the focus target.");
        }
    }
}

static void afma_auto_tune_std() { afma_auto_tune(1); }   // -20...20
static void afma_auto_tune_ext() { afma_auto_tune(2); }   // -40...40
static void afma_auto_tune_ultra() { afma_auto_tune(5); } // -100...100

// for toggling
static int afma_mode_to_index(int mode)
{
    if (mode == AFMA_MODE_PER_LENS_WIDE) return 3;
    else if (mode == AFMA_MODE_PER_LENS_TELE) return 4;
    else return mode;
}

static int afma_index_to_mode(int index)
{
    if (index == 3) return AFMA_MODE_PER_LENS_WIDE;
    else if (index == 4) return AFMA_MODE_PER_LENS_TELE;
    else return index;
}

// sync ML AFMA mode from Canon firmware
static void afma_mode_sync()
{
    int mode = AFMA_MODE;

    // no lens? disable ML AFMA controls
    if (!lens_info.name[0])
        mode = AFMA_MODE_DISABLED;

    if ((afma_mode & 0xFF) != mode)
    {
        afma_mode = mode;

        #ifdef CONFIG_AFMA_WIDE_TELE
        if (afma_mode == AFMA_MODE_PER_LENS) // wide/tele or both?
        {
            int w = get_afma(AFMA_MODE_PER_LENS_WIDE);
            int t = get_afma(AFMA_MODE_PER_LENS_TELE);
            if (w != t)
                afma_mode = AFMA_MODE_PER_LENS_WIDE; // don't know focal length limits yet, so just choose wide
        }
        #endif
    }

    afma_mode_index = afma_mode_to_index(afma_mode);
}

static MENU_UPDATE_FUNC(afma_generic_update)
{
    if (!afma_mode)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "AFMA is disabled.");
}

static MENU_UPDATE_FUNC(afma_display)
{
    afma_mode_sync();
    
    int afma = get_afma(afma_mode);
    
    if (afma_mode)
    {
        MENU_SET_VALUE(
            "%s%d", 
            afma > 0 ? "+" : "", afma
        );
        
        #ifdef CONFIG_AFMA_WIDE_TELE
        if (afma_mode == AFMA_MODE_PER_LENS)
        {
            int w = get_afma(AFMA_MODE_PER_LENS_WIDE);
            int t = get_afma(AFMA_MODE_PER_LENS_TELE);
            if (w != t)
                MENU_SET_VALUE("W:%d T:%d", w, t);
        }
        #endif

        MENU_SET_ICON(MNI_PERCENT, 50 + afma * 50 / AFMA_MAX);
    }
    else
    {
        MENU_SET_VALUE("Disabled");
        MENU_SET_ICON(MNI_OFF, 0);
    }
    
    afma_generic_update(entry, info);
}

static MENU_SELECT_FUNC(afma_toggle)
{
    if (afma_mode == AFMA_MODE_DISABLED)
        return;
    
    int afma = get_afma(afma_mode);
    menu_numeric_toggle(&afma, delta, -AFMA_MAX, AFMA_MAX);
    set_afma(afma, afma_mode);
}

static MENU_SELECT_FUNC(afma_mode_toggle)
{
    #ifdef CONFIG_AFMA_WIDE_TELE
    int afma_index_max = 4;
    #else
    int afma_index_max = 2;
    #endif
    
    if (!lens_info.name[0])
        return;

    afma_mode_sync();

    menu_numeric_toggle(&afma_mode_index, delta, 0, afma_index_max);
    afma_mode = afma_index_to_mode(afma_mode_index);
    
    set_afma_mode(afma_mode);
}

static struct menu_entry afma_menu[] = {
    {
        .name = "DotTune AFMA",
        .select = menu_open_submenu,
        .help  = "Auto calibrate AF microadjustment ( youtu.be/7zE50jCUPhM )",
        .help2 = "Before running, focus manually on a test target in LiveView.",
        .depends_on = DEP_CHIPPED_LENS | DEP_PHOTO_MODE,
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "Scan default range  (-20...+20)",
                .priv = afma_auto_tune_std,
                .select = (void (*)(void*,int))run_in_separate_task,
                .update = afma_generic_update,
                .help  = "Step 1: achieve critical focus in LiveView with 10x zoom.",
                .help2 = "Step 2: run this and leave the camera still for ~2 minutes.",
                .depends_on = DEP_MANUAL_FOCUS,
            },
            #ifdef CONFIG_AFMA_EXTENDED
            {
                .name = "Scan extended range (-40...+40)",
                .priv = afma_auto_tune_ext,
                .select = (void (*)(void*,int))run_in_separate_task,
                .update = afma_generic_update,
                .help  = "This uses AFMA values normally not available in Canon menu.",
                .help2 = "Run if focus is confirmed at +/-20, or if normal scan fails.",
                .depends_on = DEP_MANUAL_FOCUS,
            },
            {
                .name = "Scan extended range (-100..+100)",
                .priv = afma_auto_tune_ultra,
                .select = (void (*)(void*,int))run_in_separate_task,
                .update = afma_generic_update,
                .help  = "This uses AFMA values normally not available in Canon menu.",
                .help2 = "Less accurate. Run only if you have severe focus problems.",
                .depends_on = DEP_MANUAL_FOCUS,
            },
            #endif
            {
                .name = "AF microadjust",
                .update = afma_display,
                .select = afma_toggle,
                #ifdef CONFIG_AFMA_EXTENDED
                .help  = "Adjust AFMA value manually. Range: -100...+100.",
                #else
                .help  = "Adjust AFMA value manually. Range: -20...+20.",
                #endif
                .edit_mode = EM_MANY_VALUES,
            },
            {
                .name = "AFMA mode",
                .priv = &afma_mode_index,
                .select = afma_mode_toggle,
                .min = 0,
                #ifdef CONFIG_AFMA_WIDE_TELE
                .max = 4,
                #else
                .max = 2,
                #endif
                .choices = (const char *[]) {"Disabled", "All lenses", "This lens", "This lens, wide", "This lens, tele"},
                .help  = "Where to apply the AFMA adjustment.",
                .icon_type = IT_DICE_OFF,
            },
            MENU_EOL
        },
    },
};

void afma_menu_init()
{
    menu_add( "Focus", afma_menu, COUNT(afma_menu));
}

#endif
