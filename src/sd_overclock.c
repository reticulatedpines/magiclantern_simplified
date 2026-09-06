/* SDHC Overclock for DIGIC 4 SD bodies
 *
 * Raises the SD bus clock above Canon's 48 MHz High-Speed setting by
 * reprogramming PLL1, the SD root clock.
 *
 * Principle of operation
 * ----------------------
 * The SD bus is clocked from PLL1 at 0xC04000BC.  Sweeping that register
 * moved card throughput 1.65x (N=5 -> N=6) while an RTC-referenced timer
 * rate and a DRAM memcpy control stayed flat, so PLL1 is the SD root and
 * not a system or CPU clock.
 *
 * The word is a 16.16 fixed-point divider:
 *
 *     word = 0x3CFC8 + k * 0x1000
 *     f    = 1152 / k  MHz        (at the undivided mux setting)
 *     stock k = 24 -> 48.0 MHz
 *
 * CLK_SEL[25:24] at 0xC0400004 is a post-divider of (4 - mode).  Mode 3 is
 * Canon's undivided High-Speed setting.  Mode 0 is a fixed ~200 kHz
 * identification clock independent of PLL1, which is why raising the root
 * does not disturb card init.
 *
 * Valid-word rule (fatal if broken)
 * ---------------------------------
 * Only words from the table below are safe.  Two bit constraints, both
 * learned by hard hang:
 *
 *   bits [11:0] must stay 0xFC8
 *   bits [13:12] must be 00
 *
 * Together they mean k must be a multiple of 4, so the only reachable
 * roots are 288/m MHz.  There is nothing between 96 and 144.  An off-grid
 * word (e.g. 0x0004BFC8, k=15) hard hung the camera: screen off, no
 * button response, battery pull.  Never compute a word at runtime.
 *
 * Measured on 600D 1.0.2 (16 MB file, 2 MB chunks, interleaved):
 *   48 MHz  read 755 ms / write 802 ms
 *   96 MHz  read 397 ms / write 384 ms   (1.90x / 2.08x, byte-exact)
 *   144 MHz wedged the SD driver on the tested card (UI still alive)
 *
 * Apply sequence
 * --------------
 * Canon's sd_set_bus_clock_mode has no status-bit polling: it gates
 * CLOCK_ENABLE bit 28 off, writes CLK_SEL[25:24], gates back on, with a
 * ~1 ms delay on each side.  DAT at 0xFF4FAB54 reads 0xC0400000, so the
 * base is fixed hardware and the sequence can be reproduced in C with no
 * ROM address - which is what makes this portable across DIGIC 4 SD
 * bodies.  The PLL write is folded inside the gated window; changing the
 * root while the clock is live is what hung early probe attempts.
 *
 * Canon's own routine leaves the gate OFF and kills the interface if
 * called with mode > 3; the mux value here is therefore a constant, not
 * a parameter.
 */

#include "dryos.h"
#include "menu.h"
#include "config.h"
#include "notify_box.h"

#if defined(CONFIG_DIGIC_IV) && !defined(CONFIG_CF_SLOT)

/* ---------------------------------------------------------------------------
 * Register map (DIGIC 4 SD clock block at 0xC0400000)
 * ------------------------------------------------------------------------- */

#define SDCLK_REG_CLK_SEL     0xC0400004   /* [25:24] bus clock mux, div = 4 - mode */
#define SDCLK_REG_CLOCK_EN    0xC0400008   /* bit 28 gates the SD interface clock */
#define SDCLK_REG_PLL1_WORD   0xC04000BC   /* SD root PLL, 16.16 divider word */

#define SDCLK_GATE_BIT        (1u << 28)
#define SDCLK_MUX_SHIFT       24
#define SDCLK_MUX_MASK        (3u << SDCLK_MUX_SHIFT)
#define SDCLK_MUX_UNDIVIDED   3            /* div = 1; Canon's own maximum */

#define SDCLK_WORD_BASE       0x0003CFC8u  /* word = BASE + k * 0x1000 */
#define SDCLK_WORD_STOCK      0x00054FC8u  /* k = 24 -> 48.0 MHz */

/* Scratch file for the post-apply write/read verification. */
#define SDCLK_VERIFY_FILE     "ML/SDOCLK.TMP"
#define SDCLK_VERIFY_SIZE     512

/* Dirty-flag basename under get_config_dir(); armed only above MAX_VERIFIED. */
#define SDCLK_FLAG_NAME       "SDOCLK.TRY"

/* ---------------------------------------------------------------------------
 * Frequency table
 *
 * Valid words obey two constraints, both learned the hard way:
 *   bits [11:0] must stay 0xFC8 - the four words that changed them got slower,
 *                                 never faster, and then wedged;
 *   bits [13:12] must be 00     - 0x0004BFC8 has 0b11 there and hard hung the
 *                                 camera: screen off, no button response,
 *                                 battery pull.  A malformed divider output
 *                                 takes the whole system down, not just the
 *                                 card, so a bad word is unrecoverable and no
 *                                 software guard can help.  Never compute a
 *                                 word at runtime; only pick from this table.
 * Together they mean k must be a multiple of 4, so the only reachable roots are
 * 288/m MHz.  There is nothing between 96 and 144.
 * ------------------------------------------------------------------------- */

struct sdclk_setting {
    uint32_t word;      /* 0xC04000BC value */
    int16_t  k;         /* divider in sixteenths; f = 1152/k MHz */
    int16_t  mhz10;     /* bus clock in tenths of MHz, at mux 3 */
    const char * name;
};

static const struct sdclk_setting sdclk_settings[] = {
    { 0x00054FC8u, 24,  480, "48 MHz (stock)"   },
    { 0x0004CFC8u, 16,  720, "72 MHz"           },
    { 0x00048FC8u, 12,  960, "96 MHz"           },
    { 0x00044FC8u,  8, 1440, "144 MHz DANGER"   },
};

#define SDCLK_IDX_STOCK        0
#define SDCLK_IDX_MAX_VERIFIED 2   /* 96 MHz; anything past this arms the flag */

/* Persisted menu index.  Default 0 = stock. */
static CONFIG_INT("sd.overclock", sd_overclock_idx, 0);

/* Latched at INIT_FUNC, before anything writes PLL1.  Every number in the
 * table is calibrated against SDCLK_WORD_STOCK; a different reset default
 * means different silicon and the feature disables itself. */
static uint32_t sdclk_pll1_at_init = 0;
static int sdclk_calibrated = 0;

/* ---------------------------------------------------------------------------
 * Apply / decode
 * ------------------------------------------------------------------------- */

/* Faithful reproduction of Canon's sd_set_bus_clock_mode with the PLL word
 * write folded inside the gated window.  Mux is a constant (see file header). */
static void sdclk_apply(uint32_t word)
{
    uint32_t irq = cli();
    MEM(SDCLK_REG_CLOCK_EN) &= ~SDCLK_GATE_BIT;
    sei(irq);
    msleep(1);

    irq = cli();
    MEM(SDCLK_REG_PLL1_WORD) = word;
    (void)MEM(SDCLK_REG_PLL1_WORD);
    MEM(SDCLK_REG_CLK_SEL) = (MEM(SDCLK_REG_CLK_SEL) & ~SDCLK_MUX_MASK)
                           | (SDCLK_MUX_UNDIVIDED << SDCLK_MUX_SHIFT);
    MEM(SDCLK_REG_CLOCK_EN) |=  SDCLK_GATE_BIT;
    sei(irq);
    msleep(2);
}

/* Decode a live PLL1 word to tenths of MHz.  Returns 0 if the word is not
 * on the 0x1000 grid from SDCLK_WORD_BASE (and therefore not one of ours). */
static int sdclk_mhz10_from_word(uint32_t word)
{
    if (word < SDCLK_WORD_BASE)
        return 0;
    uint32_t delta = word - SDCLK_WORD_BASE;
    if (delta % 0x1000u)
        return 0;
    int k = (int)(delta / 0x1000u);
    if (k <= 0)
        return 0;
    return 11520 / k;
}

static int sdclk_index_of_word(uint32_t word)
{
    for (int i = 0; i < COUNT(sdclk_settings); i++)
        if (sdclk_settings[i].word == word)
            return i;
    return -1;
}

static void sdclk_flag_path(char *buf, size_t size)
{
    snprintf(buf, size, "%s%s", get_config_dir(), SDCLK_FLAG_NAME);
}

/* ---------------------------------------------------------------------------
 * Post-apply verification: write then read back a 512-byte pattern.
 * Exercises both directions at the new clock; cannot pass on cached data.
 * ------------------------------------------------------------------------- */

static inline uint8_t sdclk_pattern_byte(int i)
{
    return (uint8_t)((i * 17) ^ 0xA5);
}

/* buf must be SDCLK_VERIFY_SIZE bytes from fio_malloc: card I/O wants DMA-able
 * memory, and a buffer this size has no business on a task stack.  An earlier
 * revision used two 512-byte stack arrays, which gave this function a 1152-byte
 * frame inside a task created with a 1024-byte stack - it ran off the end of the
 * stack on every boot and corrupted the heap behind it. */
static int sdclk_verify(void *buf)
{
    uint8_t *p = (uint8_t *)UNCACHEABLE(buf);

    for (int i = 0; i < SDCLK_VERIFY_SIZE; i++)
        p[i] = sdclk_pattern_byte(i);

    FIO_RemoveFile(SDCLK_VERIFY_FILE);

    FILE *f = FIO_CreateFile(SDCLK_VERIFY_FILE);
    if (!f)
        return 0;
    int w = FIO_WriteFile(f, p, SDCLK_VERIFY_SIZE);
    FIO_CloseFile(f);
    if (w != SDCLK_VERIFY_SIZE)
    {
        FIO_RemoveFile(SDCLK_VERIFY_FILE);
        return 0;
    }

    f = FIO_OpenFile(SDCLK_VERIFY_FILE, O_RDONLY | O_SYNC);
    if (!f)
    {
        FIO_RemoveFile(SDCLK_VERIFY_FILE);
        return 0;
    }
    memset(p, 0, SDCLK_VERIFY_SIZE);
    int r = FIO_ReadFile(f, p, SDCLK_VERIFY_SIZE);
    FIO_CloseFile(f);
    FIO_RemoveFile(SDCLK_VERIFY_FILE);

    if (r != SDCLK_VERIFY_SIZE)
        return 0;

    /* Compare against the generator rather than a second buffer, so the
     * read-back costs no extra memory. */
    for (int i = 0; i < SDCLK_VERIFY_SIZE; i++)
        if (p[i] != sdclk_pattern_byte(i))
            return 0;
    return 1;
}

/* ---------------------------------------------------------------------------
 * Menu
 * ------------------------------------------------------------------------- */

static MENU_UPDATE_FUNC(sd_overclock_update)
{
    uint32_t live = MEM(SDCLK_REG_PLL1_WORD);
    int mhz10 = sdclk_mhz10_from_word(live);
    int live_idx = sdclk_index_of_word(live);

    if (mhz10)
        MENU_SET_RINFO("now %d.%d", mhz10 / 10, mhz10 % 10);
    else
        MENU_SET_RINFO("now ?");

    if (!sdclk_calibrated)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING,
            "PLL1 was %x, need %x - feature disabled.",
            sdclk_pll1_at_init, SDCLK_WORD_STOCK);
        return;
    }

    if (sd_overclock_idx > SDCLK_IDX_MAX_VERIFIED)
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE,
            "144 MHz wedged the SD driver on the tested card.");
    }

    if (live_idx != sd_overclock_idx)
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE,
            "Restart the camera to apply.");
    }
}

static struct menu_entry sd_overclock_menu[] = {
    {
        .name    = "SDHC Overclock",
        .priv    = &sd_overclock_idx,
        .max     = COUNT(sdclk_settings) - 1,
        .choices = CHOICES("48 MHz (stock)", "72 MHz", "96 MHz", "144 MHz DANGER"),
        .update  = sd_overclock_update,
        .help    = "SD bus clock. Applied at boot; restart to take effect.",
        .help2   = "48 MHz: Canon's setting.\n"
                   "72 MHz: 1.45x read. Verified.\n"
                   "96 MHz: 1.90x read, 2.08x write. Verified.\n"
                   "144 MHz: wedged the SD driver on the tested card.\n",
    }
};

/* ---------------------------------------------------------------------------
 * Boot: INIT_FUNC latches the guard; TASK applies after config_load
 * ------------------------------------------------------------------------- */

static void sd_overclock_init(void *unused)
{
    (void)unused;
    /* Sample PLL1 before any ML code writes it.  The task (and the menu
     * update) both consult this latch; if it is not stock the feature is
     * inert on this body. */
    sdclk_pll1_at_init = MEM(SDCLK_REG_PLL1_WORD);
    sdclk_calibrated = (sdclk_pll1_at_init == SDCLK_WORD_STOCK);

    menu_add("Debug", sd_overclock_menu, COUNT(sd_overclock_menu));
}

static void sd_overclock_task(void *unused)
{
    (void)unused;
    /* Same delay as FEATURE_SD_AUTOTUNE in sd.c: do not touch the SD clock
     * while DryOS is still configuring the card.  That hangs. */
    msleep(1500);

    if (!sdclk_calibrated)
        return;

    struct card_info *card = get_ml_card();
    if (!card || !card->type || !streq(card->type, "SD"))
        return;

    /* Clamp a corrupted config value rather than indexing off the table. */
    if (sd_overclock_idx < 0 || sd_overclock_idx >= COUNT(sdclk_settings))
        sd_overclock_idx = SDCLK_IDX_STOCK;

    char flag[0x80];
    sdclk_flag_path(flag, sizeof(flag));

    /* Previous boot armed the flag and never cleared it - that attempt
     * wedged.  Refuse, reset to stock, and clear the flag so the next boot
     * is clean. */
    if (config_flag_file_setting_load(flag))
    {
        sd_overclock_idx = SDCLK_IDX_STOCK;
        config_flag_file_setting_save(flag, 0);
        config_save();
        NotifyBox(5000, "SDHC Overclock: previous attempt failed.\n"
                        "Reset to 48 MHz (stock).");
        return;
    }

    if (sd_overclock_idx == SDCLK_IDX_STOCK)
        return;

    /* Get the verification buffer before touching the clock: if there is no
     * memory to verify with there is no way to tell a working overclock from a
     * broken one, so the clock is left alone entirely. */
    void *buf = fio_malloc(SDCLK_VERIFY_SIZE);
    if (!buf)
        return;

    /* Arm the flag only for unverified rungs.  Verified 72/96 MHz pay no
     * per-boot file write; if a verified rung somehow wedges there is no
     * auto-recovery, which is the trade-off accepted for those settings. */
    if (sd_overclock_idx > SDCLK_IDX_MAX_VERIFIED)
        config_flag_file_setting_save(flag, 1);

    sdclk_apply(sdclk_settings[sd_overclock_idx].word);

    int ok = sdclk_verify(buf);
    fio_free(buf);

    if (ok)
    {
        if (sd_overclock_idx > SDCLK_IDX_MAX_VERIFIED)
            config_flag_file_setting_save(flag, 0);
        return;
    }

    /* Verification failed: put the clock back, clear the setting so it
     * does not retry every boot, and clear any flag we just armed. */
    sdclk_apply(SDCLK_WORD_STOCK);
    sd_overclock_idx = SDCLK_IDX_STOCK;
    config_flag_file_setting_save(flag, 0);
    config_save();
    NotifyBox(5000, "SDHC Overclock: verify failed.\n"
                    "Reset to 48 MHz (stock).");
}

INIT_FUNC("sd_oc", sd_overclock_init);

/* 0x1000 stack, the size every ML task that calls into snprintf and FIO uses.
 * The 0x400 that sd.c gets away with is only enough for a task whose body is a
 * single ROM call; this one builds a path and does file I/O.  GCC reserves the
 * whole frame at entry, so an undersized stack here overflows on every boot
 * regardless of which branch runs, and DryOS task stacks come off the heap -
 * the symptom is malloc failures in unrelated code. */
TASK_CREATE("sd_oc", sd_overclock_task, 0, 0x1e, 0x1000);

#endif /* CONFIG_DIGIC_IV && !CONFIG_CF_SLOT */
