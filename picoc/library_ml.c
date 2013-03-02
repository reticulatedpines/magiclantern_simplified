#include "interpreter.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define UNPRESS -10000 // generic unpress code
#define NO_KEY -1

static char camera_model[32];
static char firmware_version[32];
PROP_HANDLER(PROP_CAM_MODEL)
{
    snprintf(camera_model, sizeof(camera_model), (const char *)buf);
}

PROP_HANDLER(PROP_FIRMWARE_VER)
{
    snprintf(firmware_version, sizeof(firmware_version), (const char *)buf);
}

static void LibSleep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int ms = (int)roundf(Param[0]->Val->FP * 1000.0f);
    script_msleep(ms);
}

static void LibBeep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    beep();
}

static void LibBeeps(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    beep_times(Param[0]->Val->Integer);
}

static void LibConsoleShow(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    console_show();
}

static void LibConsoleHide(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    console_hide();
}

static void LibCls(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    console_clear();
}

static void LibScreenshot(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    #ifdef FEATURE_SCREENSHOT
    take_screenshot(1);
    #else
    console_printf("screenshot: not available\n");
    #endif
}

static void LibRand(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = rand();
}

struct _tm { int hour; int minute; int second; int year; int month; int day; };
static void LibGetTime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct tm now;
    script_LoadCalendarFromRTC(&now);
    
    static struct _tm t;
    t.hour = now.tm_hour;
    t.minute = now.tm_min;
    t.second = now.tm_sec;
    t.year = now.tm_year + 1900;
    t.month = now.tm_mon + 1;
    t.day = now.tm_mday;
    ReturnValue->Val->Pointer = &t;
}

static void LibGetUptime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = get_ms_clock_value() / 1000.0f;
}

static void LibTakePic(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    take_a_pic(AF_DONT_CHANGE, 0);
}

static void LibBulbPic(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int ms = (int)roundf(Param[0]->Val->FP * 1000.0f);
    bulb_take_pic(ms);
}

static void LibTakeFastPics(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int num = Param[0]->Val->Integer;
    take_fast_pictures(num);
}

static void LibWaitPic(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int f0 = file_number;
    while (file_number == f0) msleep(20);
    lens_wait_readytotakepic(64);
}

static void LibMovieStart(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    movie_start();
}

static void LibMovieEnd(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    movie_end();
}

static void LibPress(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int btn = Param[0]->Val->Integer;
    if (btn < 0) return;
    
    switch (btn)
    {
        case BGMT_PRESS_HALFSHUTTER:
            SW1(1,50);
            return;
        case BGMT_PRESS_FULLSHUTTER:
            SW2(1,50);
            return;
        default:
            if (btn >= 0) fake_simple_button(btn);
    }
}

static void LibUnpress(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int press = Param[0]->Val->Integer;
    if (press < 0) return;

    int unpress = 0;
    switch (press)
    {
        case BGMT_PRESS_HALFSHUTTER:
            SW1(0,50);
            return;
        case BGMT_PRESS_FULLSHUTTER:
            SW2(0,50);
            return;
        case BGMT_PRESS_SET:
            unpress = BGMT_UNPRESS_SET;
            break;
        #ifdef BGMT_UNPRESS_ZOOMIN_MAYBE
        case BGMT_PRESS_ZOOMIN_MAYBE:
            unpress = BGMT_UNPRESS_ZOOMIN_MAYBE;
            break;
        #endif
        #ifdef BGMT_PRESS_ZOOMOUT_MAYBE
        case BGMT_PRESS_ZOOMOUT_MAYBE:
            unpress = BGMT_UNPRESS_ZOOMOUT_MAYBE;
            break;
        #endif
        
        #ifdef BGMT_UNPRESS_UDLR
        case BGMT_PRESS_LEFT:
        case BGMT_PRESS_RIGHT:
        case BGMT_PRESS_UP:
        case BGMT_PRESS_DOWN:
            unpress = BGMT_UNPRESS_UDLR;
            break;
        #else
        case BGMT_PRESS_LEFT: 
            unpress = BGMT_UNPRESS_LEFT; 
            break;
        case BGMT_PRESS_RIGHT: 
            unpress = BGMT_UNPRESS_RIGHT; 
            break;
        case BGMT_PRESS_UP: 
            unpress = BGMT_UNPRESS_UP; 
            break;
        case BGMT_PRESS_DOWN: 
            unpress = BGMT_UNPRESS_DOWN; 
            break;
        #endif
        
        default:
            // this button does not have an unpress code
            return;
    }
    ASSERT(unpress > 0);
    fake_simple_button(unpress);
}

static void LibClick(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    LibPress(Parser, ReturnValue, Param, NumArgs);
    LibUnpress(Parser, ReturnValue, Param, NumArgs);
}

static volatile int key_pressed = -1;
static volatile int waiting_for_key_last_request = 0; // will expire after 1 second

static void queue_cleanup()
{
    int t = get_ms_clock_value();
    if (t > waiting_for_key_last_request + 700) // events from queue are too old, discard them
    {
        while (script_key_dequeue() != -1)
            { /* send to /dev/null */ };
    }
}

static void LibWaitKey(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    queue_cleanup();
    
    int key_pressed;
    console_set_status_text("Waiting key press...");
    while ((key_pressed = script_key_dequeue()) == -1)
    {
        waiting_for_key_last_request = get_ms_clock_value();
        script_msleep(20);
    }
    console_set_status_text("Script running...");
    
    ReturnValue->Val->Integer = key_pressed;
}

static void LibLastKey(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    queue_cleanup();

    console_set_status_text("Script uses keys...");
    waiting_for_key_last_request = get_ms_clock_value();
    ReturnValue->Val->Integer = script_key_dequeue();
}

int handle_picoc_lib_keys(struct event * event)
{
    if (IS_FAKE(event))
        return 1; // so we can pass certain keys back to Canon code from script, e.g. if (key == ERASE) click(ERASE);
    
    if (get_ms_clock_value() > waiting_for_key_last_request + 1000)
    {
        waiting_for_key_last_request = 0;
        console_set_status_text("Script running...");
    }

    if (!waiting_for_key_last_request)
        return 1;
    
    switch (event->param)
    {
        case BGMT_PRESS_LEFT:
            console_printf("{LEFT}\n");
            goto key_can_block;

        case BGMT_PRESS_RIGHT:
            console_printf("{RIGHT}\n");
            goto key_can_block;

        case BGMT_PRESS_UP:
            console_printf("{UP}\n");
            goto key_can_block;

        case BGMT_PRESS_DOWN:
            console_printf("{DOWN}\n");
            goto key_can_block;

        case BGMT_WHEEL_UP:
            console_printf("{WHEEL_UP}\n");
            goto key_can_block;

        case BGMT_WHEEL_DOWN:
            console_printf("{WHEEL_DOWN}\n");
            goto key_can_block;

        case BGMT_WHEEL_LEFT:
            console_printf("{WHEEL_LEFT}\n");
            goto key_can_block;

        case BGMT_WHEEL_RIGHT:
            console_printf("{WHEEL_RIGHT}\n");
            goto key_can_block;

        #ifdef BGMT_JOY_CENTER
        case BGMT_JOY_CENTER:
            event->param = BGMT_PRESS_SET;
            // fallthru
        #endif

        case BGMT_PRESS_SET:
            console_printf("{SET}\n");
            goto key_can_block;

        case BGMT_MENU:
            console_printf("{MENU}\n");
            goto key_can_block;

        case BGMT_PLAY:
            console_printf("{PLAY}\n");
            goto key_can_block;

        case BGMT_TRASH:
            console_printf("{ERASE}\n");
            goto key_can_block;

        case BGMT_INFO:
            console_printf("{INFO}\n");
            goto key_can_block;

        case BGMT_LV:
            console_printf("{LV}\n");
            goto key_can_block;

        #ifdef BGMT_Q
        case BGMT_Q:
            console_printf("{Q}\n");
            goto key_can_block;
        #endif

        case BGMT_PRESS_ZOOMIN_MAYBE:
            console_printf("{ZOOM_IN}\n");
            goto key_can_block;

        #ifdef BGMT_PRESS_ZOOMOUT_MAYBE
        case BGMT_PRESS_ZOOMOUT_MAYBE:
            console_printf("{ZOOM_OUT}\n");
            goto key_can_block;
        #endif
            
        case BGMT_PRESS_HALFSHUTTER:
            console_printf("{SHOOT_HALF}\n");
            goto key_cannot_block;
        
        case BGMT_UNPRESS_HALFSHUTTER:
            console_printf("{UNPRESS}\n");
            script_key_enqueue(UNPRESS);
            return 1;

        case BGMT_UNPRESS_SET:
        #ifdef BGMT_UNPRESS_UDLR
        case BGMT_UNPRESS_UDLR:
        #else
        case BGMT_UNPRESS_LEFT:
        case BGMT_UNPRESS_RIGHT:
        case BGMT_UNPRESS_UP:
        case BGMT_UNPRESS_DOWN:
        #endif
        #ifdef BGMT_UNPRESS_ZOOMIN_MAYBE
        case BGMT_UNPRESS_ZOOMIN_MAYBE:
        #endif
            console_printf("{UNPRESS}\n");
            script_key_enqueue(UNPRESS);
            return 0;
        
        default:
            // unknown event, pass to canon code
            return 1;
    }
    
key_can_block:
    script_key_enqueue(event->param);
    return 0;

key_cannot_block:
    script_key_enqueue(event->param);
    return 1;
}

static void LibGetTv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = APEX_TV(lens_info.raw_shutter) / 8.0f;
}
static void LibGetAv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = APEX_AV(lens_info.raw_aperture) / 8.0f;
}
static void LibGetSv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = APEX_SV(lens_info.raw_iso) / 8.0f;
}
static void LibSetTv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int tv = (int)roundf(Param[0]->Val->FP * 8.0f);
    lens_set_rawshutter(-APEX_TV(-tv));
}
static void LibSetAv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int av = (int)roundf(Param[0]->Val->FP * 8.0f);
    lens_set_rawaperture(-APEX_AV(-av));
}
static void LibSetSv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int sv = (int)roundf(Param[0]->Val->FP * 8.0f);
    lens_set_rawiso(-APEX_SV(-sv));
}

static void LibGetShutter(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = raw2shutterf(lens_info.raw_shutter);
}
static void LibGetAperture(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = lens_info.aperture / 10.0f;
}
static void LibGetISO(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = lens_info.iso;
}
static void LibSetShutter(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_set_rawshutter(shutterf_to_raw(Param[0]->Val->FP));
}
static void LibSetAperture(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int val = (int)roundf(Param[0]->Val->FP * 10.0f);
    lens_set_aperture(val);
}
static void LibSetISO(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_set_iso(Param[0]->Val->Integer);
}

static void LibGetRawShutter(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = lens_info.raw_shutter;
}
static void LibGetRawAperture(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = lens_info.raw_aperture;
}
static void LibGetRawISO(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = lens_info.raw_iso;
}
static void LibSetRawShutter(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_set_rawshutter(Param[0]->Val->Integer);
}
static void LibSetRawAperture(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_set_rawaperture(Param[0]->Val->Integer);
}
static void LibSetRawISO(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_set_rawiso(Param[0]->Val->Integer);
}

static void LibGetAE(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = lens_info.ae / 8.0f;
}
static void LibSetAE(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int ae = (int)roundf(Param[0]->Val->FP * 8.0f);
    lens_set_ae(ae);
}

static void LibGetFlashAE(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = lens_info.flash_ae / 8.0f;
}
static void LibSetFlashAE(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int ae = (int)roundf(Param[0]->Val->FP * 8.0f);
    lens_set_flash_ae(ae);
}

// canon codes: 0=on, 1=off, 2=auto
// ml: 0=off, 1=on, 2=auto
int strobo_fix_logic(int mode)
{
    if (mode == 0) return 1;
    else if (mode == 1) return 0;
    return mode;
}
static void LibGetFlash(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strobo_fix_logic(strobo_firing);
}
static void LibSetFlash(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    set_flash_firing(strobo_fix_logic(Param[0]->Val->Integer));
}

static void LibPopFlash(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int req = 1;
    prop_request_change(PROP_POPUP_BUILTIN_FLASH, &req, 4);
}

static void LibGetKelvin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = lens_info.kelvin;
}
static void LibSetKelvin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_set_kelvin(Param[0]->Val->Integer);
}
static void LibGetGreen(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = lens_info.wbs_gm;
}
static void LibSetGreen(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_set_wbs_gm(Param[0]->Val->Integer);
}

static void LibFocus(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    LensFocus(Param[0]->Val->Integer);
}

static void LibFocusSetup(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    LensFocusSetup(Param[0]->Val->Integer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

static void LibGetFocusConfirm(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = FOCUS_CONFIRMATION;
}

static int af_dirty = 0;
static void LibSetAF(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    af_dirty = 1;
    lens_setup_af(Param[0]->Val->Integer);
}

static void LibResetAF(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_cleanup_af();
}

// called when script finishes
void script_cleanup_af() { if (af_dirty) lens_cleanup_af(); }

static void LibGetAFMA(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    #ifdef CONFIG_AFMA
    ReturnValue->Val->Integer = get_afma(Param[0]->Val->Integer);
    #else
    console_printf("get_afma: not available\n");
    #endif
}

static void LibSetAFMA(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    #ifdef CONFIG_AFMA
    set_afma(Param[0]->Val->Integer, Param[1]->Val->Integer);
    #else
    console_printf("set_afma: not available\n");
    #endif
}

struct _dof { char* lens_name; int focal_len; int focus_dist; int dof; int far; int near; int hyperfocal; };
static void LibGetDofInfo(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    static struct _dof d;
    d.lens_name = lens_info.name;
    d.focal_len = lens_info.focal_len;
    d.focus_dist = lens_info.focus_dist;
    d.far = lens_info.dof_far;
    d.near = lens_info.dof_near;
    d.dof = d.far - d.near;
    d.hyperfocal = lens_info.hyperfocal;
    ReturnValue->Val->Pointer = &d;
}

static void LibMicOut(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    #ifdef FEATURE_MIC_POWER
    mic_out(Param[0]->Val->Integer);
    // todo: will also work on 600D, http://www.magiclantern.fm/forum/index.php?topic=4577.msg26886#msg26886
    #else
    console_printf("mic_out: not available\n");
    #endif
}

static void LibSetLed(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int led = Param[0]->Val->Integer;
    int val = Param[1]->Val->Integer;
    if (led)
    {
        if (val) info_led_on();
        else info_led_off();
    }
    else
    {
        if (val) _card_led_on();
        else _card_led_off();
    }
}

static void LibNotifyBox(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int ms = (int)roundf(Param[0]->Val->FP * 1000.0f);
    char msg[512];

    struct OutputStream StrStream;
    
    extern void SPutc(unsigned char Ch, union OutputStreamInfo *Stream);
    StrStream.Putch = &SPutc;
    StrStream.i.Str.Parser = Parser;
    StrStream.i.Str.WritePos = msg;
    
    // there doesn't seem to be any bounds checking :(

    GenericPrintf(Parser, ReturnValue, Param+1, NumArgs-1, &StrStream);
    PrintCh(0, &StrStream);

    NotifyBox(ms, msg);
}

static void LibBmpPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int fnt = Param[0]->Val->Integer;
    int x = Param[1]->Val->Integer;
    int y = Param[2]->Val->Integer;
    char msg[512];

    struct OutputStream StrStream;
    
    extern void SPutc(unsigned char Ch, union OutputStreamInfo *Stream);
    StrStream.Putch = &SPutc;
    StrStream.i.Str.Parser = Parser;
    StrStream.i.Str.WritePos = msg;
    
    // there doesn't seem to be any bounds checking :(

    GenericPrintf(Parser, ReturnValue, Param+3, NumArgs-3, &StrStream);
    PrintCh(0, &StrStream);

    bmp_printf(fnt, x, y, "%s", msg);
}

static void LibClrScr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    clrscr();
}

static void LibGetPixel(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    ReturnValue->Val->Integer = bmp_getpixel(x, y);
}

static void LibPutPixel(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    int color = Param[2]->Val->Integer;
    bmp_putpixel(x, y, color);
}

static void LibDrawLine(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int x1 = Param[0]->Val->Integer;
    int y1 = Param[1]->Val->Integer;
    int x2 = Param[2]->Val->Integer;
    int y2 = Param[3]->Val->Integer;
    int color = Param[4]->Val->Integer;
    draw_line(x1, y1, x2, y2, color);
}

static void LibDrawLinePolar(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    int radius = Param[2]->Val->Integer;
    int angle = (int)roundf(Param[3]->Val->FP * 10.0);
    int color = Param[4]->Val->Integer;
    draw_angled_line(x, y, radius, angle, color);
}

static void LibDrawCircle(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    int radius = Param[2]->Val->Integer;
    int color = Param[3]->Val->Integer;
    draw_circle(x, y, radius, color);
}

static void LibFillCircle(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    int radius = Param[2]->Val->Integer;
    int color = Param[3]->Val->Integer;
    fill_circle(x, y, radius, color);
}

static void LibDrawRect(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    int w = Param[2]->Val->Integer;
    int h = Param[3]->Val->Integer;
    int color = Param[4]->Val->Integer;
    bmp_draw_rect(color, x, y, w, h);
}

static void LibFillRect(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    int w = Param[2]->Val->Integer;
    int h = Param[3]->Val->Integer;
    int color = Param[4]->Val->Integer;
    bmp_fill(color, x, y, w, h);
}

static void LibSetCanonGUI(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int enabled = Param[0]->Val->Integer;
    if (enabled)
        canon_gui_enable_front_buffer(1);
    else
        canon_gui_disable_front_buffer();
}

static void LibSetGuiMode(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int mode = Param[0]->Val->Integer;
    SetGUIRequestMode(mode);
}

static void LibGetGuiMode(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = CURRENT_DIALOG_MAYBE;
}

static void LibMenuOpen(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    gui_open_menu();
}

static void LibMenuClose(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    gui_stop_menu();
}

static void LibMenuSelect(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char* tab = Param[0]->Val->Pointer;
    char* entry = Param[1]->Val->Pointer;
    select_menu_by_name(tab, entry);
}

static void LibMenuGet(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char* tab = Param[0]->Val->Pointer;
    char* entry = Param[1]->Val->Pointer;
    ReturnValue->Val->Integer = menu_get_value_from_script(tab, entry);
}

static void LibMenuGetStr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char* tab = Param[0]->Val->Pointer;
    char* entry = Param[1]->Val->Pointer;
    
    if (gui_menu_shown())
    {
        console_printf("menu_get_str: close ML menu first\n");
        ReturnValue->Val->Pointer = "(err)";
    }
    ReturnValue->Val->Pointer = (char*) menu_get_str_value_from_script(tab, entry);
}

static void LibMenuSet(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char* tab = Param[0]->Val->Pointer;
    char* entry = Param[1]->Val->Pointer;
    int value = Param[2]->Val->Integer;
    ReturnValue->Val->Integer = menu_set_value_from_script(tab, entry, value);
}

static void LibMenuSetStr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char* tab = Param[0]->Val->Pointer;
    char* entry = Param[1]->Val->Pointer;
    char* value = Param[2]->Val->Pointer;
    
    if (gui_menu_shown())
    {
        console_printf("menu_set_str: close ML menu first\n");
        ReturnValue->Val->Integer = 0;
    }
    ReturnValue->Val->Integer = menu_set_str_value_from_script(tab, entry, value, 0xdeadbeaf);
}

static void LibDisplayOn(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    display_on();
}
static void LibDisplayOff(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    display_off();
}
static void LibDisplayIsOn(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = DISPLAY_IS_ON;
}
static void LibLVPause(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    PauseLiveView();
    display_off();
}
static void LibLVResume(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ResumeLiveView();
}

static void LibCacheLocked(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = script_cache_locked();
}
static void LibCacheLock(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    script_cache_lock();
}
static void LibCacheUnlock(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    script_cache_unlock();
}
static void LibCacheFake(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    uint32_t address = Param[0]->Val->Integer;
    uint32_t data = Param[1]->Val->Integer;
    uint32_t type = Param[2]->Val->Integer;
    
    ReturnValue->Val->Integer = script_cache_fake(address, data, type);
}

static void LibGetModel(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = camera_model;
}

static void LibGetFirmware(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = firmware_version;
}


/* list of all library functions and their prototypes */
struct LibraryFunction PlatformLibrary[] =
{
    /** General-purpose functions **/
    {LibSleep,          "void sleep(float seconds);"    },  // sleep X seconds
    {LibBeep,           "void beep();"                  },  // short beep sound
    {LibBeeps,          "void beeps(int num);"          },  // short consecutive beeps
    {LibConsoleShow,    "void console_show();"          },  // show the script console
    {LibConsoleHide,    "void console_hide();"          },  // hide the script console
    {LibCls,            "void cls();"                   },  // clear the script console
    {LibScreenshot,     "void screenshot();"            },  // take a screenshot (BMP+422)
    {LibRand,           "unsigned int rand();"          },  // random numbers

    /** Date/time **/
    
    /**
     * struct tm 
     * {
     *      int hour; 
     *      int minute; 
     *      int second;
     *      int year; 
     *      int month; 
     *      int day; 
     * }
     */
    
    {LibGetTime,        "struct tm * get_time();"       },  // get current date/time
    {LibGetUptime,      "float get_uptime();"           },  // get uptime, in seconds, 1ms resolution (from DIGIC clock)

    /** Picture taking **/
    {LibTakePic,        "void takepic();"               },  // take a picture
    {LibBulbPic,        "void bulbpic(float seconds);"  },  // take a picture in bulb mode
    {LibTakeFastPics,   "void take_fast_pics(int number);" },  // take N pictures in burst mode
    
    {LibWaitPic,        "void wait_pic();"              },  // waits until you take a picture (e.g. for starting a custom bracket sequence)
    
    /** Video recording **/
    {LibMovieStart,     "void movie_start();"           },  // start recording
    {LibMovieEnd,       "void movie_end();"             },  // stop recording

    /** Key press emulation **/
    
    /**
     * Available button codes:
     *      LEFT, RIGHT, UP, DOWN, SET, MENU, PLAY, ERASE, LV, INFO, ZOOM_IN
     *      SHOOT_FULL, SHOOT_HALF
     *      Q, ZOOM_OUT (if present on your camera)
     *      UNPRESS (for key input only)
     */
    
    {LibPress,          "void press(int button);"       },  // "press" a button
    {LibUnpress,        "void unpress(int button);"     },  // "unpress" a button
    {LibClick,          "void click(int button);"       },  // "press" and then "unpress" a button
    
    /** Key input **/

    /**
     * Two methods:
     * - blocking:      int key = wait_key();      // waits for key to be pressed, then returns the key code
     * - non-blocking:  int key = last_key();     // returns the last key code without waiting (or -1)
     * 
     * Keys are trapped when you call one of those, and also 1 second after. This lets you write loops like:
     * 
     * while(1)
     * {
     *     int key = wait_key();
     * 
     *     // process the key
     *     if (key == SET) { ... }
     * }
     * 
     * or
     * 
     * while(1)
     * {
     *     int key = last_key();
     * 
     *     // process the key
     *     if (key == SET) { ... }
     * 
     *     sleep(0.1);
     * }
     */

    {LibWaitKey,        "int wait_key();"               },  // waits until you press some key, then returns key code
    {LibLastKey,        "int last_key();"               },  // returns last key pressed, without waiting

    /** Exposure settings **/
    
    // APEX units
    {LibGetTv,        "float get_tv();"                 },
    {LibGetAv,        "float get_av();"                 },
    {LibGetSv,        "float get_sv();"                 },
    {LibSetTv,        "void set_tv(float tv);"          },
    {LibSetAv,        "void set_av(float av);"          },
    {LibSetSv,        "void set_sv(float sv);"          },

    // Conventional units ( ISO 100, 1.0/4000, 2.8 )
    {LibGetISO,        "int get_iso();"                 },
    {LibGetShutter,    "float get_shutter();"           },
    {LibGetAperture,   "float get_aperture();"          },
    {LibSetISO,        "void set_iso(int iso);"         },
    {LibSetShutter,    "void set_shutter(float s);"     },
    {LibSetAperture,   "void set_aperture(float s);"    },

    // Raw units (1/8 EV steps)
    {LibGetRawISO,      "int get_rawiso();"             },
    {LibGetRawShutter,  "int get_rawshutter();"         },
    {LibGetRawAperture, "int get_rawaperture();"        },
    {LibSetRawISO,      "void set_rawiso(int raw);"     },
    {LibSetRawShutter,  "void set_rawshutter(int raw);" },
    {LibSetRawAperture, "void set_rawaperture(int raw);"},
    
    /** Exposure compensation (in EV) **/
    {LibGetAE,          "float get_ae();"               },
    {LibSetAE,          "void set_ae(float ae);"        },

    /** Flash functions **/
    {LibGetFlash,       "int get_flash();"              }, // 1=enabled, 0=disabled, 2=auto
    {LibSetFlash,       "int set_flash(int enabled);"   },
    {LibPopFlash,       "int pop_flash();"              }, // pop-up built-in flash
    {LibGetFlashAE,     "float get_flash_ae();"         },
    {LibSetFlashAE,     "void set_flash_ae(float ae);"  }, // flash exposure compensation

    /** White balance **/
    {LibGetKelvin,      "int get_kelvin();"             },
    {LibGetGreen,       "int get_green();"              },
    {LibSetKelvin,      "void set_kelvin(int k);"       }, // from 1500 to 15000
    {LibSetGreen,       "void set_green(int gm);"       }, // green-magenta shift, from -9 to 9

    /** Focus **/
    {LibFocus,          "void focus(int steps);"                }, // move the focus ring by X steps
    {LibFocusSetup,     "void focus_setup(int stepsize, int delay, int wait);" }, // see Focus -> Focus Settings menu
    {LibGetFocusConfirm, "int get_focus_confirm();"             }, // return AF confirmation state (outside LiveView, with shutter halfway pressed)
    
    {LibSetAF,          "void set_af(int af);"                  }, // enable or disable AF for half-shutter press
    {LibResetAF,        "void reset_af(int af);"                }, // restore the original setting from Canon menu (CFn) (auto-called when script finishes)

    {LibGetAFMA,        "int get_afma(int mode);"               }, // get AF microadjust value
    {LibSetAFMA,        "void set_afma(int value, int mode);"   }, // set AF microadjust value
    
    /**
     * struct dof 
     * {
     *      char* lens_name; 
     *      int focal_len; 
     *      int focus_dist; 
     *      int dof; int far; 
     *      int near; 
     *      int hyperfocal; 
     * }
     */
    
    {LibGetDofInfo,     "struct dof * get_dof();"   },
    
    /** Low-level I/O **/
    {LibMicOut,          "void mic_out(int value);"                                  }, // digital output via microphone jack, by toggling mic power
    {LibSetLed,          "void set_led(int led, int value);"                         }, // set LED state; 1 = card LED, 2 = blue LED

#if 0
    {LibMicPrintf,       "void mic_printf(int baud, char* fmt, ...);"                }, // UART via microphone jack
    {LibWavPrintf,       "void wav_printf(int baud, char* fmt, ...);"                }, // UART via audio out (WAV)
    {LibLedPrintf,       "void led_printf(int baud, char* fmt, ...);"                }, // UART via card LED
    
    {LibMorseLedPrintf,  "void morse_led_printf(float dit_duration, char* fmt, ...);"    }, // Morse via card LED
    {LibMorseWavPrintf,  "void morse_wav_printf(float dit_duration, char* fmt, ...);"    }, // Morse via audio out (WAV)
#endif

    /** Graphics **/
    /**
    * Graphics constants:
    * 
    * Colors:
    *     COLOR_EMPTY, COLOR_BLACK, COLOR_WHITE, COLOR_BG,
    *     COLOR_RED, COLOR_DARK_RED, COLOR_GREEN1, COLOR_GREEN2, COLOR_BLUE, COLOR_LIGHT_BLUE,
    *     COLOR_CYAN, COLOR_MAGENTA, COLOR_YELLOW, COLOR_ORANGE,
    *     COLOR_ALMOST_BLACK, COLOR_ALMOST_WHITE,
    *     COLOR_GRAY(percent)
    *
    * Fonts: 
    *     FONT_LARGE, FONT_MED, FONT_SMALL
    *     FONT(fnt, fg, bg)
    *     SHADOW_FONT(fnt)
    *     e.g. FONT(FONT_LARGE, COLOR_YELLOW, COLOR_BLACK) or SHADOW_FONT(FONT_MED)
    */
    
    {LibClrScr,         "void clrscr();"                                                            },  // clear screen
    {LibGetPixel,       "int get_pixel(int x, int y);"                                              },
    {LibPutPixel,       "void put_pixel(int x, int y, int color);"                                  },
    {LibDrawLine,       "void draw_line(int x1, int y1, int x2, int y2, int color);"                },
    {LibDrawLinePolar,  "void draw_line_polar(int x, int y, int radius, float angle, int color);"   },
    {LibDrawCircle,     "void draw_circle(int x, int y, int radius, int color);"                    },
    {LibFillCircle,     "void fill_circle(int x, int y, int radius, int color);"                    },
    {LibDrawRect,       "void draw_rect(int x, int y, int w, int h, int color);"                    },
    {LibFillRect,       "void fill_rect(int x, int y, int w, int h, int color);"                    },

    /** Text output **/
    {LibBmpPrintf,      "void bmp_printf(int fnt, int x, int y, char* fmt, ...);"                   },
    {LibNotifyBox,      "void notify_box(float duration, char* fmt, ...);"                          },

    /** Interaction with Canon GUI **/
    {LibSetCanonGUI,    "void set_canon_gui(int enabled);"                          }, // allow disabling Canon graphics
    {LibSetGuiMode,     "void set_gui_mode(int mode);"    }, // set Canon GUI mode (current dialog, 0=idle, 1=play, 2=menu, others camera-specific)
    {LibGetGuiMode,     "int get_gui_mode();"    },

    /** Interaction with ML menus **/
    // Tip: to get a list with menus and possible values, go to Prefs -> Config File -> Export as PicoC script
    // You can also use these functions to create custom presets.
    { LibMenuOpen,       "void menu_open();"                                     }, // open ML menu
    { LibMenuClose,      "void menu_close();"                                    }, // close ML menu
    { LibMenuSelect,     "void menu_select(char* tab, char* entry);"             }, // select a menu tab and entry (e.g. Overlay, Focus Peak)
    { LibMenuGet,        "int menu_get(char* tab, char* entry);"                 }, // return the raw (integer) value from a menu entry
    { LibMenuSet,        "int menu_set(char* tab, char* entry, int value);"      }, // set a menu entry to some arbitrary value; 1 = success, 0 = failure
    { LibMenuGetStr,     "char* menu_get_str(char* tab, char* entry);"           }, // return the displayed (string) value from a menu entry
    { LibMenuSetStr,     "int menu_set_str(char* tab, char* entry, char* value);"}, // set a menu entry to some arbitrary string value (cycles until it gets it); 1 = success, 0 = failure

#if 0 // not yet implemented
    /** MLU, HTP, misc settings */
    {LibGetMLU,         "int get_mlu();"                },
    {LibGetHTP,         "int get_htp();"                },
    {LibGetALO,         "int get_alo();"                },
    {LibSetMLU,         "void set_mlu(int mlu);"        },
    {LibSetHTP,         "void set_htp(int htp);"        },
    {LibSetALO,         "void set_alo(int alo);"        },
    
    /** Image analysis */
    { LibGetLVBuf,       "struct vram_info * get_lv_buffer();" }        // get LiveView image buffer
    { LibGetHDBuf,       "struct vram_info * get_hd_buffer();" }        // get LiveView recording buffer
    { LibGetPixelYUV,    "void get_pixel_yuv(int x, int y, int* Y, int* U, int* V);" },          // get the YUV components of a pixel from LiveView (720x480)
    { LibGetSpotYUV,     "void get_spot_yuv(int x, int y, int size, int* Y, int* U, int* V);" }, // spotmeter: average pixels from a (small) box and return average YUV
    { LibYUV2RGB,        "void yuv2rgb(int Y, int U, int V, int* R, int* G, int* B);"},          // convert from YUV to RGB
    { LibRGB2YUV,        "void rgb2yuv(int R, int G, int B, int* Y, int* U, int* V);"},          // convert from RGB to YUV
    { LibGetHistoRange,  "float get_histo_range(int from, int to);"},                            // percent of values between <from> and <to> in histogram data
    
    /** Audio stuff */
    { LibAudioGetLevel,  "int audio_get_level(int channel, int type);" }, // channel: 0/1; type: INSTANT, AVG, PEAK, PEAK_FAST
    { LibAudioLevelToDB, "int audio_level_to_db(int level)"            }, // conversion from 16-bit signed to dB
#endif

    /** Powersaving **/
    { LibDisplayOn,     "void display_on();"           },
    { LibDisplayOff,    "void display_off();"          },
    { LibDisplayIsOn,   "int display_is_on();"         },

    { LibLVPause,       "void lv_pause();"             }, // pause LiveView without dropping the mirror
    { LibLVResume,      "void lv_resume();"            },
    
    /** Cache hacking **/
    { LibCacheLocked,   "unsigned int cache_locked();" },
    { LibCacheLock,     "void cache_lock();"           },
    { LibCacheUnlock,   "void cache_unlock();"         },
    { LibCacheFake,     "void cache_fake(unsigned int address, unsigned int data, unsigned int type);" },
    
    /** Camera info **/
    { LibGetModel,      "char* get_model();"           },
    { LibGetFirmware,   "char* get_firmware();"        },
    { NULL,         NULL }
};

static void lib_parse(const char* definition)
{
    PicocParse("ml lib", definition, strlen(definition), TRUE, TRUE, FALSE);
}

#define READONLY_VAR(x) VariableDefinePlatformVar(NULL, #x, &IntType, (union AnyValue *)&x,      FALSE);

#define CONST(var, value) lib_parse("#define " #var " " STR(value));
#define CONST0(var)       lib_parse("#define " #var " " STR(var));

void PlatformLibraryInit()
{
    lib_parse("struct tm { int hour; int minute; int second; int year; int month; int day; };");
    lib_parse("struct dof { char* lens_name; int focal_len; int focus_dist; int dof; int far; int near; int hyperfocal; };");

    LibraryAdd(&GlobalTable, "platform library", &PlatformLibrary[0]);

    /** Button codes */
    CONST(LEFT,         BGMT_PRESS_LEFT)
    CONST(RIGHT,        BGMT_PRESS_RIGHT)
    CONST(UP,           BGMT_PRESS_UP)
    CONST(DOWN,         BGMT_PRESS_DOWN)
    CONST(WHEEL_LEFT,   BGMT_WHEEL_LEFT)
    CONST(WHEEL_RIGHT,  BGMT_WHEEL_RIGHT)
    CONST(WHEEL_UP,     BGMT_WHEEL_UP)
    CONST(WHEEL_DOWN,   BGMT_WHEEL_DOWN)
    CONST(SET,          BGMT_PRESS_SET)
    CONST(MENU,         BGMT_MENU)
    CONST(PLAY,         BGMT_PLAY)
    CONST(ERASE,        BGMT_TRASH)
    CONST(INFO,         BGMT_INFO)
    CONST(LV,           BGMT_LV)
    CONST(ZOOM_IN,      BGMT_PRESS_ZOOMIN_MAYBE)
    CONST(SHOOT_HALF,   BGMT_PRESS_HALFSHUTTER)
    CONST(SHOOT_FULL,   BGMT_PRESS_FULLSHUTTER)

    #ifdef BGMT_Q
    CONST(Q,            BGMT_Q)
    #else
    CONST(Q,            -1)
    #endif

    #ifdef BGMT_PRESS_ZOOMOUT_MAYBE
    CONST(ZOOM_OUT,     BGMT_PRESS_ZOOMOUT_MAYBE)
    #else
    CONST(ZOOM_OUT,     -1)
    #endif

    CONST0(UNPRESS)
    CONST0(NO_KEY)

    /** Color codes */
    CONST0(COLOR_EMPTY)
    CONST0(COLOR_BLACK)
    CONST0(COLOR_WHITE)
    CONST0(COLOR_BG)

    CONST0(COLOR_RED)
    CONST0(COLOR_DARK_RED)
    CONST0(COLOR_GREEN1)
    CONST0(COLOR_GREEN2)
    CONST0(COLOR_BLUE)
    CONST0(COLOR_LIGHT_BLUE)
    CONST0(COLOR_CYAN)
    CONST0(COLOR_MAGENTA)
    CONST0(COLOR_YELLOW)
    CONST0(COLOR_ORANGE)

    CONST0(COLOR_ALMOST_BLACK) // 38
    CONST0(COLOR_ALMOST_WHITE) // 79
    lib_parse("#define COLOR_GRAY(percent) (38 + (percent) * 41 / 100)"); // COLOR_GRAY(50) is 50% gray

    /** Font constants */
    CONST0(FONT_LARGE)
    CONST0(FONT_MED)
    CONST0(FONT_SMALL)
    CONST0(FONT_MASK)
    CONST0(SHADOW_MASK)
    
    lib_parse("#define FONT(font,fg,bg) (((font) & (FONT_MASK | SHADOW_MASK)) | ((bg) & 0xFF) << 8 | ((fg) & 0xFF) << 0)");
    lib_parse("#define SHADOW_FONT(fnt) ((fnt) | SHADOW_MASK)");

    /** Common operators */
    lib_parse("#define MIN(a,b) ((a) < (b) ? (a) : (b))");
    lib_parse("#define MAX(a,b) ((a) > (b) ? (a) : (b))");
    lib_parse("#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))");
    lib_parse("#define ABS(a) ((a) < (0) ? -(a) : (a))");
    lib_parse("#define SGN(a) ((a) > 0 ? 1 : (a) < 0 ? -1 : 0)");
    
    CONST0(M_PI);

    /** cache hacking */
    CONST0(TYPE_DCACHE);
    CONST0(TYPE_ICACHE);
    
    /** common properties */

    READONLY_VAR(lv)
    READONLY_VAR(recording)
}
