#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <console.h>
#include <shoot.h>
#include <beep.h>
#include <edmac.h>
#include <timer.h>
#include <asm.h>

static int edmac_selection;

static void edmac_display_page(int i0, int x0, int y0)
{
    bmp_printf(
        FONT_MONO_20,
        x0, y0,
        "EDM# Address  Size\n"
    );

    y0 += fontspec_font(FONT_MONO_20)->height * 2;

    for (int i = 0; i < 16; i++)
    {
        char msg[100];
        int ch = i0 + i;

        uint32_t addr = edmac_get_address(ch);

        int state = edmac_get_state(ch);

        struct edmac_info info = edmac_get_info(ch);
        char * sz = edmac_format_size(&info);
        if (strlen(sz) <= 10)
        {
            snprintf(msg, sizeof(msg), "[%2d] %8x: %s", ch, addr, sz);
        }
        else
        {
            snprintf(msg, sizeof(msg), "[%2d] %8x: %d", ch, addr, edmac_get_total_size(&info, 0));
        }

        if (state != 0 && state != 1)
        {
            STR_APPEND(msg, " (%x)", state);
        }

        uint32_t dir     = edmac_get_dir(ch);
        uint32_t conn_w  = edmac_get_connection(ch, EDMAC_DIR_WRITE);
        uint32_t conn_r  = edmac_get_connection(ch, EDMAC_DIR_READ);

        int color =
            dir == EDMAC_DIR_UNUSED ? COLOR_GRAY(20) :   /* unused? */
            state == 0              ? COLOR_GRAY(50) :   /* inactive? */
            state == 1              ? COLOR_GREEN1   :   /* active? */
                                      COLOR_RED      ;   /* no idea */

        if (dir == EDMAC_DIR_WRITE)
        {
            if (conn_w == 0)
            {
                /* Write EDMAC, but could not figure out where it's connected */
                /* May be either unused, or connected to 0 (RAW data) */
                STR_APPEND(msg, " <w!>");
            }
            else
            {
                STR_APPEND(msg, " <w%x>", conn_w);
            }
        }
        else if (dir == EDMAC_DIR_READ)
        {
            if (conn_r == 0xFF)
            {
                /* Read EDMAC, but could not figure out where it's connected */
                STR_APPEND(msg, " <r!>");
            }
            else
            {
                STR_APPEND(msg, " <r%x>", conn_r);
            }
        }

        if (dir != EDMAC_DIR_UNUSED && strchr(msg, '!'))
        {
            color = COLOR_YELLOW;
        }


        bmp_printf(
            FONT(FONT_MONO_20, color, COLOR_BLACK),
            x0, y0 + i * fontspec_font(FONT_MONO_20)->height,
            msg
        );
    }
}

static void edmac_display_detailed(int channel)
{
    uint32_t base = edmac_get_base(channel);

    int x = 50;
    int y = 50;
    bmp_printf(
        FONT_LARGE,
        x, y,
        "EDMAC #%d - %x\n",
        channel,
        base
    );
    y += font_large.height;

    /* http://magiclantern.wikia.com/wiki/Register_Map#EDMAC */

    uint32_t state = edmac_get_state(channel);
    uint32_t flags = edmac_get_flags(channel);
    uint32_t addr  = edmac_get_address(channel);

    union edmac_size_t
    {
        struct { short x, y; } size;
        uint32_t raw;
    };

    union edmac_size_t size_n = (union edmac_size_t) shamem_read(base + 0x0C);
    union edmac_size_t size_b = (union edmac_size_t) shamem_read(base + 0x10);
    union edmac_size_t size_a = (union edmac_size_t) shamem_read(base + 0x14);

    uint32_t off1b = shamem_read(base + 0x18);
    uint32_t off2b = shamem_read(base + 0x1C);
    uint32_t off1a = shamem_read(base + 0x20);
    uint32_t off2a = shamem_read(base + 0x24);
    uint32_t off3  = shamem_read(base + 0x28);
    uint32_t off40 = shamem_read(base + 0x40);

    uint32_t dir     = edmac_get_dir(channel);
    char* dir_s      = 
        dir == EDMAC_DIR_READ  ? "read"  :
        dir == EDMAC_DIR_WRITE ? "write" :
                                 "unused?";
    
    uint32_t conn_w  = edmac_get_connection(channel, EDMAC_DIR_WRITE);
    uint32_t conn_r  = edmac_get_connection(channel, EDMAC_DIR_READ);

    struct edmac_info info = edmac_get_info(channel);
    
    int fh = fontspec_font(FONT_MONO_20)->height;

    bmp_printf(FONT_MONO_20, 50, y += fh, "Address    : %8x ", addr);
    bmp_printf(FONT_MONO_20, 50, y += fh, "State      : %8x ", state);
    bmp_printf(FONT_MONO_20, 50, y += fh, "Flags      : %8x ", flags);
    bmp_printf(FONT_MONO_20, 50, y += fh, "Size       : %s ", edmac_format_size(&info));
    y += fh;
    bmp_printf(FONT_MONO_20, 50, y += fh, "Size A     : %8x (%d x %d) ", size_a.raw, size_a.size.x, size_a.size.y);
    bmp_printf(FONT_MONO_20, 50, y += fh, "Size B     : %8x (%d x %d) ", size_b.raw, size_b.size.x, size_b.size.y);
    bmp_printf(FONT_MONO_20, 50, y += fh, "Size N     : %8x (%d x %d) ", size_n.raw, size_n.size.x, size_n.size.y);
    bmp_printf(FONT_MONO_20, 50, y += fh, "off1a      : %8x ", off1a);
    bmp_printf(FONT_MONO_20, 50, y += fh, "off1b      : %8x ", off1b);
    bmp_printf(FONT_MONO_20, 50, y += fh, "off2a      : %8x ", off2a);
    bmp_printf(FONT_MONO_20, 50, y += fh, "off2b      : %8x ", off2b);
    bmp_printf(FONT_MONO_20, 50, y += fh, "off3       : %8x ", off3);
    bmp_printf(FONT_MONO_20, 50, y += fh, "off40      : %8x ", off40);
    y += fh;
    bmp_printf(FONT_MONO_20, 50, y += fh, "Connection : write=0x%x read=0x%x dir=%s", conn_w, conn_r, dir_s);

    #if defined(CONFIG_5D3)
    /**
     * ConnectReadEDmac(channel, conn)
     * RAM:edmac_register_interrupt(channel, cbr_handler, ...)
     * => *(8 + 32*arg0 + *0x12400) = arg1
     * and also: *(12 + 32*arg0 + *0x12400) = arg1
     */
    uint32_t cbr1 = MEM(8 + 32*(channel) + MEM(0x12400));
    uint32_t cbr2 = MEM(12 + 32*(channel) + MEM(0x12400));
    bmp_printf(FONT_MONO_20, 50, y += fh, "CBR handler: %8x %s", cbr1, asm_guess_func_name_from_string(cbr1));
    bmp_printf(FONT_MONO_20, 50, y += fh, "CBR abort  : %8x %s", cbr2, asm_guess_func_name_from_string(cbr2));
    #endif
}

static MENU_UPDATE_FUNC(edmac_display)
{
    if (!info->can_custom_draw) return;
    info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
    bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

    if (edmac_selection == 0 || edmac_selection == 1) // overview
    {
        if (edmac_selection == 0)
        {
            edmac_display_page(0, 0, 30);
            edmac_display_page(16, 360, 30);
        }
        else
        {
            edmac_display_page(16, 0, 30);
            #ifdef CONFIG_DIGIC_V
            edmac_display_page(32, 360, 30);
            #endif
        }

        //~ int x = 20;
        bmp_printf(
            FONT_MONO_20,
            20, 450, "EDMAC state: "
        );

        bmp_printf(
            FONT(FONT_MONO_20, COLOR_GRAY(50), COLOR_BLACK),
            20+200, 450, "inactive"
        );

        bmp_printf(
            FONT(FONT_MONO_20, COLOR_GREEN1, COLOR_BLACK),
            20+350, 450, "running"
        );

        bmp_printf(
            FONT_MONO_20,
            720 - fontspec_font(FONT_MONO_20)->width * 13, 450, "[Scrollwheel]"
        );
    }
    else // detailed view
    {
        edmac_display_detailed(edmac_selection - 2);
    }
}


/* attempt to detect available edmac channels */
static volatile int trying_to_lock = 0;
static volatile int timed_out = 0;

static void check_timeout()
{
    timed_out = 1;

    if (lv && trying_to_lock)
    {
        int key_play = module_translate_key(MODULE_KEY_PLAY, MODULE_KEY_CANON);
        fake_simple_button(key_play);
    }
}

static int try_lock_edmac_channel(uint32_t resource)
{
    uint32_t res[] = { resource };
    struct LockEntry * resLock = CreateResLockEntry(res, 1);

    /* the Lock call will block if the resource is used by someone else */
    /* to get past it, try to enter PLAY mode after a timeout */
    delayed_call(1000, check_timeout, 0);
    timed_out = 0;
    trying_to_lock = 1;
    LockEngineResources(resLock);
    trying_to_lock = 0;
    UnLockEngineResources(resLock);
    int success = !timed_out;

    while (!timed_out)
        msleep(10);
    
    return success;
}

static void find_free_edmac_channels()
{
    console_show();
    msleep(2000);
    
    for (int ch = 0; ch < 32; ch++)
    {
        force_liveview();
        wait_lv_frames(5);

        int dir = edmac_get_dir(ch);
        int i = edmac_channel_to_index(ch);
        uint32_t resource = 0;
        
        switch (dir)
        {
            case EDMAC_DIR_WRITE:
            {
                printf("Trying write channel #%d... ", ch);
                resource = 0x00000000 + i; /* write edmac channel */
                break;
            }
            case EDMAC_DIR_READ:
            {
                printf("Trying read channel #%d... ", ch);
                resource = 0x00010000 + i; /* read edmac channel */
                break;
            }
            default:
            {
                printf("Skipping unused channel #%d...\n", ch);
                continue;
            }
        }

        if (try_lock_edmac_channel(resource))
        {
            printf("seems to work!\n");
        }
        else
        {
            printf("\n");
        }
    }
}

/* log EDMAC state every X microseconds */
static const int LOG_INTERVAL = 100;

/* a little faster when hardcoded */
/* should match edmac_chanlist from src/edmac.c */
static const int edmac_regs[] = {
    0xC0F04000, 0xC0F04100, 0xC0F04200, 0xC0F04300, 0xC0F04400, 0xC0F04500, 0xC0F04600,
    0xC0F04800, 0xC0F04900, 0xC0F04A00, 0xC0F04B00, 0xC0F04C00, 0xC0F04D00,
    0xC0F26000, 0xC0F26100, 0xC0F26200, 0xC0F26300, 0xC0F26400, 0xC0F26500, 0xC0F26600,
    0xC0F26800, 0xC0F26900, 0xC0F26A00, 0xC0F26B00, 0xC0F26C00, 0xC0F26D00,
    0xC0F30000, 0xC0F30100,
    0xC0F30800, 0xC0F30900, 0xC0F30A00, 0xC0F30B00,
};

static uint32_t edmac_states[2048][COUNT(edmac_regs) + 4] = {{1}};
static uint32_t edmac_index = 0;

#define CLK_IDX (COUNT(edmac_regs))
#define TSK_IDX (COUNT(edmac_regs)+1)
#define OVH_IDX (COUNT(edmac_regs)+2)
#define XTR_IDX (COUNT(edmac_regs)+3)

struct edmac_extra_info
{
    uint32_t ch;
    uint32_t addr;
    uint32_t conn;
    uint32_t cbr;
    struct edmac_info info;
};

static struct edmac_extra_info edmac_extra_infos[512];
static int edmac_extra_index = 0;

static void FAST edmac_spy_poll(int last_expiry, void* unused)
{
    uint32_t start_clock = MEM(0xC0242014);

    if (edmac_index >= COUNT(edmac_states))
    {
        /* finished */
        return;
    }
    
    /* schedule next call */
    SetHPTimerNextTick(last_expiry, LOG_INTERVAL, edmac_spy_poll, edmac_spy_poll, 0);

    /* this routine requires LCLK enabled */
    if (!(MEM(0xC0400008) & 0x2))
    {
        return;
    }

    /* log events starting with the first meaningful change */
    static int started = 0;

    for (uint32_t i = 0; i < COUNT(edmac_regs); i++)
    {
        /* edmac_get_state/pointer(channel), just faster */
        uint32_t state = MEM(edmac_regs[i]);
        uint32_t ptr   = MEM(edmac_regs[i] + 8);
        ASSERT((state & ~1) == 0);
        edmac_states[edmac_index][i] = ptr | (state << 31);
        
        uint32_t prev_state = (edmac_index)
            ? (edmac_states[edmac_index-1][i] & 0x80000000)
            : 0;
        
        if (state || ptr)
        {
            started = 1;
        }

        if (state && !prev_state)
        {
            /* when a channel is starting, record some extra info,
             * such as connection and geometry
             * (this is slow, so we don't record it continuously)
             */

            if (edmac_extra_index >= COUNT(edmac_extra_infos))
            {
                /* buffer full */
                return;
            }

            uint32_t ch = edmac_get_channel(edmac_regs[i]);
            edmac_extra_infos[edmac_extra_index] = (struct edmac_extra_info) {
                .ch     = ch,
                .addr   = edmac_get_address(ch),
                .conn   = edmac_get_connection(ch, edmac_get_dir(ch)),
                .info   = edmac_get_info(ch),
                .cbr    = MEM(8 + 32*(ch) + MEM(0x12400)),  /* 5D3 only */
            };

            edmac_extra_index++;
        }
    }

    /* also store:
     * - microsecond timer
     * - current task name (pointer)
     * - estimated overhead for this routine (microseconds)
     * - extra index (for infos that are not logged continuously)
     *   (if it was 5 at edmax_index-1 and now it's 7,
     *    this step stored extra infos at indices 5 and 6)
     */
    edmac_states[edmac_index][CLK_IDX] = start_clock;
    edmac_states[edmac_index][TSK_IDX] = (uint32_t) current_task->name;
    edmac_states[edmac_index][OVH_IDX] = MEM(0xC0242014) - start_clock;
    edmac_states[edmac_index][XTR_IDX] = edmac_extra_index;

    if (started)
    {
        edmac_index++;
    }
}

static void edmac_spy_dump()
{
    int len = 0;
    int maxlen = 8*1024*1024;
    char* out = fio_malloc(maxlen);
    if (!out) return;
    memset(out, ' ', maxlen);
    
    uint32_t extra_index = 0;

    for (uint32_t i = 0; i < edmac_index; i++)
    {
        for (; extra_index < edmac_states[i][XTR_IDX]; extra_index++)
        {
            struct edmac_extra_info info = edmac_extra_infos[extra_index];
            len += snprintf(out+len, maxlen-len,
                "EDMAC#%d: addr=0x%x conn=%d cbr=%x name='''%s''' size='''%s'''\n",
                info.ch, info.addr, info.conn, info.cbr,
                asm_guess_func_name_from_string(info.cbr),
                edmac_format_size(&info.info)
            );
        }

        snprintf(out+len, maxlen-len, "%08X %s          ",
            edmac_states[i][CLK_IDX], edmac_states[i][TSK_IDX]
        );
        len += 16;

        for (int j = 0; j < COUNT(edmac_regs); j++)
        {
            int ch = edmac_get_channel(edmac_regs[j]);
            len += snprintf(out+len, maxlen-len, " %d:%08X", ch, edmac_states[i][j]);
        }

        len += snprintf(out+len, maxlen-len, " OVH:%d\n", edmac_states[i][OVH_IDX]);
    }

    len += snprintf(out+len, maxlen-len, "\nSaved %d events, %d extra infos.\n", edmac_index, edmac_extra_index);

    dump_seg(out, len, "edmacspy.log");
    free(out);
    
    edmac_index = 0;
    edmac_extra_index = 0;
}

static void log_edmac_usage()
{
    SetHPTimerAfterNow(1000, edmac_spy_poll, edmac_spy_poll, 0);
    NotifyBox(10000, "Logging EDMAC usage...");
    
    /* wait until buffer full */
    while (edmac_index < COUNT(edmac_states))
    {
        info_led_on();
        msleep(100);
    }
    info_led_off();

    NotifyBox(2000, "Saving log...");
    edmac_spy_dump();
    NotifyBox(2000, "Finished!");
}

/* edmac_test.c */
extern void edmac_test();

static struct menu_entry edmac_menu[] =
{
    {
        .name   = "EDMAC tools",
        .select = menu_open_submenu,
        .children =  (struct menu_entry[]) {
            {
                .name       = "Show EDMAC channels",
                .select     = menu_open_submenu,
                .icon_type  = IT_ACTION,
                .help       = "Useful for finding image buffers.",
                .children =  (struct menu_entry[]) {
                    {
                        .name   = "EDMAC display",
                        .priv   = &edmac_selection,
                        .max    = 49,
                        .update = edmac_display,
                    },
                    MENU_EOL
                },
            },
            {
                .name   = "Find free EDMAC channels",
                .select = run_in_separate_task,
                .priv   = find_free_edmac_channels,
                .help   = "Useful to find which channels can be used in LiveView.\n",
            },
            {
                .name   = "Log EDMAC usage",
                .select = run_in_separate_task,
                .priv   = log_edmac_usage,
                .help   = "Log EDMAC status changes every 0.1ms.",
            },
            {
                .name   = "EDMAC model test",
                .select = run_in_separate_task,
                .priv   = edmac_test,
                .help   = "Tests to confirm our hypothesis on how EDMAC works.",
                .help2  = "Should be executed on both camera and QEMU."
            },
            MENU_EOL
        }
    },
};

static unsigned int edmac_init()
{
    menu_add("Debug", edmac_menu, COUNT(edmac_menu));
    return 0;
}

static unsigned int edmac_deinit()
{
    menu_remove("Debug", edmac_menu, COUNT(edmac_menu));
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(edmac_init)
    MODULE_DEINIT(edmac_deinit)
MODULE_INFO_END()
