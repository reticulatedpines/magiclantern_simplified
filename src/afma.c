/** 
 * AF microadjustment functions: get_afma, set_afma
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
    memcpy(afma_buf, buf, sizeof(afma_buf));
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

int get_afma_mode()
{
    return AFMA_MODE;
}

int get_afma_max()
{
    return AFMA_MAX;
}
