#include "interpreter.h"

static void LibSleep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int ms = (int)roundf(Param[0]->Val->FP * 1000.0f);
    script_msleep(ms);
}

static void LibBeep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    beep();
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

static void LibTakePic(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_take_picture(64,0);
}

static void LibBulbPic(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int ms = (int)roundf(Param[0]->Val->FP * 1000.0f);
    bulb_take_pic(ms);
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
    switch (btn)
    {
        case BGMT_PRESS_HALFSHUTTER:
            SW1(1,50);
            return;
        case BGMT_PRESS_FULLSHUTTER:
            SW2(1,50);
            return;
        default:
            if (btn > 0) fake_simple_button(btn);
    }
}

static void LibUnpress(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int press = Param[0]->Val->Integer;
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
        case BGMT_PRESS_ZOOMIN_MAYBE:
            unpress = BGMT_UNPRESS_ZOOMIN_MAYBE;
            break;
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

static volatile int key_pressed = 0;
static volatile int waiting_for_key = 0;
static void LibGetKey(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    while (!key_pressed)
    {
        waiting_for_key = 1;
        script_msleep(20);
    }
    waiting_for_key = 0;
    
    ReturnValue->Val->Integer = key_pressed;
    key_pressed = 0;
}

int handle_picoc_lib_keys(struct event * event)
{
    if (!waiting_for_key)
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
            
        default:
            // unknown event, pass to canon code
            return 1;
    }
    
key_can_block:
    key_pressed = event->param;
    return 0;

key_cannot_block:
    key_pressed = event->param;
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


/* list of all library functions and their prototypes */
struct LibraryFunction PlatformLibrary[] =
{
    /** General-purpose functions */
    {LibSleep,          "void sleep(float seconds);"    },  // sleep X seconds
    {LibBeep,           "void beep();"                  },  // short beep sound
    {LibConsoleShow,    "void console_show();"          },  // show the script console
    {LibConsoleHide,    "void console_hide();"          },  // hide the script console
    {LibCls,            "void cls();"                   },  // clear the script console
    {LibScreenshot,     "void screenshot();"            },  // take a screenshot (BMP+422)

    /** Date/time */
    // struct tm { int hour; int minute; int second; int year; int month; int day; }
    {LibGetTime,        "struct tm * get_time();"       },  // get current date/time

    /** Picture taking */
    {LibTakePic,        "void takepic();"               },  // take a picture
    {LibBulbPic,        "void bulbpic(float seconds);"  },  // take a picture in bulb mode
    
    /** Video recording */
    {LibMovieStart,     "void movie_start();"           },  // start recording
    {LibMovieEnd,       "void movie_end();"             },  // stop recording

    /** Button press emulation */
    // LEFT, RIGHT, UP, DOWN, SET, MENU, PLAY, ERASE, Q, LV, INFO, ZOOM_IN, ZOOM_OUT
    // SHOOT_FULL, SHOOT_HALF
    {LibPress,          "void press(int button);"       },  // "press" a button
    {LibUnpress,        "void unpress(int button);"     },  // "unpress" a button
    {LibClick,          "void click(int button);"       },  // "press" and then "unpress" a button
    
    /** Key input */
    {LibGetKey,         "int get_key();"                },  // waits until you press some key, then returns key code

    /** Exposure settings */
    // APEX units
    {LibGetTv,        "float get_tv();"                 },
    {LibGetAv,        "float get_av();"                 },
    {LibGetSv,        "float get_sv();"                 },
    {LibSetTv,        "void set_tv(float tv);"          },
    {LibSetAv,        "void set_av(float av);"          },
    {LibSetSv,        "void set_sv(float sv);"          },

    // Conventional units (ISO 100, 1.0/4000, 2.8)
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
    
    /** Exposure compensation (in EV) */
    {LibGetAE,          "float get_ae();"               },
    {LibGetFlashAE,     "float get_flash_ae();"         },
    {LibSetAE,          "void set_ae(float ae);"        },
    {LibSetFlashAE,     "void set_flash_ae(float ae);"  },

    /** White balance */
    {LibGetKelvin,      "int get_kelvin();"             },
    {LibGetGreen,       "int get_green();"              },
    {LibSetKelvin,      "void set_kelvin(int k);"       }, // from 1500 to 15000
    {LibSetGreen,       "void set_green(int gm);"       }, // green-magenta shift, from -9 to 9

    /** Focus */
    {LibFocus,          "void focus(int steps);"             }, // move the focus ring by X steps
    {LibFocusSetup,     "void focus_setup(int stepsize, int delay, int wait);" }, // see Focus -> Focus step settings
    {LibGetFocusConfirm, "int get_focus_confirm();"          }, // return AF confirmation state (outside LiveView, with shutter halfway pressed)

    {LibGetAFMA,        "int get_afma(int mode);"            }, // get AF microadjust value
    {LibSetAFMA,        "void set_afma(int value, int mode);" }, // set AF microadjust value
    
    // struct dof { char* lens_name; int focal_len; int focus_dist; int dof; int far; int near; int hyperfocal; }
    {LibGetDofInfo,     "struct dof * get_dof();"   },
    
    /** Low-level I/O */
    {LibMicOut,          "void mic_out(int value);"                                  }, // digital output via microphone jack, by toggling mic power
    {LibSetLed,          "void set_led(int led, int value);"                         }, // set LED state; 1 = card LED, 2 = blue LED

    //~ {LibMicPrintf,       "void mic_printf(int baud, char* fmt, ...);"                }, // UART via microphone jack
    //~ {LibWavPrintf,       "void wav_printf(int baud, char* fmt, ...);"                }, // UART via audio out (WAV)
    //~ {LibLedPrintf,       "void led_printf(int baud, char* fmt, ...);"                }, // UART via card LED
    
    //~ {LibMorseLedPrintf,  "void morse_led_printf(float dit_duration, char* fmt, ...);"    }, // Morse via card LED
    //~ {LibMorseWavPrintf,  "void morse_wav_printf(float dit_duration, char* fmt, ...);"    }, // Morse via audio out (WAV)
    
    /** Text output */
    //~ {LibBmpPrintf,      "void bmp_printf(int fnt, int x, int y, char* fmt, ...);"                   },
    {LibNotifyBox,      "void notify_box(float duration, char* fmt, ...);"                          },
    
    /** Graphics */
    //~ {LibClrScr,         "void clrscr();"                                                            },  // clear screen
    //~ {LibGetPixel,       "int get_pixel(int x, int y);"                                              },
    //~ {LibGetPixel,       "void put_pixel(int x, int y, int color);"                                  },
    //~ {LibDrawLine,       "void draw_line(int x1, int y1, int x2, int y2, int color);"                },
    //~ {LibDrawLinePolar,  "void draw_line_polar(int x, int y, int radius, float angle, int color);"   },
    //~ {LibDrawCircle,     "void draw_circle(int x, int y, int radius, int color);"                    },
    //~ {LibFillCircle,     "void fill_circle(int x, int y, int radius, int color);"                    },
    //~ {LibDrawRect,       "void draw_rect(int x, int y, int w, int h, int color);"                    },
    //~ {LibFillRect,       "void fill_rect(int x, int y, int w, int h, int color);"                    },

    /** Interaction with menus */
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

    /** Powersaving */
    { LibDisplayOn,     "void display_on();"           },
    { LibDisplayOff,    "void display_off();"          },
    { LibDisplayIsOn,   "int display_is_on();"         },

    { LibLVPause,       "void lv_pause();"             }, // pause LiveView without dropping the mirror
    { LibLVResume,      "void lv_resume();"            },
    { NULL,         NULL }
};

static void add_structure(const char* IntrinsicName, const char* StructDefinition)
{
    struct ParseState Parser;
    char *Identifier;
    struct ValueType *ParsedType;
    void *Tokens;
    Tokens = LexAnalyse(IntrinsicName, StructDefinition, strlen(StructDefinition), NULL);
    LexInitParser(&Parser, StructDefinition, Tokens, IntrinsicName, TRUE);
    TypeParse(&Parser, &ParsedType, &Identifier, NULL);
    HeapFreeMem(Tokens);
}

void PlatformLibraryInit()
{
    add_structure("tm struct",  "struct tm { int hour; int minute; int second; int year; int month; int day; }");
    add_structure("dof struct", "struct dof { char* lens_name; int focal_len; int focus_dist; int dof; int far; int near; int hyperfocal; }");

    LibraryAdd(&GlobalTable, "platform library", &PlatformLibrary[0]);

    static int LEFT = BGMT_PRESS_LEFT;
    static int RIGHT = BGMT_PRESS_RIGHT;
    static int UP = BGMT_PRESS_UP;
    static int DOWN = BGMT_PRESS_DOWN;

    static int WHEEL_UP = BGMT_WHEEL_UP;
    static int WHEEL_DOWN = BGMT_WHEEL_DOWN;
    static int WHEEL_LEFT = BGMT_WHEEL_LEFT;
    static int WHEEL_RIGHT = BGMT_WHEEL_RIGHT;

    static int SET = BGMT_PRESS_SET;
    static int MENU = BGMT_MENU;
    static int PLAY = BGMT_PLAY;
    static int ERASE = BGMT_TRASH;
    static int INFO = BGMT_INFO;
    static int Q = 
        #ifdef BGMT_Q
            BGMT_Q;
        #else
            0;
        #endif
    static int LV = BGMT_LV;
    static int ZOOM_IN = BGMT_PRESS_ZOOMIN_MAYBE;
    static int ZOOM_OUT = 
        #ifdef BGMT_PRESS_ZOOMOUT_MAYBE
            BGMT_PRESS_ZOOMOUT_MAYBE;
        #else
            0;
        #endif
    static int SHOOT_HALF = BGMT_PRESS_HALFSHUTTER;
    static int SHOOT_FULL = BGMT_PRESS_FULLSHUTTER;
    
    #define READONLY_VAR(x) VariableDefinePlatformVar(NULL, #x, &IntType, (union AnyValue *)&x,      FALSE);
    READONLY_VAR(LEFT)
    READONLY_VAR(RIGHT)
    READONLY_VAR(UP)
    READONLY_VAR(DOWN)
    READONLY_VAR(WHEEL_LEFT)
    READONLY_VAR(WHEEL_RIGHT)
    READONLY_VAR(WHEEL_UP)
    READONLY_VAR(WHEEL_DOWN)
    READONLY_VAR(SET)
    READONLY_VAR(MENU)
    READONLY_VAR(PLAY)
    READONLY_VAR(ERASE)
    READONLY_VAR(INFO)
    READONLY_VAR(Q)
    READONLY_VAR(LV)
    READONLY_VAR(ZOOM_IN)
    READONLY_VAR(ZOOM_OUT)
    READONLY_VAR(SHOOT_HALF)
    READONLY_VAR(SHOOT_FULL)

    READONLY_VAR(lv)
    READONLY_VAR(recording)
}
