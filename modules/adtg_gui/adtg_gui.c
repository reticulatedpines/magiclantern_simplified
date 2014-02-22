/**
 * ADTG register editing GUI
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>

#include <gdb.h>
#include <cache_hacks.h>

#define DST_CMOS16  0xFF000000
#define DST_CMOS    0x00FF0000
#define DST_ADTG    0x000000FF      /* any ADTG */
#define DST_ANY     0xFFFFFFFF

struct known_reg
{
    uint32_t dst;
    uint16_t reg;
    uint16_t is_nrzi;   /* this will override the default guess */
    char* description;
};

static struct known_reg known_regs[] = {
    {DST_CMOS,      0, 0, "Analog ISO (most cameras)"},
    {DST_CMOS,      1, 0, "Vertical offset"},
    {DST_CMOS,      2, 0, "Horizontal offset / column skipping"},
    {DST_CMOS,      3, 0, "Analog ISO on 6D"},
    {DST_CMOS,      5, 0, "Fine vertical offset, black area maybe"},
    {DST_CMOS,      6, 0, "ISO 50 or timing related: FFF => darker image"},
    {DST_CMOS,      7, 0, "Looks like the cmos is dieing (g3gg0)"},
    {DST_ADTG, 0x8000, 0, "Causes interlacing (g3gg0)"},
    {DST_ADTG, 0x800C, 0, "Line skipping factor (2 = 1080p, 4 = 720p, 0 = zoom)"},
    {DST_ADTG, 0x805E, 1, "Shutter blanking for x5/x10 zoom"},
    {DST_ADTG, 0x8060, 1, "Shutter blanking for LiveView 1x"},
    {DST_ADTG, 0x8172, 1, "Line count to sample. same as video resolution (g3gg0)"},
    {DST_ADTG, 0x8178, 1, "dwSrFstAdtg1[4], Line count + 1"},
    {DST_ADTG, 0x8179, 1, "dwSrFstAdtg1[5]"},
    {DST_ADTG, 0x8196, 1, "dwSrFstAdtg1[2], Line count + 1"},
    {DST_ADTG, 0x8197, 1, "dwSrFstAdtg1[3]"},
    {DST_ADTG, 0x82F3, 1, "Line count that gets darker (top optical black related)"},
    {DST_ADTG, 0x82F8, 1, "Line count"},
    {DST_ADTG, 0x8830, 0, "Only slightly changes the color of the image (g3gg0)"},
    {DST_ADTG, 0x8880, 0, "Some weird black value (value 0x0800 means normal) (g3gg0)"},
    {DST_ADTG, 0x8882, 0, "Digital gain (per column maybe)"}, /* I believe it's digital, not 100% sure */
    {DST_ADTG, 0x8884, 0, "Digital gain (per column maybe)"},
    {DST_ADTG, 0x8886, 0, "Digital gain (per column maybe)"},
    {DST_ADTG, 0x8888, 0, "Digital gain (per column maybe)"},

    {DST_ADTG, 0x105F, 1, "Shutter blanking for x5/x10 zoom"},
    {DST_ADTG, 0x106E, 1, "Shutter blanking for LiveView 1x"},

    {DST_ADTG,   0x14, 1, "ISO related"},
    {DST_ADTG,   0x15, 1, "ISO related"},
};

static int adtg_enabled = 0;
static int edit_multiplier = 0;
static int show_what = 0;

#define SHOW_ALL 0
#define SHOW_KNOWN_ONLY 1
#define SHOW_MODIFIED 2

static uint32_t ADTG_WRITE_FUNC = 0;
static uint32_t CMOS_WRITE_FUNC = 0;
static uint32_t CMOS2_WRITE_FUNC = 0;
static uint32_t CMOS16_WRITE_FUNC = 0;

struct reg_entry
{
    uint32_t dst;
    void* addr;
    uint16_t reg;
    uint16_t val;
    uint16_t prev_val;
    int override;
    unsigned is_nrzi:1;
};

static struct reg_entry regs[512] = {{0}};
static int reg_num = 0;

static int known_match(int i, int reg)
{
    return
        (
            (known_regs[i].dst == regs[reg].dst) || 
            (known_regs[i].dst == DST_ADTG && regs[reg].dst == (regs[reg].dst & 0xF)) || 
            (known_regs[i].dst == DST_ANY)
        )
        &&
        (
            (known_regs[i].reg == regs[reg].reg)
        )
    ;
}

static uint32_t nrzi_decode( uint32_t in_val )
{
    uint32_t val = 0;
    if (in_val & 0x8000)
        val |= 0x8000;
    for (int num = 0; num < 31; num++)
    {
        uint32_t old_bit = (val & 1<<(30-num+1)) >> 1;
        val |= old_bit ^ (in_val & 1<<(30-num));
    }
    return val;
}

static uint32_t nrzi_encode( uint32_t in_val )
{
    uint32_t out_val = 0;
    uint32_t old_bit = 0;
    for (int num = 0; num < 32; num++)
    {
        uint32_t bit = in_val & 1<<(30-num) ? 1 : 0;
        if (bit != old_bit)
            out_val |= (1 << (30-num));
        old_bit = bit;
    }
    return out_val;
}

static void reg_update_unique(uint32_t dst, void* addr, uint32_t data, uint32_t reg_shift, uint32_t is_nrzi)
{
    uint32_t reg = data >> reg_shift;
    uint32_t val = data & ((1 << reg_shift) - 1);
    struct reg_entry * re = 0;
    
    for (int i = 0; i < reg_num; i++)
    {
        re = &regs[i];
        
        if (re->dst == dst && re->reg == reg)
        {
            /* overwrite existing entry */
            goto found;
        }
    }
    
    /* new entry */
    re = &regs[reg_num];
    re->dst = dst;
    re->reg = reg;
    re->override = INT_MIN;
    re->is_nrzi = is_nrzi; /* initial guess; may be overriden */
    re->prev_val = val;

    if (reg_num + 1 < COUNT(regs))
        reg_num++;

    /* fill the data */
found:

    if (re->override != INT_MIN)
    {
        int ovr = re->is_nrzi ? (int)nrzi_encode(re->override) : re->override;
        uint16_t* val_ptr = addr;
        ovr &= ((1 << reg_shift) - 1);
        *val_ptr &= ~((1 << reg_shift) - 1);
        *val_ptr |= ovr;
    }

    re->addr = addr;
    re->val = val;
}

static void adtg_log(breakpoint_t *bkpt)
{
    unsigned int cs = bkpt->ctx[0];
    unsigned int *data_buf = (unsigned int *) bkpt->ctx[1];
    int dst = cs & 0xF;
    
    /* log all ADTG writes */
    while(*data_buf != 0xFFFFFFFF)
    {
        /* ADTG4 registers seem to use NRZI */
        reg_update_unique(dst, data_buf, *data_buf, 16, dst == 4);
        data_buf++;
    }
}

static void cmos_log(breakpoint_t *bkpt)
{
    unsigned short *data_buf = (unsigned short *) bkpt->ctx[0];
    
    /* log all CMOS writes */
    while(*data_buf != 0xFFFF)
    {
        reg_update_unique(DST_CMOS, data_buf, *data_buf, 12, 0);
        data_buf++;
    }
}

static void cmos16_log(breakpoint_t *bkpt)
{
    unsigned short *data_buf = (unsigned short *) bkpt->ctx[0];
    
    /* log all CMOS writes */
    while(*data_buf != 0xFFFF)
    {
        reg_update_unique(DST_CMOS16, data_buf, *data_buf, 12, 0);
        data_buf++;
    }
}

static MENU_SELECT_FUNC(adtg_toggle)
{
    adtg_enabled = !adtg_enabled;
    
    static breakpoint_t * bkpt1 = 0;
    static breakpoint_t * bkpt2 = 0;
    static breakpoint_t * bkpt3 = 0;
    static breakpoint_t * bkpt4 = 0;
    
    if (adtg_enabled)
    {
        /* set watchpoints at ADTG and CMOS writes */
        gdb_setup();
        if (ADTG_WRITE_FUNC)   bkpt1 = gdb_add_watchpoint(ADTG_WRITE_FUNC, 0, &adtg_log);
        if (CMOS_WRITE_FUNC)   bkpt2 = gdb_add_watchpoint(CMOS_WRITE_FUNC, 0, &cmos_log);
        if (CMOS2_WRITE_FUNC)  bkpt3 = gdb_add_watchpoint(CMOS2_WRITE_FUNC, 0, &cmos_log);
        if (CMOS16_WRITE_FUNC) bkpt4 = gdb_add_watchpoint(CMOS16_WRITE_FUNC, 0, &cmos16_log);
    }
    else
    {
        /* uninstall watchpoints */
        if (bkpt1) gdb_delete_bkpt(bkpt1);
        if (bkpt2) gdb_delete_bkpt(bkpt2);
        if (bkpt3) gdb_delete_bkpt(bkpt3);
        if (bkpt4) gdb_delete_bkpt(bkpt4);
    }
}

static MENU_UPDATE_FUNC(reg_update)
{
    int reg = (int) entry->priv;
    if (reg < 0 || reg >= COUNT(regs))
        return;
    
    char dst_name[10];
    if (regs[reg].dst == DST_CMOS)
        snprintf(dst_name, sizeof(dst_name), "CMOS");
    else if (regs[reg].dst == DST_CMOS16)
        snprintf(dst_name, sizeof(dst_name), "CMOS16");
    else
        snprintf(dst_name, sizeof(dst_name), "ADTG%d", regs[reg].dst);

    for (int i = 0; i < COUNT(known_regs); i++)
    {
        if (known_match(i, reg))
        {
            regs[reg].is_nrzi = known_regs[i].is_nrzi;
        }
    }

    MENU_SET_NAME("%s[%x]%s", dst_name, regs[reg].reg, regs[reg].is_nrzi ? " N" : "");
    
    if (show_what == SHOW_MODIFIED && regs[reg].override == INT_MIN)
    {
        MENU_SET_VALUE(
            "0x%x (was 0x%x)",
            regs[reg].is_nrzi ? nrzi_decode(regs[reg].val) : regs[reg].val,
            regs[reg].is_nrzi ? nrzi_decode(regs[reg].prev_val) : regs[reg].prev_val
        );
    }
    else
    {
        MENU_SET_VALUE(
            "0x%x",
            regs[reg].is_nrzi ? nrzi_decode(regs[reg].val) : regs[reg].val
        );
    }
    
    MENU_SET_HELP("Addr=%x, value=%d (0x%x), nrzi=%d (0x%x).", regs[reg].addr, regs[reg].val, regs[reg].val, nrzi_decode(regs[reg].val), nrzi_decode(regs[reg].val));
    
    if (reg_num >= COUNT(regs)-1)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Too many registers.");

    MENU_SET_ICON(MNI_BOOL(regs[reg].override != INT_MIN), 0);

    if (regs[reg].override != INT_MIN)
    {
        MENU_SET_RINFO("-> 0x%x", regs[reg].override);
        if (menu_active_and_not_hidden())
            MENU_SET_WARNING(MENU_WARN_INFO, "Press Q to stop overriding this register.");
    }
    else
    {
        for (int i = 0; i < COUNT(known_regs); i++)
        {
            if (known_match(i, reg))
            {
                MENU_SET_WARNING(MENU_WARN_INFO, "%s.", known_regs[i].description);
                
                if (show_what != SHOW_MODIFIED) /* do we have enough space to show a shortened description? */
                {
                    char msg[12];
                    snprintf(msg, sizeof(msg), "%s", known_regs[i].description);
                    if (!streq(msg, known_regs[i].description))
                    {
                        int len = COUNT(msg);
                        msg[len-1] = 0;
                        msg[len-2] = '.';
                        msg[len-3] = '.';
                        msg[len-4] = '.';
                    }
                    MENU_SET_RINFO("%s", msg);
                }
            }
        }
    }
}

static MENU_SELECT_FUNC(reg_toggle)
{
    int reg = (int) priv;
    if (reg < 0 || reg >= COUNT(regs))
        return;
    
    if (regs[reg].override == INT_MIN)
        regs[reg].override = regs[reg].is_nrzi ? nrzi_decode(regs[reg].val) : regs[reg].val;
 
    static int multipliers[] = {1, 16, 256, 10, 100, 1000};
    regs[reg].override += delta * multipliers[edit_multiplier];
}

static MENU_SELECT_FUNC(reg_clear_override)
{
    int reg = (int) priv;
    if (reg < 0 || reg >= COUNT(regs))
        return;
    
    if (regs[reg].override != INT_MIN)
        regs[reg].override = INT_MIN;
    else
        menu_close_submenu(); 
}

#define REG_ENTRY(i) \
        { \
            .priv = (void*)i, \
            .select = reg_toggle, \
            .select_Q = reg_clear_override, \
            .update = reg_update, \
            .edit_mode = EM_MANY_VALUES_LV, \
            .shidden = 1, \
        }

static MENU_UPDATE_FUNC(show_update);

static MENU_UPDATE_FUNC(adtg_update)
{
    MENU_SET_ICON(MNI_BOOL(adtg_enabled), 0);
    if (adtg_enabled)
        MENU_SET_WARNING(MENU_WARN_INFO, "ADTG hooks enabled, press PLAY to disable.");
}

static MENU_SELECT_FUNC(adtg_main)
{
    if (delta > 0)
    {
        if (!adtg_enabled)
            adtg_toggle(priv, delta);
        menu_open_submenu();
    }
    else
    {
        if (adtg_enabled)
            adtg_toggle(priv, delta);
    }
}

static struct menu_entry adtg_gui_menu[] =
{
    {
        .name = "ADTG registers",
        .update = adtg_update,
        .select = &adtg_main,
        .icon_type = IT_SUBMENU,
        .help = "Edit ADTG/CMOS register values.",
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Editing step", 
                .priv = &edit_multiplier,
                .max = 5,
                .choices = CHOICES("1", "16 (x << 4)", "256 (x << 8)", "10", "100", "1000"),
                .help = "Step used when editing register values."
            },
            {
                .name = "Show",
                .priv = &show_what,
                .update = show_update,
                .max = 2,
                .choices = CHOICES("Everything", "Known regs only", "Modified regs only"),
                .help2 =    "Everything: show all registers as soon as they are written.\n"
                            "Known: show only the registers with a known description.\n"
                            "Modified: show only regs that have changed their values.\n"
            },
            // for i in range(512): print "            REG_ENTRY(%d)," % i
            REG_ENTRY(0),
            REG_ENTRY(1),
            REG_ENTRY(2),
            REG_ENTRY(3),
            REG_ENTRY(4),
            REG_ENTRY(5),
            REG_ENTRY(6),
            REG_ENTRY(7),
            REG_ENTRY(8),
            REG_ENTRY(9),
            REG_ENTRY(10),
            REG_ENTRY(11),
            REG_ENTRY(12),
            REG_ENTRY(13),
            REG_ENTRY(14),
            REG_ENTRY(15),
            REG_ENTRY(16),
            REG_ENTRY(17),
            REG_ENTRY(18),
            REG_ENTRY(19),
            REG_ENTRY(20),
            REG_ENTRY(21),
            REG_ENTRY(22),
            REG_ENTRY(23),
            REG_ENTRY(24),
            REG_ENTRY(25),
            REG_ENTRY(26),
            REG_ENTRY(27),
            REG_ENTRY(28),
            REG_ENTRY(29),
            REG_ENTRY(30),
            REG_ENTRY(31),
            REG_ENTRY(32),
            REG_ENTRY(33),
            REG_ENTRY(34),
            REG_ENTRY(35),
            REG_ENTRY(36),
            REG_ENTRY(37),
            REG_ENTRY(38),
            REG_ENTRY(39),
            REG_ENTRY(40),
            REG_ENTRY(41),
            REG_ENTRY(42),
            REG_ENTRY(43),
            REG_ENTRY(44),
            REG_ENTRY(45),
            REG_ENTRY(46),
            REG_ENTRY(47),
            REG_ENTRY(48),
            REG_ENTRY(49),
            REG_ENTRY(50),
            REG_ENTRY(51),
            REG_ENTRY(52),
            REG_ENTRY(53),
            REG_ENTRY(54),
            REG_ENTRY(55),
            REG_ENTRY(56),
            REG_ENTRY(57),
            REG_ENTRY(58),
            REG_ENTRY(59),
            REG_ENTRY(60),
            REG_ENTRY(61),
            REG_ENTRY(62),
            REG_ENTRY(63),
            REG_ENTRY(64),
            REG_ENTRY(65),
            REG_ENTRY(66),
            REG_ENTRY(67),
            REG_ENTRY(68),
            REG_ENTRY(69),
            REG_ENTRY(70),
            REG_ENTRY(71),
            REG_ENTRY(72),
            REG_ENTRY(73),
            REG_ENTRY(74),
            REG_ENTRY(75),
            REG_ENTRY(76),
            REG_ENTRY(77),
            REG_ENTRY(78),
            REG_ENTRY(79),
            REG_ENTRY(80),
            REG_ENTRY(81),
            REG_ENTRY(82),
            REG_ENTRY(83),
            REG_ENTRY(84),
            REG_ENTRY(85),
            REG_ENTRY(86),
            REG_ENTRY(87),
            REG_ENTRY(88),
            REG_ENTRY(89),
            REG_ENTRY(90),
            REG_ENTRY(91),
            REG_ENTRY(92),
            REG_ENTRY(93),
            REG_ENTRY(94),
            REG_ENTRY(95),
            REG_ENTRY(96),
            REG_ENTRY(97),
            REG_ENTRY(98),
            REG_ENTRY(99),
            REG_ENTRY(100),
            REG_ENTRY(101),
            REG_ENTRY(102),
            REG_ENTRY(103),
            REG_ENTRY(104),
            REG_ENTRY(105),
            REG_ENTRY(106),
            REG_ENTRY(107),
            REG_ENTRY(108),
            REG_ENTRY(109),
            REG_ENTRY(110),
            REG_ENTRY(111),
            REG_ENTRY(112),
            REG_ENTRY(113),
            REG_ENTRY(114),
            REG_ENTRY(115),
            REG_ENTRY(116),
            REG_ENTRY(117),
            REG_ENTRY(118),
            REG_ENTRY(119),
            REG_ENTRY(120),
            REG_ENTRY(121),
            REG_ENTRY(122),
            REG_ENTRY(123),
            REG_ENTRY(124),
            REG_ENTRY(125),
            REG_ENTRY(126),
            REG_ENTRY(127),
            REG_ENTRY(128),
            REG_ENTRY(129),
            REG_ENTRY(130),
            REG_ENTRY(131),
            REG_ENTRY(132),
            REG_ENTRY(133),
            REG_ENTRY(134),
            REG_ENTRY(135),
            REG_ENTRY(136),
            REG_ENTRY(137),
            REG_ENTRY(138),
            REG_ENTRY(139),
            REG_ENTRY(140),
            REG_ENTRY(141),
            REG_ENTRY(142),
            REG_ENTRY(143),
            REG_ENTRY(144),
            REG_ENTRY(145),
            REG_ENTRY(146),
            REG_ENTRY(147),
            REG_ENTRY(148),
            REG_ENTRY(149),
            REG_ENTRY(150),
            REG_ENTRY(151),
            REG_ENTRY(152),
            REG_ENTRY(153),
            REG_ENTRY(154),
            REG_ENTRY(155),
            REG_ENTRY(156),
            REG_ENTRY(157),
            REG_ENTRY(158),
            REG_ENTRY(159),
            REG_ENTRY(160),
            REG_ENTRY(161),
            REG_ENTRY(162),
            REG_ENTRY(163),
            REG_ENTRY(164),
            REG_ENTRY(165),
            REG_ENTRY(166),
            REG_ENTRY(167),
            REG_ENTRY(168),
            REG_ENTRY(169),
            REG_ENTRY(170),
            REG_ENTRY(171),
            REG_ENTRY(172),
            REG_ENTRY(173),
            REG_ENTRY(174),
            REG_ENTRY(175),
            REG_ENTRY(176),
            REG_ENTRY(177),
            REG_ENTRY(178),
            REG_ENTRY(179),
            REG_ENTRY(180),
            REG_ENTRY(181),
            REG_ENTRY(182),
            REG_ENTRY(183),
            REG_ENTRY(184),
            REG_ENTRY(185),
            REG_ENTRY(186),
            REG_ENTRY(187),
            REG_ENTRY(188),
            REG_ENTRY(189),
            REG_ENTRY(190),
            REG_ENTRY(191),
            REG_ENTRY(192),
            REG_ENTRY(193),
            REG_ENTRY(194),
            REG_ENTRY(195),
            REG_ENTRY(196),
            REG_ENTRY(197),
            REG_ENTRY(198),
            REG_ENTRY(199),
            REG_ENTRY(200),
            REG_ENTRY(201),
            REG_ENTRY(202),
            REG_ENTRY(203),
            REG_ENTRY(204),
            REG_ENTRY(205),
            REG_ENTRY(206),
            REG_ENTRY(207),
            REG_ENTRY(208),
            REG_ENTRY(209),
            REG_ENTRY(210),
            REG_ENTRY(211),
            REG_ENTRY(212),
            REG_ENTRY(213),
            REG_ENTRY(214),
            REG_ENTRY(215),
            REG_ENTRY(216),
            REG_ENTRY(217),
            REG_ENTRY(218),
            REG_ENTRY(219),
            REG_ENTRY(220),
            REG_ENTRY(221),
            REG_ENTRY(222),
            REG_ENTRY(223),
            REG_ENTRY(224),
            REG_ENTRY(225),
            REG_ENTRY(226),
            REG_ENTRY(227),
            REG_ENTRY(228),
            REG_ENTRY(229),
            REG_ENTRY(230),
            REG_ENTRY(231),
            REG_ENTRY(232),
            REG_ENTRY(233),
            REG_ENTRY(234),
            REG_ENTRY(235),
            REG_ENTRY(236),
            REG_ENTRY(237),
            REG_ENTRY(238),
            REG_ENTRY(239),
            REG_ENTRY(240),
            REG_ENTRY(241),
            REG_ENTRY(242),
            REG_ENTRY(243),
            REG_ENTRY(244),
            REG_ENTRY(245),
            REG_ENTRY(246),
            REG_ENTRY(247),
            REG_ENTRY(248),
            REG_ENTRY(249),
            REG_ENTRY(250),
            REG_ENTRY(251),
            REG_ENTRY(252),
            REG_ENTRY(253),
            REG_ENTRY(254),
            REG_ENTRY(255),
            REG_ENTRY(256),
            REG_ENTRY(257),
            REG_ENTRY(258),
            REG_ENTRY(259),
            REG_ENTRY(260),
            REG_ENTRY(261),
            REG_ENTRY(262),
            REG_ENTRY(263),
            REG_ENTRY(264),
            REG_ENTRY(265),
            REG_ENTRY(266),
            REG_ENTRY(267),
            REG_ENTRY(268),
            REG_ENTRY(269),
            REG_ENTRY(270),
            REG_ENTRY(271),
            REG_ENTRY(272),
            REG_ENTRY(273),
            REG_ENTRY(274),
            REG_ENTRY(275),
            REG_ENTRY(276),
            REG_ENTRY(277),
            REG_ENTRY(278),
            REG_ENTRY(279),
            REG_ENTRY(280),
            REG_ENTRY(281),
            REG_ENTRY(282),
            REG_ENTRY(283),
            REG_ENTRY(284),
            REG_ENTRY(285),
            REG_ENTRY(286),
            REG_ENTRY(287),
            REG_ENTRY(288),
            REG_ENTRY(289),
            REG_ENTRY(290),
            REG_ENTRY(291),
            REG_ENTRY(292),
            REG_ENTRY(293),
            REG_ENTRY(294),
            REG_ENTRY(295),
            REG_ENTRY(296),
            REG_ENTRY(297),
            REG_ENTRY(298),
            REG_ENTRY(299),
            REG_ENTRY(300),
            REG_ENTRY(301),
            REG_ENTRY(302),
            REG_ENTRY(303),
            REG_ENTRY(304),
            REG_ENTRY(305),
            REG_ENTRY(306),
            REG_ENTRY(307),
            REG_ENTRY(308),
            REG_ENTRY(309),
            REG_ENTRY(310),
            REG_ENTRY(311),
            REG_ENTRY(312),
            REG_ENTRY(313),
            REG_ENTRY(314),
            REG_ENTRY(315),
            REG_ENTRY(316),
            REG_ENTRY(317),
            REG_ENTRY(318),
            REG_ENTRY(319),
            REG_ENTRY(320),
            REG_ENTRY(321),
            REG_ENTRY(322),
            REG_ENTRY(323),
            REG_ENTRY(324),
            REG_ENTRY(325),
            REG_ENTRY(326),
            REG_ENTRY(327),
            REG_ENTRY(328),
            REG_ENTRY(329),
            REG_ENTRY(330),
            REG_ENTRY(331),
            REG_ENTRY(332),
            REG_ENTRY(333),
            REG_ENTRY(334),
            REG_ENTRY(335),
            REG_ENTRY(336),
            REG_ENTRY(337),
            REG_ENTRY(338),
            REG_ENTRY(339),
            REG_ENTRY(340),
            REG_ENTRY(341),
            REG_ENTRY(342),
            REG_ENTRY(343),
            REG_ENTRY(344),
            REG_ENTRY(345),
            REG_ENTRY(346),
            REG_ENTRY(347),
            REG_ENTRY(348),
            REG_ENTRY(349),
            REG_ENTRY(350),
            REG_ENTRY(351),
            REG_ENTRY(352),
            REG_ENTRY(353),
            REG_ENTRY(354),
            REG_ENTRY(355),
            REG_ENTRY(356),
            REG_ENTRY(357),
            REG_ENTRY(358),
            REG_ENTRY(359),
            REG_ENTRY(360),
            REG_ENTRY(361),
            REG_ENTRY(362),
            REG_ENTRY(363),
            REG_ENTRY(364),
            REG_ENTRY(365),
            REG_ENTRY(366),
            REG_ENTRY(367),
            REG_ENTRY(368),
            REG_ENTRY(369),
            REG_ENTRY(370),
            REG_ENTRY(371),
            REG_ENTRY(372),
            REG_ENTRY(373),
            REG_ENTRY(374),
            REG_ENTRY(375),
            REG_ENTRY(376),
            REG_ENTRY(377),
            REG_ENTRY(378),
            REG_ENTRY(379),
            REG_ENTRY(380),
            REG_ENTRY(381),
            REG_ENTRY(382),
            REG_ENTRY(383),
            REG_ENTRY(384),
            REG_ENTRY(385),
            REG_ENTRY(386),
            REG_ENTRY(387),
            REG_ENTRY(388),
            REG_ENTRY(389),
            REG_ENTRY(390),
            REG_ENTRY(391),
            REG_ENTRY(392),
            REG_ENTRY(393),
            REG_ENTRY(394),
            REG_ENTRY(395),
            REG_ENTRY(396),
            REG_ENTRY(397),
            REG_ENTRY(398),
            REG_ENTRY(399),
            REG_ENTRY(400),
            REG_ENTRY(401),
            REG_ENTRY(402),
            REG_ENTRY(403),
            REG_ENTRY(404),
            REG_ENTRY(405),
            REG_ENTRY(406),
            REG_ENTRY(407),
            REG_ENTRY(408),
            REG_ENTRY(409),
            REG_ENTRY(410),
            REG_ENTRY(411),
            REG_ENTRY(412),
            REG_ENTRY(413),
            REG_ENTRY(414),
            REG_ENTRY(415),
            REG_ENTRY(416),
            REG_ENTRY(417),
            REG_ENTRY(418),
            REG_ENTRY(419),
            REG_ENTRY(420),
            REG_ENTRY(421),
            REG_ENTRY(422),
            REG_ENTRY(423),
            REG_ENTRY(424),
            REG_ENTRY(425),
            REG_ENTRY(426),
            REG_ENTRY(427),
            REG_ENTRY(428),
            REG_ENTRY(429),
            REG_ENTRY(430),
            REG_ENTRY(431),
            REG_ENTRY(432),
            REG_ENTRY(433),
            REG_ENTRY(434),
            REG_ENTRY(435),
            REG_ENTRY(436),
            REG_ENTRY(437),
            REG_ENTRY(438),
            REG_ENTRY(439),
            REG_ENTRY(440),
            REG_ENTRY(441),
            REG_ENTRY(442),
            REG_ENTRY(443),
            REG_ENTRY(444),
            REG_ENTRY(445),
            REG_ENTRY(446),
            REG_ENTRY(447),
            REG_ENTRY(448),
            REG_ENTRY(449),
            REG_ENTRY(450),
            REG_ENTRY(451),
            REG_ENTRY(452),
            REG_ENTRY(453),
            REG_ENTRY(454),
            REG_ENTRY(455),
            REG_ENTRY(456),
            REG_ENTRY(457),
            REG_ENTRY(458),
            REG_ENTRY(459),
            REG_ENTRY(460),
            REG_ENTRY(461),
            REG_ENTRY(462),
            REG_ENTRY(463),
            REG_ENTRY(464),
            REG_ENTRY(465),
            REG_ENTRY(466),
            REG_ENTRY(467),
            REG_ENTRY(468),
            REG_ENTRY(469),
            REG_ENTRY(470),
            REG_ENTRY(471),
            REG_ENTRY(472),
            REG_ENTRY(473),
            REG_ENTRY(474),
            REG_ENTRY(475),
            REG_ENTRY(476),
            REG_ENTRY(477),
            REG_ENTRY(478),
            REG_ENTRY(479),
            REG_ENTRY(480),
            REG_ENTRY(481),
            REG_ENTRY(482),
            REG_ENTRY(483),
            REG_ENTRY(484),
            REG_ENTRY(485),
            REG_ENTRY(486),
            REG_ENTRY(487),
            REG_ENTRY(488),
            REG_ENTRY(489),
            REG_ENTRY(490),
            REG_ENTRY(491),
            REG_ENTRY(492),
            REG_ENTRY(493),
            REG_ENTRY(494),
            REG_ENTRY(495),
            REG_ENTRY(496),
            REG_ENTRY(497),
            REG_ENTRY(498),
            REG_ENTRY(499),
            REG_ENTRY(500),
            REG_ENTRY(501),
            REG_ENTRY(502),
            REG_ENTRY(503),
            REG_ENTRY(504),
            REG_ENTRY(505),
            REG_ENTRY(506),
            REG_ENTRY(507),
            REG_ENTRY(508),
            REG_ENTRY(509),
            REG_ENTRY(510),
            REG_ENTRY(511),
            /* sorry for this; but since it's a hacker module, it shouldn't be that bad */
            /* 256 is not enough... */
            MENU_EOL,
        }
    }
};

static MENU_UPDATE_FUNC(show_update)
{
    static struct tm tm;
    if (show_what == SHOW_MODIFIED)
        MENU_SET_VALUE("Modified since %02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    else
        LoadCalendarFromRTC(&tm);

    int changed = 0;
    
    for (int reg = 0; reg < reg_num; reg++)
    {
        struct menu_entry * entry = &(adtg_gui_menu[0].children[reg + 2]);
        
        if ((int)entry->priv != reg)
            break;

        switch (show_what)
        {
            case SHOW_KNOWN_ONLY:
            {
                int found = 0;
                for (int i = 0; i < COUNT(known_regs); i++)
                {
                    if (known_match(i, reg))
                    {
                        found = 1;
                    }
                }
             
                if (entry->shidden != !found)
                {
                    entry->shidden = !found;
                    changed = 1;
                }
                break;
            }
            case SHOW_MODIFIED:
            {
                int modified = regs[reg].val != regs[reg].prev_val;
                if (entry->shidden != !modified)
                {
                    entry->shidden = !modified;
                    changed = 1;
                }
                break;
            }
            case SHOW_ALL:
            {
                if (entry->shidden)
                {
                    entry->shidden = 0;
                    changed = 1;
                }
                break;
            }
        }
        
        if (show_what != SHOW_MODIFIED)
        {
            regs[reg].prev_val = regs[reg].val;
        }
    }
    
    if (changed && info->can_custom_draw)
    {
        /* just a little trick to avoid transient redrawing artifacts */
        /* todo: better backend support for dynamic menus? */
        info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
        bmp_printf(FONT_LARGE, info->x, info->y, "Updating...");
    }
}

static unsigned int adtg_gui_init()
{
    if (is_camera("5D3", "1.1.3"))
    {
        ADTG_WRITE_FUNC = 0x11644;
        CMOS_WRITE_FUNC = 0x119CC;
        CMOS2_WRITE_FUNC = 0x11784;
        CMOS16_WRITE_FUNC = 0x11AB8;
    }
    else if (is_camera("5D2", "2.1.2"))
    {
        ADTG_WRITE_FUNC = 0xffa35cbc;
        CMOS_WRITE_FUNC = 0xffa35e70;
    }
    else if (is_camera("500D", "1.1.1")) // http://www.magiclantern.fm/forum/index.php?topic=6751.msg70325#msg70325
    {
        ADTG_WRITE_FUNC = 0xFF22F8F4; //"[REG] @@@@@@@@@@@@ Start ADTG[CS:%lx]"
        CMOS_WRITE_FUNC = 0xFF22F9DC; //"[REG] ############ Start CMOS"
    }
    else if (is_camera("550D", "1.0.9")) // http://www.magiclantern.fm/forum/index.php?topic=6751.msg61551#msg61551
    {
        ADTG_WRITE_FUNC = 0xff27ee34;
        CMOS_WRITE_FUNC = 0xff27f028;
    }
    else if (is_camera("60D", "1.1.1")) // http://www.magiclantern.fm/forum/index.php?topic=6751.msg69719#msg69719
    {
        ADTG_WRITE_FUNC = 0xFF2C9788; //"[REG] @@@@@@@@@@@@ Start ADTG[CS:%lx]"
        CMOS_WRITE_FUNC = 0xFF2C997C; //"[REG] ############ Start CMOS"
    }
    else if (is_camera("50D", "1.0.9")) // http://www.magiclantern.fm/forum/index.php?topic=6751.msg63322#msg63322
    {
        ADTG_WRITE_FUNC = 0xFFA11FDC;
        CMOS_WRITE_FUNC = 0xFFA12190;
    }
    else if (is_camera("6D", "1.1.3")) // from 1%
    {
        CMOS_WRITE_FUNC = 0x2445C; //"[REG] ############ Start CMOS OC_KICK"
        CMOS2_WRITE_FUNC = 0x2420C; //"[REG] ############ Start CMOS"
        ADTG_WRITE_FUNC = 0x24108; //"[REG] @@@@@@@@@@@@ Start ADTG[CS:%lx]"
        CMOS16_WRITE_FUNC = 0x24548; //"[REG] ############ Start CMOS16 OC_KICK"
    }
    else if (is_camera("EOSM", "2.0.2")) // from 1%
    {
        ADTG_WRITE_FUNC = 0x2986C;
        CMOS_WRITE_FUNC = 0x2998C;
    }
    else if (is_camera("600D", "1.0.2")) // from 1% TL 2.0
    {
        ADTG_WRITE_FUNC = 0xFF2DCEF4; //"[REG] @@@@@@@@@@@@ Start ADTG[CS:%lx]"
        CMOS_WRITE_FUNC = 0xFF2DD0E8; //"[REG] ############ Start CMOS"
    }
    else if (is_camera("650D", "1.0.4"))
    {
        ADTG_WRITE_FUNC = 0x178FC; //"[REG] @@@@@@@@@@@@ Start ADTG[CS:%lx]"
        CMOS_WRITE_FUNC = 0x17A1C; //"[REG] ############ Start CMOS"
    }
    else if (is_camera("700D", "1.1.1"))
    {
        ADTG_WRITE_FUNC = 0x178FC; //"[REG] @@@@@@@@@@@@ Start ADTG[CS:%lx]"
        CMOS_WRITE_FUNC = 0x17A1C; //"[REG] ############ Start CMOS"
    }    
    else return CBR_RET_ERROR;

    
    menu_add("Debug", adtg_gui_menu, COUNT(adtg_gui_menu));
    return 0;
}

static unsigned int adtg_gui_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(adtg_gui_init)
    MODULE_DEINIT(adtg_gui_deinit)
MODULE_INFO_END()
