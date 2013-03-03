#include <dryos.h>
#include <property.h>

int GUI_GetCFnForTab4(int);
int GUI_SetCFnForTab4(int,int);

int get_htp() { return GetCFnData(1, 3); }
void set_htp(int value) { SetCFnData(1, 3, value); }

PROP_INT(PROP_ALO, alo);
int get_alo() { return alo; }

void set_alo(int value)
{
	value = COERCE(value, 0, 3);
	prop_request_change(PROP_ALO, &value, 4);
}

int get_mlu() { return GetCFnData(2, 13); }
void set_mlu(int value) { SetCFnData(2, 13, value); }

int cfn_get_af_button_assignment() { return GUI_GetCFnForTab4(6); }
void cfn_set_af_button(int value) { GUI_SetCFnForTab4(6, value); }

// 7D only; other cameras have different offsets, buffer size etc
#define PROP_AFMA_CFN 0x80010006
static int8_t afma_buf[0x17];
#define AFMA_MODE       afma_buf[0x05]
#define AFMA_PER_LENS   afma_buf[0x11]
#define AFMA_ALL_LENSES afma_buf[0x13]

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
