/** 
 * For decoding 14-bit RAW
 * 
 **/

/**
* RAW pixels (document mode, as with dcraw -D -o 0):

    01 23 45 67 89 AB ... (SENSOR_RES_X-1)
    ab cd ef gh ab cd ...

    v-------------------------- first pixel should be red
0   RG RG RG RG RG RG ...   <-- first line (even)
1   GB GB GB GB GB GB ...   <-- second line (odd)
2   RG RG RG RG RG RG ...
3   GB GB GB GB GB GB ...
...
SENSOR_RES_Y-1
*/

/**
* 14-bit encoding:

hi          lo
aaaaaaaaaaaaaabb
bbbbbbbbbbbbcccc
ccccccccccdddddd
ddddddddeeeeeeee
eeeeeeffffffffff
ffffgggggggggggg
gghhhhhhhhhhhhhh
*/

/* group 8 pixels in 14 bytes to simplify decoding */
struct raw_pixblock
{
    unsigned int b_hi: 2;
    unsigned int a: 14;     // even lines: red; odd lines: green
    unsigned int c_hi: 4;
    unsigned int b_lo: 12;
    unsigned int d_hi: 6;
    unsigned int c_lo: 10;
    unsigned int e_hi: 8;
    unsigned int d_lo: 8;
    unsigned int f_hi: 10;
    unsigned int e_lo: 6;
    unsigned int g_hi: 12;
    unsigned int f_lo: 4;
    unsigned int h: 14;     // even lines: green; odd lines: blue
    unsigned int g_lo: 2;
} __attribute__((packed));

/* a full line of pixels */
typedef struct raw_pixblock raw_pixline[SENSOR_RES_X / 8];

/* get a red pixel near the specified coords (approximate) */
static inline int raw_red_pixel(struct raw_pixblock * buf, int x, int y)
{
    y = (y/2) * 2;
    int i = ((y * SENSOR_RES_X + x) / 8);
    return buf[i].a;
}

/* get a green pixel near the specified coords (approximate) */
static inline int raw_green_pixel(struct raw_pixblock * buf, int x, int y)
{
    y = (y/2) * 2;
    int i = ((y * SENSOR_RES_X + x) / 8);
    return buf[i].h;
}

/* get a blue pixel near the specified coords (approximate) */
static inline int raw_blue_pixel(struct raw_pixblock * buf, int x, int y)
{
    y = (y/2) * 2 + 1;
    int i = ((y * SENSOR_RES_X + x) / 8);
    return buf[i].h;
}

/* get the pixel at specified coords (exact, but you can get whatever color happens to be there) */
static int raw_get_pixel(struct raw_pixblock * buf, int x, int y) {
    struct raw_pixblock * p = (void*)buf + y * (SENSOR_RES_X*14/8) + (x/8)*14;
    switch (x%8) {
        case 0: return p->a;
        case 1: return p->b_lo | (p->b_hi << 12);
        case 2: return p->c_lo | (p->c_hi << 10);
        case 3: return p->d_lo | (p->d_hi << 8);
        case 4: return p->e_lo | (p->e_hi << 6);
        case 5: return p->f_lo | (p->f_hi << 4);
        case 6: return p->g_lo | (p->g_hi << 2);
        case 7: return p->h;
    }
    return p->a;
}

static int raw_set_pixel(struct raw_pixblock * buf, int x, int y, int value) {
    struct raw_pixblock * p = (void*)buf + y * (SENSOR_RES_X*14/8) + (x/8)*14;
    switch (x%8) {
        case 0: p->a = value; break;
        case 1: p->b_lo = value; p->b_hi = value >> 12; break;
        case 2: p->c_lo = value; p->c_hi = value >> 10; break;
        case 3: p->d_lo = value; p->d_hi = value >> 8; break;
        case 4: p->e_lo = value; p->e_hi = value >> 6; break;
        case 5: p->f_lo = value; p->f_hi = value >> 4; break;
        case 6: p->g_lo = value; p->g_hi = value >> 2; break;
        case 7: p->h = value; break;
    }
    return p->a;
}

/* input: 0 - 16384 (valid range: from black level to white level) */
/* output: -14 ... 0 */
static float raw_to_ev(int raw, int black_level, int white_level)
{
    int raw_max = white_level - black_level;
    float raw_ev = -log2f(raw_max) + log2f(COERCE(raw - black_level, 1, raw_max));
    return raw_ev;
}

static int autodetect_black_level(struct raw_pixblock * buf)
{
    /* fixme: define and use proper borders for black calibration */
    int black = 0;
    for (int i = 5; i < 10; i++)
    {
        for (int j = 5; j < 10; j++)
        {
            black += raw_get_pixel(buf, i, j);
        }
    }
    return black / 25;
}
