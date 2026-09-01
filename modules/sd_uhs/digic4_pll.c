/* DIGIC 4 SD root-clock (PLL1) overclock backend.
 *
 * Raises the SD bus clock above Canon's 48 MHz High-Speed setting by
 * reprogramming PLL1 at 0xC04000BC.  No ROM addresses — portable across
 * DIGIC 4 SD bodies.  CF-only bodies are excluded at runtime via
 * get_ml_card()->type.
 *
 * PLL1 word: 0x3CFC8 + k * 0x1000,  f = 1152/k MHz at mux mode 3.
 * Stock k = 24 -> 48 MHz.  Only table entries are safe; off-grid words
 * can hard-hang the camera.  Apply with CLOCK_ENABLE bit 28 gated off,
 * write PLL1, set CLK_SEL[25:24] to mode 3, gate back on.
 */

#include <module.h>
#include <dryos.h>
#include <menu.h>
#include <config.h>
#include <notify_box.h>
#include "digic4_pll.h"

#define SDCLK_REG_CLK_SEL     0xC0400004
#define SDCLK_REG_CLOCK_EN    0xC0400008
#define SDCLK_REG_PLL1_WORD   0xC04000BC

#define SDCLK_GATE_BIT        (1u << 28)
#define SDCLK_MUX_SHIFT       24
#define SDCLK_MUX_MASK        (3u << SDCLK_MUX_SHIFT)
#define SDCLK_MUX_UNDIVIDED   3

#define SDCLK_WORD_STOCK      0x00054FC8u

#define SDCLK_VERIFY_FILE     "ML/SDOCLK.TMP"
#define SDCLK_VERIFY_SIZE     512
#define SDCLK_FLAG_NAME       "SDOCLK.TRY"

struct sdclk_setting {
    uint32_t word;
};

static const struct sdclk_setting sdclk_settings[] = {
    { 0x00054FC8u },
    { 0x0004CFC8u },
    { 0x00048FC8u },
};

#define SDCLK_IDX_STOCK 0

static uint32_t sdclk_pll1_at_init = 0;
static int sdclk_calibrated = 0;

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

static void sdclk_flag_path(char *buf, size_t size)
{
    snprintf(buf, size, "%s%s", get_config_dir(), SDCLK_FLAG_NAME);
}

static inline uint8_t sdclk_pattern_byte(int i)
{
    return (uint8_t)((i * 17) ^ 0xA5);
}

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

    for (int i = 0; i < SDCLK_VERIFY_SIZE; i++)
        if (p[i] != sdclk_pattern_byte(i))
            return 0;
    return 1;
}

static MENU_UPDATE_FUNC(sd_pll_update)
{
    if (!sdclk_calibrated)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING,
            "PLL1 was %x, need %x - feature disabled.",
            sdclk_pll1_at_init, SDCLK_WORD_STOCK);
        return;
    }

    if (sd_pll_freq != SDCLK_IDX_STOCK)
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE,
            "Restart the camera to apply.");
    }
}

static struct menu_entry sd_pll_menu[] = {
    {
        .name    = "SD Frequency",
        .priv    = &sd_pll_freq,
        .max     = COUNT(sdclk_settings) - 1,
        .choices = CHOICES("48 MHz (stock)", "72 MHz", "96 MHz"),
        .update  = sd_pll_update,
        .help    = "SD bus clock (PLL1). Applied at module load; restart to take effect.",
        .help2   = "48 MHz: Canon's setting.\n"
                   "72/96 MHz: untested on most bodies; verify on boot.\n",
    },
};

static void sd_pll_apply_selected(void)
{
    if (!sdclk_calibrated)
        return;

    if (sd_pll_freq < 0 || sd_pll_freq >= COUNT(sdclk_settings))
        sd_pll_freq = SDCLK_IDX_STOCK;

    char flag[0x80];
    sdclk_flag_path(flag, sizeof(flag));

    if (config_flag_file_setting_load(flag))
    {
        sd_pll_freq = SDCLK_IDX_STOCK;
        config_flag_file_setting_save(flag, 0);
        config_save();
        NotifyBox(5000, "SD Overclock: previous attempt failed.\n"
                        "Reset to 48 MHz (stock).");
        return;
    }

    if (sd_pll_freq == SDCLK_IDX_STOCK)
        return;

    void *buf = fio_malloc(SDCLK_VERIFY_SIZE);
    if (!buf)
        return;

    config_flag_file_setting_save(flag, 1);

    sdclk_apply(sdclk_settings[sd_pll_freq].word);

    int ok = sdclk_verify(buf);
    fio_free(buf);

    if (ok)
    {
        config_flag_file_setting_save(flag, 0);
        return;
    }

    sdclk_apply(SDCLK_WORD_STOCK);
    sd_pll_freq = SDCLK_IDX_STOCK;
    config_flag_file_setting_save(flag, 0);
    config_save();
    NotifyBox(5000, "SD Overclock: verify failed.\n"
                    "Reset to 48 MHz (stock).");
}

unsigned int digic4_pll_init(void)
{
    sdclk_pll1_at_init = MEM(SDCLK_REG_PLL1_WORD);
    sdclk_calibrated = (sdclk_pll1_at_init == SDCLK_WORD_STOCK);

    struct card_info *card = get_ml_card();
    if (!card || !card->type || !streq(card->type, "SD"))
        return 0;

    menu_add("Prefs", sd_pll_menu, COUNT(sd_pll_menu));

    sd_pll_apply_selected();
    return 0;
}
