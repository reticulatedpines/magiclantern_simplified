#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../src/raw.h"
#include "dcraw-bridge.h"
#include "kelvin.h"

/** Compute the number of entries in a static array */
#define COUNT(x)        ((int)(sizeof(x)/sizeof((x)[0])))

// The follwing two tables are copied straight from dcraw.c
// Update them as needed :)

/* also used in kelvin.c */
const struct {
    const char *prefix;
    short black, maximum, trans[12];
} table[] = {
    { "EOS 5D Mark III", 0, 0x3c80,
        { 6722,-635,-963,-4287,12460,2028,-908,2162,5668 } },
    { "EOS 5D Mark II", 0, 0x3cf0,
        { 4716,603,-830,-7798,15474,2480,-1496,1937,6651 } },
    { "EOS 5D", 0, 0xe6c,
        { 6347,-479,-972,-8297,15954,2480,-1968,2131,7649 } },
    { "EOS 6D", 0, 0x3c82,
        { 7034,-804,-1014,-4420,12564,2058,-851,1994,5758 } },
    { "EOS 7D", 0, 0x3510,
        { 6844,-996,-856,-3876,11761,2396,-593,1772,6198 } },
    { "EOS 10D", 0, 0xfa0,
        { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
    { "EOS 20Da", 0, 0,
        { 14155,-5065,-1382,-6550,14633,2039,-1623,1824,6561 } },
    { "EOS 20D", 0, 0xfff,
        { 6599,-537,-891,-8071,15783,2424,-1983,2234,7462 } },
    { "EOS 30D", 0, 0,
        { 6257,-303,-1000,-7880,15621,2396,-1714,1904,7046 } },
    { "EOS 40D", 0, 0x3f60,
        { 6071,-747,-856,-7653,15365,2441,-2025,2553,7315 } },
    { "EOS 50D", 0, 0x3d93,
        { 4920,616,-593,-6493,13964,2784,-1774,3178,7005 } },
    { "EOS 60D", 0, 0x2ff7,
        { 6719,-994,-925,-4408,12426,2211,-887,2129,6051 } },
    { "EOS 70D", 0, 0x3bc7,
        { 7034,-804,-1014,-4420,12564,2058,-851,1994,5758 } },	
    { "EOS 100D", 0, 0x350f,
        { 6602,-841,-939,-4472,12458,2247,-975,2039,6148 } },
    { "EOS 300D", 0, 0xfa0,
        { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
    { "EOS 350D", 0, 0xfff,
        { 6018,-617,-965,-8645,15881,2975,-1530,1719,7642 } },
    { "EOS 400D", 0, 0xe8e,
        { 7054,-1501,-990,-8156,15544,2812,-1278,1414,7796 } },
    { "EOS 450D", 0, 0x390d,
        { 5784,-262,-821,-7539,15064,2672,-1982,2681,7427 } },
    { "EOS 500D", 0, 0x3479,
        { 4763,712,-646,-6821,14399,2640,-1921,3276,6561 } },
    { "EOS 550D", 0, 0x3dd7,
        { 6941,-1164,-857,-3825,11597,2534,-416,1540,6039 } },
    { "EOS 600D", 0, 0x3510,
        { 6461,-907,-882,-4300,12184,2378,-819,1944,5931 } },
    { "EOS 650D", 0, 0x354d,
        { 6602,-841,-939,-4472,12458,2247,-975,2039,6148 } },
    { "EOS 700D", 0, 0x3c00,
        { 6602,-841,-939,-4472,12458,2247,-975,2039,6148 } },
    { "EOS 1000D", 0, 0xe43,
        { 6771,-1139,-977,-7818,15123,2928,-1244,1437,7533 } },
    { "EOS 1100D", 0, 0x3510,
        { 6444,-904,-893,-4563,12308,2535,-903,2016,6728 } },
    { "EOS M", 0, 0,
        { 6602,-841,-939,-4472,12458,2247,-975,2039,6148 } },
    { "EOS-1Ds Mark III", 0, 0x3bb0,
        { 5859,-211,-930,-8255,16017,2353,-1732,1887,7448 } },
    { "EOS-1Ds Mark II", 0, 0xe80,
        { 6517,-602,-867,-8180,15926,2378,-1618,1771,7633 } },
    { "EOS-1D Mark IV", 0, 0x3bb0,
        { 6014,-220,-795,-4109,12014,2361,-561,1824,5787 } },
    { "EOS-1D Mark III", 0, 0x3bb0,
        { 6291,-540,-976,-8350,16145,2311,-1714,1858,7326 } },
    { "EOS-1D Mark II N", 0, 0xe80,
        { 6240,-466,-822,-8180,15825,2500,-1801,1938,8042 } },
    { "EOS-1D Mark II", 0, 0xe80,
        { 6264,-582,-724,-8312,15948,2504,-1744,1919,8664 } },
    { "EOS-1DS", 0, 0xe20,
        { 4374,3631,-1743,-7520,15212,2472,-2892,3632,8161 } },
    { "EOS-1D C", 0, 0x3c4e,
        { 6847,-614,-1014,-4669,12737,2139,-1197,2488,6846 } },
    { "EOS-1D X", 0, 0x3c4e,
        { 6847,-614,-1014,-4669,12737,2139,-1197,2488,6846 } },
    { "EOS-1D", 0, 0xe20,
        { 6806,-179,-1020,-8097,16415,1687,-3267,4236,7690 } },
    
    /* end of list */
    { NULL, 0, 0, { 0,0,0,0,0,0,0,0,0 } },
};

static const struct {
    unsigned short id;
    char model[20];
} unique[] = {
    { 0x168, "EOS 10D" },    { 0x001, "EOS-1D" },
    { 0x175, "EOS 20D" },    { 0x174, "EOS-1D Mark II" },
    { 0x234, "EOS 30D" },    { 0x232, "EOS-1D Mark II N" },
    { 0x190, "EOS 40D" },    { 0x169, "EOS-1D Mark III" },
    { 0x261, "EOS 50D" },    { 0x281, "EOS-1D Mark IV" },
    { 0x287, "EOS 60D" },    { 0x167, "EOS-1DS" },
    { 0x325, "EOS 70D" },
    { 0x170, "EOS 300D" },   { 0x188, "EOS-1Ds Mark II" },
    { 0x176, "EOS 450D" },   { 0x215, "EOS-1Ds Mark III" },
    { 0x189, "EOS 350D" },   { 0x324, "EOS-1D C" },
    { 0x236, "EOS 400D" },   { 0x269, "EOS-1D X" },
    { 0x252, "EOS 500D" },   { 0x213, "EOS 5D" },
    { 0x270, "EOS 550D" },   { 0x218, "EOS 5D Mark II" },
    { 0x286, "EOS 600D" },   { 0x285, "EOS 5D Mark III" },
    { 0x301, "EOS 650D" },   { 0x302, "EOS 6D" },
    { 0x326, "EOS 700D" },   { 0x250, "EOS 7D" },
    { 0x254, "EOS 1000D" },
    { 0x288, "EOS 1100D" },
    { 0x346, "EOS 100D" },
    { 0x331, "EOS M" },
};

static int* trans_to_calib(const short* trans)
{
    int* calib = calloc(18, sizeof(int));
    int i=0;
    for(i=0;i<9;++i)
    {
        calib[i*2] = trans[i];
        calib[(i*2)+1] = 10000;
    }
    return calib;
}

static int find_camera_id(const char * model)
{
    for(int i=0; table[i].prefix; ++i)
    {
        if(strcmp(model, table[i].prefix) == 0)
        {
            return i;
        }
    }
    
    return -1;
}

int get_raw_info(const char * model, struct raw_info * raw_info)
{
    printf("Camera          : Canon %s", model);
    
    int i = find_camera_id(model);
    if (i == -1)
    {
        printf(" (unknown, assuming 5D Mark III)");
        model = "EOS 5D Mark III";
        i = find_camera_id(model);
    }
    
    printf("\n");
    
    adobe_coeff("Canon", model);

    int* calib = trans_to_calib(table[i].trans);
    memcpy(raw_info->color_matrix1, calib, 18*sizeof(int));
    free(calib);
    
    return 0;
}
