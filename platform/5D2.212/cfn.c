#include <dryos.h>
#include <property.h>
// look on camera menu or review sites to get custom function numbers

int get_htp() { return GetCFnData(1, 3); }
void set_htp(int value) { SetCFnData(1, 3, value); }

int get_alo() { return GetCFnData(1, 4); }
void set_alo(int value) { SetCFnData(1, 4, value); }

int get_mlu() { return GetCFnData(2, 6); }
void set_mlu(int value) { SetCFnData(2, 6, value); }

int cfn_get_af_button_assignment() { return GetCFnData(3, 1); }
void cfn_set_af_button(int value) { SetCFnData(3, 1, value); }

int get_af_star_swap() { return GetCFnData(3, 2); }
void set_af_star_swap(int value) { SetCFnData(3, 2, value); }

// 5D2 only; other cameras have different offsets, buffer size etc
#define PROP_AFMA_CFN 0x80010006
static int8_t afma_buf[0xF];
#define AFMA_MODE       afma_buf[0x8]
#define AFMA_PER_LENS   afma_buf[0xC]
#define AFMA_ALL_LENSES afma_buf[0xE]

PROP_HANDLER(PROP_AFMA_CFN)
{
    ASSERT(len == sizeof(afma_buf));
    my_memcpy(afma_buf, buf, sizeof(afma_buf));
}

int get_afma(int per_lens)
{
    if (per_lens == -1) per_lens = (AFMA_MODE == 2 ? 1 : 0);
    
    if (per_lens)
        return AFMA_PER_LENS;
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
        AFMA_PER_LENS = value;
        AFMA_MODE = 2;
    }
    else
    {
        AFMA_ALL_LENSES = value;
        AFMA_MODE = 1;
    }
    prop_request_change(PROP_AFMA_CFN, afma_buf, sizeof(afma_buf));
}
