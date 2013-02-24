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


/* from Max Chen:

here is what i found:

80010006  |  000e  |  8  |  mode  |  value of adjust by lens  |  value of adjust all by same amount  |  0  |  0

mode                                          value
----------------------------------------------------------------
0:disable                                        0
1:adjust all by same amount                1000000
2:adjust by lens                           2000000

amount  value of adjust by lens       value ofadjust all by same amount
----------------------------------------------------------------------------------------------------------------------------
-20                     ec000706       ec00
-19                     ed000706       ed00
....                      .....
               .....
-1                       ff000706      ff00
0                        706             00
1                        1000706        100
....                       ....
               ....
19                      13000706        1300
20                       14000706       1400

and ,if you change the value of adjust by lens,the value of adjust all
by same amount will add 0x02,no matter what the value of adjust by
lens is.
*/

// 50D only; other cameras have different offsets, buffer size etc
#define PROP_AFMA_CFN 0x80010006
static int8_t afma_buf[0xE];
#define AFMA_MODE       afma_buf[0x7]
#define AFMA_PER_LENS   afma_buf[0xB]
#define AFMA_ALL_LENSES afma_buf[0xD]

#ifdef CONFIG_AFMA_EXTENDED
#define AFMA_MAX 100
#else
#define AFMA_MAX 20
#endif

int get_afma_max() { return AFMA_MAX; }
int get_afma_mode() { return AFMA_MODE; }

PROP_HANDLER(PROP_AFMA_CFN)
{
    ASSERT(len == sizeof(afma_buf));
    my_memcpy(afma_buf, buf, sizeof(afma_buf));
}

int get_afma(int mode)
{
    if (mode == AFMA_MODE_AUTODETECT) mode = AFMA_MODE;
    
    if (mode == AFMA_MODE_PER_LENS)
        return AFMA_PER_LENS;

    else if (mode == AFMA_MODE_ALL_LENSES)
        return AFMA_ALL_LENSES;

    return 0;
}

void set_afma(int value, int mode)
{
    if (ml_shutdown_requested) return;

    value = COERCE(value, -AFMA_MAX, AFMA_MAX);

    if (mode == AFMA_MODE_AUTODETECT) mode = AFMA_MODE;
    if (mode == AFMA_MODE_DISABLED) mode = AFMA_MODE_ALL_LENSES;

    if (mode == AFMA_MODE_PER_LENS)
        AFMA_PER_LENS = value;
    
    else if (mode == AFMA_MODE_ALL_LENSES)
        AFMA_ALL_LENSES = value;
    
    else return; // bad arguments

    AFMA_MODE = mode;
    prop_request_change(PROP_AFMA_CFN, afma_buf, sizeof(afma_buf));
}
