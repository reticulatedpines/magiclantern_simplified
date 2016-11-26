#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <console.h>
#include <edmac.h>

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
    uint32_t addr = edmac_get_address(channel);

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


static struct menu_entry edmac_menu[] =
{
    {
        .name = "Show EDMAC",
        .select = menu_open_submenu,
        .help = "Useful for finding image buffers.",
        .children =  (struct menu_entry[]) {
            {
                .name = "EDMAC display",
                .priv = &edmac_selection,
                .max = 49,
                .update = edmac_display,
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
