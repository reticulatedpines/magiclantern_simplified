#include <dryos.h>
#include <property.h>

/* Generic implementation for various custom functions
 * On some cameras, these are buried into CFn properties;
 * on others, they are implemented with simple properties,
 * so they can be accessed with generic code.
 */

/*
 * PROP_ALO (from 1%):
 * buf[0]  actual ALO setting (maybe disabled by Manual mode or HTP)  
 * buf[1]  original ALO setting (5D3 also has this)
 * buf[2]  1: disable ALO in Manual mode 0: no effect (5D3 also has this)
 */

/* ALO: we only display the status on the screen */
/* Setting it is more difficult on recent cameras, and there's little point in doing that */
#define GENERIC_GET_ALO \
    static PROP_INT(PROP_ALO, alo); \
    int get_alo() { return alo & 0xFF; }

/* HTP: only interesting for displaying status and ISO values */
/* (only useful for JPG/H.264; no effect on RAW) */
#define GENERIC_GET_HTP \
    static PROP_INT(PROP_HTP, htp); \
    int get_htp() { return htp; }

/* Here we want both the getter and the setter */
#define GENERIC_GET_MLU \
    static PROP_INT(PROP_MLU, mlu); \
    int get_mlu() { return mlu; }

#define GENERIC_SET_MLU \
    void set_mlu(int value) { \
        value = COERCE(value, 0, 1); \
        prop_request_change(PROP_MLU, &value, 4); \
    }
