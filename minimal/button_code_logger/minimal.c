/** \file
 * Minimal code for hooking Canon GUI task,
 * so we can log button presses to get the constants for gui.h
 */

#include "dryos.h"
#include "vram.h"
#include "bmp.h"
#include "font_direct.h"
#include "imgconv.h"

struct task *first_task = NULL; // needed to build due to usage in function_overrides.c
                                // for D678 cams, not used

#ifdef CONFIG_DIGIC_45
/** Returns a pointer to the real BMP vram, as reported by Canon firmware.
 *  Not to be used directly - it may be somewhere in the middle of VRAM! */
inline uint8_t* _bmp_vram_raw() { return bmp_vram_info[1].vram2; } 
#endif

#ifdef FEATURE_VRAM_RGBA
static uint8_t *bmp_vram_indexed = NULL;

inline uint8_t *_bmp_vram_raw() {
    struct MARV *marv = _rgb_vram_info;
    return marv ? marv->bitmap_data : NULL;
}

// XimrExe is used to trigger refreshing the OSD after the RGBA buffer
// has been updated.  Should probably take a XimrContext *,
// but this struct is not yet determined for 200D
extern int XimrExe(void *);
extern struct semaphore *winsys_sem;
void refresh_yuv_from_rgb(void)
{
    // get our indexed buffer, convert into our real rgb buffer
    uint8_t *b = bmp_vram_indexed;
    uint32_t *rgb_data = NULL;

    if (_rgb_vram_info != NULL)
        rgb_data = (uint32_t *)_rgb_vram_info->bitmap_data;
    else
    {
        DryosDebugMsg(0, 15, "_rgb_vram_info was NULL, can't refresh OSD");
        return;
    }

    //SJE FIXME benchmark this loop, it probably wants optimising
    for (size_t n = 0; n < BMP_VRAM_SIZE; n++)
    {
        // limited alpha support, if dest pixel would be full alpha,
        // don't copy into dest.  This is COLOR_TRANSPARENT_BLACK in
        // the LUT
        uint32_t rgb = indexed2rgb(*b);
        if ((rgb && 0xff000000) == 0x00000000)
            rgb_data++;
        else
            *rgb_data++ = rgb;
        b++;
    }

    // trigger Ximr to render to OSD from RGB buffer
#ifdef CONFIG_DIGIC_VI
    XimrExe((void *)XIMR_CONTEXT);
#else
    take_semaphore(winsys_sem, 0);
    XimrExe((void *)XIMR_CONTEXT);
    give_semaphore(winsys_sem);
#endif
}

static uint32_t indexed2rgbLUT[RGB_LUT_SIZE] = {
    0xffffffff, 0xffebebeb, 0xff000000, 0x00000000, 0xffa33800, // 0
    0xff20bbd9, 0xff009900, 0xff01ad01, 0xffea0001, 0xff0042d4, // 5
    0xffb9bb8c, 0xff1c237e, 0xffc80000, 0xff0000a8, 0xffc9009a, // 10
    0xffd1c000, 0xffe800e8, 0xffd95e4c, 0xff003e4b, 0xffe76d00, // 15
    0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, // 20
    0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, // 25
    0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, // 30
    0xffe800e8, 0xffe800e8, 0xffe800e8, 0xff090909, 0xff121212, // 35
    0xff1b1b1b, 0xff242424, 0xff292929, 0xff2e2e2e, 0xff323232, // 40
    0xff373737, 0xff3b3b3b, 0xff404040, 0xff454545, 0xff494949, // 45
    0xff525252, 0xff5c5c5c, 0xff656565, 0xff6e6e6e, 0xff757575, // 50
    0xff777777, 0xff7c7c7c, 0xff818181, 0xff858585, 0xff8a8a8a, // 55
    0xff8e8e8e, 0xff939393, 0xff989898, 0xff9c9c9c, 0xffa1a1a1, // 60
    0xffa5a5a5, 0xffaaaaaa, 0xffafafaf, 0xffb3b3b3, 0xffb8b8b8, // 65
    0xffbcbcbc, 0xffc1c1c1, 0xffc6c6c6, 0xffcacaca, 0xffcfcfcf, // 70
    0xffd3d3d3, 0xffd8d8d8, 0xffdddddd, 0xffe1e1e1, 0xffe6e6e6  // 75
};

uint32_t indexed2rgb(uint8_t color)
{
    if (color < RGB_LUT_SIZE)
    {
        return indexed2rgbLUT[color];
    }
    else
    {
        // return gray so it's probably visible
        return indexed2rgbLUT[4];
    }
}
#endif

struct gui_main_struct {
  void              *obj;               // off_0x00;
  uint32_t          counter_550d;
  uint32_t          off_0x08;
  uint32_t          counter;            // off_0x0c;
  uint32_t          off_0x10;
  uint32_t          off_0x14;
  struct msg_queue  *msg_queue_m50;     // off_0x18;
  struct msg_queue  *msg_queue_eosr;    // off_0x1c;
  uint32_t          off_0x20;
  uint32_t          off_0x24;
  uint32_t          off_0x28;
  uint32_t          off_0x2c;
  struct msg_queue  *msg_queue;         // off_0x30;
  struct msg_queue  *off_0x34;          // off_0x34;
  struct msg_queue  *msg_queue_550d;    // off_0x38;
  uint32_t          off_0x3c;
};

extern struct gui_main_struct gui_main_struct;

#define TASK_OVERRIDE( orig_func, replace_func ) \
extern void orig_func( void ); \
__attribute__((section(".task_overrides"))) \
struct task_mapping task_mapping_##replace_func = { \
        .orig           = orig_func, \
        .replacement    = replace_func, \
}

extern int vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
void gui_printf_auto(const char *fmt, ...)
{
    va_list ap;
    static int x = 120;
    static int y = 0;

    if (y > 420)
    {
        if (x == 400)
            x = 160;
        else
            x = 400;
        y = 30;
    }
    else
    {
        y += 30;
    }

    char buf[128];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf)-1, fmt, ap);
    msleep(20); // wait for canon menu to draw so we don't get overwritten (as much)
    font_draw(x, y, FONT_SMALL, 2, buf);
    #ifdef FEATURE_VRAM_RGBA
    refresh_yuv_from_rgb();
    #endif
    va_end(ap);
}

void ml_gui_main_task()
{
    struct event *event = NULL;
    int index = 0;
    void *funcs[GMT_NFUNCS];

    memcpy(funcs, (void*)GMT_FUNCTABLE, 4*GMT_NFUNCS);

    gui_init_end();

    while(1)
    {
        #if defined(CONFIG_550D) || defined(CONFIG_7D)
        msg_queue_receive(gui_main_struct.msg_queue_550d, &event, 0);
        gui_main_struct.counter_550d--;
        #elif defined(CONFIG_R) || defined(CONFIG_RP) || defined(CONFIG_850D)
        msg_queue_receive(gui_main_struct.msg_queue_eosr, &event, 0);
        gui_main_struct.counter_550d--;
        #elif defined(CONFIG_M50)
        msg_queue_receive(gui_main_struct.msg_queue_m50, &event, 0);
        gui_main_struct.counter_550d--;
        #else
        msg_queue_receive(gui_main_struct.msg_queue, &event, 0);
        gui_main_struct.counter--;
        #endif

        if (event == NULL) {
            continue;
        }

        index = event->type;

        if (event->type == 0)
        {
            DryosDebugMsg(0, 15, "event->param 0x%x", event->param);
            gui_printf_auto("e->p 0x%x", event->param);
        }

        if (IS_FAKE(event)) {
           event->arg = 0;      /* do not pass the "fake" flag to Canon code */
        }

        if (event->type == 0 && event->param < 0) {
            continue;           /* do not pass internal ML events to Canon code */
        }

        if ((index >= GMT_NFUNCS) || (index < 0)) {
            continue;
        }

        void(*f)(struct event *) = funcs[index];
        if (f != NULL)
            f(event);
    }
}

TASK_OVERRIDE(gui_main_task, ml_gui_main_task);

// Some utility functions
static void led_blink(int times, int delay_on, int delay_off)
{
    for (int i = 0; i < times; i++)
    {
        MEM(CARD_LED_ADDRESS) = LEDON;
        msleep(delay_on);
        MEM(CARD_LED_ADDRESS) = LEDOFF;
        msleep(delay_off);
    }
}

int ml_started = 0; // 1 after ML is fully loaded
static void
my_task_dispatch_hook(
        struct context **p_context_old,    /* on new DryOS (6D+), this argument is different (small number, unknown meaning) */
        struct task *prev_task_unused,     /* only present on new DryOS */
        struct task *next_task_new         /* only present on new DryOS; old versions use HIJACK_TASK_ADDR */
)
{
    struct task * next_task = 
        #ifdef CONFIG_NEW_DRYOS_TASK_HOOKS
        next_task_new;
        #else
        *(struct task **)(HIJACK_TASK_ADDR);
        #endif

    if (!next_task)
        return;

#ifdef CONFIG_NEW_DRYOS_TASK_HOOKS
    /* on new DryOS, first argument is not context; get it from the task structure */
    /* this also works for some models with old-style DryOS, but not all */
    struct context *context = next_task->context;
#else
    /* on old DryOS, context is passed as argument
     * on some models (not all!), it can be found in the task structure as well */
    struct context *context = p_context_old ? (*p_context_old) : 0;
#endif
    
    if (!context)
        return;
    
    if (ml_started)
    {
        /* all task overrides should be done by now */
        return;
    }

    // Do nothing unless a new task is starting via the trampoile
    if(context->pc != (uint32_t)task_trampoline)
        return;

    thunk entry = (thunk) next_task->entry;

    qprintf("[****] Starting task %x(%x) %s\n", next_task->entry, next_task->arg, next_task->name);

    // Search the task_mappings array for a matching entry point
    extern struct task_mapping _task_overrides_start[];
    extern struct task_mapping _task_overrides_end[];
    struct task_mapping *mapping = _task_overrides_start;

    for( ; mapping < _task_overrides_end ; mapping++ )
    {
#if defined(POSITION_INDEPENDENT)
        mapping->replacement = PIC_RESOLVE(mapping->replacement);
#endif
        thunk original_entry = mapping->orig;
        if( original_entry != entry )
            continue;

/* -- can't call debugmsg from this context */
        qprintf("[****] Replacing task %x with %x\n",
            original_entry,
            mapping->replacement
        );

        next_task->entry = mapping->replacement;
        break;
    }
}

// called before Canon's init_task
void boot_pre_init_task(void)
{
    // Install our task creation hooks
    qprint("[BOOT] installing task dispatch hook at "); qprintn((int)&task_dispatch_hook); qprint("\n");
    DryosDebugMsg(0, 15, "replacing task_dispatch_hook");
    task_dispatch_hook = my_task_dispatch_hook;
}

// called right after Canon's init_task, while their initialization continues in background
#ifdef FEATURE_VRAM_RGBA
extern void* _malloc(size_t size); // for real ML, malloc is preferred, which may wrap
                                   // the function with one with more logging.  That's not
                                   // always available so we use the underlying malloc
                                   // in this simple test code.
#endif
void boot_post_init_task(void)
{
#ifdef FEATURE_VRAM_RGBA
    bmp_vram_indexed = _malloc(BMP_VRAM_SIZE);
    if (bmp_vram_indexed == NULL)
    { // can't display anything, blink led to indicate sadness
        while(1)
        {
            MEM(CARD_LED_ADDRESS) = LEDON;
            msleep(150);
            MEM(CARD_LED_ADDRESS) = LEDOFF;
            msleep(150);
        }
    }
    // initialise to transparent, this allows us to draw over
    // existing screen, rather than replace it, due to checks
    // in refresh_yuv_from_rgb()
    memset(bmp_vram_indexed, COLOR_TRANSPARENT_BLACK, BMP_VRAM_SIZE);
//    memset(bmp_vram_indexed, 50, BMP_VRAM_SIZE);
#endif
    //task_create("run_test", 0x1e, 0x4000, hello_world, 0);
    msleep(2000); // wait for OS to initialise
    ml_started = 1; // mark ML as loaded, probably not necessary,
                    // stops task hooking doing unnecessary lookups
}

// used by font_draw
void disp_set_pixel(int x, int y, int c)
{
#ifdef FEATURE_VRAM_RGBA
    bmp_vram_indexed[x + y * BMPPITCH] = c;
#else
    uint8_t *bmp = _bmp_vram_raw();
    bmp[x + y * BMPPITCH] = c;
#endif
}
