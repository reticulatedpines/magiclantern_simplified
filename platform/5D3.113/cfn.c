#include <dryos.h>
#include <property.h>
// on 5D3 these are not CFn, but in main menus

PROP_INT(PROP_HTP, htp);
PROP_INT(PROP_ALO, alo);
PROP_INT(PROP_MLU, mlu);

int get_htp() { return htp; }
void set_htp(int value)
{
    value = COERCE(value, 0, 1);
    prop_request_change(PROP_HTP, &value, 4);
}

int get_alo() { return alo & 0xFF; }
//~ void set_alo(int value)
//~ {
    //~ value = COERCE(value, 0, 3);
    //~ prop_request_change(PROP_ALO, &value, 4);
//~ }

int get_mlu() { return mlu; }
void set_mlu(int value)
{
    value = COERCE(value, 0, 1);
    prop_request_change(PROP_MLU, &value, 4);
}

static int8_t some_cfn[0x2f];
PROP_HANDLER(0x80010007)
{
    ASSERT(len == 0x2f);
    memcpy(some_cfn, buf, 0x2f);
}

int cfn_get_af_button_assignment() { return some_cfn[9]; }
void cfn_set_af_button(int value) 
{  
    some_cfn[9] = COERCE(value, 0, 2);
    prop_request_change(0x80010007, some_cfn, 0x2f);
}

// 5D3 only; other cameras have different offsets, buffer size etc
#define PROP_AFMA_CFN 0x80040027
static int8_t afma_buf[0x10];
#define AFMA_MODE       afma_buf[0x0]
#define AFMA_PER_LENS_A afma_buf[0x2]
#define AFMA_PER_LENS_B afma_buf[0x3]
#define AFMA_ALL_LENSES afma_buf[0x5]

PROP_HANDLER(PROP_AFMA_CFN)
{
    ASSERT(len == sizeof(afma_buf));
    my_memcpy(afma_buf, buf, sizeof(afma_buf));
}

int get_afma(int per_lens)
{
    if (per_lens == -1) per_lens = (AFMA_MODE == 2 ? 1 : 0);
    
    if (per_lens)
        return AFMA_PER_LENS_A;
    else
        return AFMA_ALL_LENSES;
}

void set_afma(int value, int per_lens)
{
    if (ml_shutdown_requested) return;

    if (per_lens == -1) per_lens = (AFMA_MODE == 2 ? 1 : 0);
    
    value = COERCE(value, -20, 20);
    if (per_lens)
    {
        AFMA_PER_LENS_A = AFMA_PER_LENS_B = value;
        AFMA_MODE = 2;
    }
    else
    {
        AFMA_ALL_LENSES = value;
        AFMA_MODE = 1;
    }
    prop_request_change(PROP_AFMA_CFN, afma_buf, sizeof(afma_buf));
}
