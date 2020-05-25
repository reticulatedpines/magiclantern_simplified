/**
 * Emulation of Canon's image processing engine
 * (EDMAC, PREPRO, JPCORE and so on)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "eos.h"
#include "engine.h"
#include "model_list.h"
#include "eos_utils.h"

#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))

/* http://www.developpez.net/forums/d544518/c-cpp/c/equivalent-randn-matlab-c/#post3241904 */

#define TWOPI (6.2831853071795864769252867665590057683943387987502) /* 2 * pi */
 
/* 
   RAND is a macro which returns a pseudo-random numbers from a uniform
   distribution on the interval [0 1]
*/
#define RAND (rand())/((double) RAND_MAX)
 
/* 
   RANDN is a macro which returns a pseudo-random numbers from a normal
   distribution with mean zero and standard deviation one. This macro uses Box
   Muller's algorithm
*/
#define RANDN (sqrt(-2.0*log(RAND))*cos(TWOPI*RAND))


/* adapted from cr2hdr */
static void * load_pgm(const char * filename, int * p_width, int * p_height)
{
    /* try to open using dcraw */
    char dcraw_cmd[1000];
    snprintf(dcraw_cmd, sizeof(dcraw_cmd), "dcraw -4 -E -c -t 0 \"%s\"", filename);

    FILE* fp = popen(dcraw_cmd, "r");
    assert(fp);

    /* PGM read code from dcraw */
      int dim[3]={0,0,0}, comment=0, number=0, error=0, nd=0, c;

      if (fgetc(fp) != 'P' || fgetc(fp) != '5') error = 1;
      while (!error && nd < 3 && (c = fgetc(fp)) != EOF) {
        if (c == '#')  comment = 1;
        if (c == '\n') comment = 0;
        if (comment) continue;
        if (isdigit(c)) number = 1;
        if (number) {
          if (isdigit(c)) dim[nd] = dim[nd]*10 + c -'0';
          else if (isspace(c)) {
            number = 0;  nd++;
          } else error = 1;
        }
      }

    if (error || nd < 3)
    {
        pclose(fp);
        printf("dcraw output is not a valid PGM file\n");
        return 0;
    }

    int width = *p_width = dim[0];
    int height = *p_height = dim[1];

    void * buf = malloc(width * height * 2);
    assert(buf);
    int size = fread(buf, 1, width * height * 2, fp);
    assert(size == width * height * 2);
    pclose(fp);

    /* PGM is big endian, need to reverse it */
    reverse_bytes_order(buf, width * height * 2);

    return buf;
}

/* Canon's 14-bit raw format (from ML raw.h) */
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
} __attribute__((packed,aligned(2)));

#define SET_PA(x) { int v = (x); p->a = v; }
#define SET_PB(x) { int v = (x); p->b_lo = v; p->b_hi = v >> 12; }
#define SET_PC(x) { int v = (x); p->c_lo = v; p->c_hi = v >> 10; }
#define SET_PD(x) { int v = (x); p->d_lo = v; p->d_hi = v >> 8; }
#define SET_PE(x) { int v = (x); p->e_lo = v; p->e_hi = v >> 6; }
#define SET_PF(x) { int v = (x); p->f_lo = v; p->f_hi = v >> 4; }
#define SET_PG(x) { int v = (x); p->g_lo = v; p->g_hi = v >> 2; }
#define SET_PH(x) { int v = (x); p->h = v; }

static void raw_pack14(uint16_t* buf16, struct raw_pixblock * buf14, int N)
{
    fprintf(stderr, "[CAPTURE] Packing %d pixels to 14-bit...\n", N);

    for (int i = 0; i < N; i += 8)
    {
        struct raw_pixblock * p = buf14 + i/8;
        SET_PA(buf16[i]);
        SET_PB(buf16[i+1]);
        SET_PC(buf16[i+2]);
        SET_PD(buf16[i+3]);
        SET_PE(buf16[i+4]);
        SET_PF(buf16[i+5]);
        SET_PG(buf16[i+6]);
        SET_PH(buf16[i+7]);
    }
}

static void gen_dark_frame(EOSState *s, void * buf, int requested_width, int requested_height)
{
    fprintf(stderr, "[CAPTURE] Generating a %dx%d dark frame...\n", requested_width, requested_height);

    uint16_t * buf16 = malloc(requested_width * requested_height * 2);
    assert(buf16);

    for (int i = 0; i < requested_width * requested_height; i++)
    {
        /* todo: use model-specific white level,
         * simulate ISO noise, pattern noise, hot pixels... */
        buf16[i] = 2048 + RANDN * 8;
    }

    raw_pack14(buf16, buf, requested_width * requested_height);
    free(buf16);
}

/* inp: malloc'd buffer (will be freed)
 * w, h: input size
 * W, H: output size
 * returns: output buffer (malloc'd)
 */
static uint16_t * pad_pgm(uint16_t * inp, int w, int h, int W, int H)
{
    fprintf(stderr, "[CAPTURE] Padding from %dx%d to %dx%d...\n", w, h, W, H);
    uint16_t * out = malloc(W * H * 2);
    assert(out);
 
    for (int yo = 0; yo < H; yo++)
    {
        for (int xo = 0; xo < W; xo++)
        {
            /* assume the right and bottom offsets are 0 (5D3) */
            int xi = COERCE(xo - (W - w), 0, w - 1);
            int yi = COERCE(yo - (H - h), 0, h - 1);
            uint16_t p = inp[xi + yi * w];
            out[xo + yo * W] = p;
        }
    }
    free(inp);
    return out;
}

static void load_fullres_14bit_raw(EOSState *s, void * buf, int requested_width, int requested_height)
{
    /* todo: autodetect all DNG files and cycle between them */
    const char * user_filename = getenv("QEMU_EOS_VRAM_PH_QR_RAW");

    const char * filename = (user_filename && user_filename[0])
        ? user_filename
        : eos_get_cam_path(s, "VRAM/PH-QR/RAW-000.DNG");

    fprintf(stderr, "[CAPTURE] Loading photo raw data from %s (expecting %dx%d)...\n",
        filename, requested_width, requested_height
    );
    int width, height;
    uint16_t * pgm = load_pgm(filename, &width, &height);
    if (pgm)
    {
        if (width != requested_width ||
            height != requested_height)
        {
            /* pgm will be reallocated here */
            pgm = pad_pgm(pgm, width, height, requested_width, requested_height);
        }
    
        raw_pack14(pgm, buf, requested_width * requested_height);
        free(pgm); pgm = 0;
    }
    else
    {
        fprintf(stderr, "[CAPTURE] %s load error\n", filename);
        gen_dark_frame(s, buf, requested_width, requested_height);
    }
}

unsigned int eos_handle_cartridge ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    io_log("Cartridge", s, address, type, value, 0, 0, 0, 0);
    return 0;
}

static void edmac_trigger_interrupt(EOSState* s, int channel, int delay)
{
    /* from register_interrupt calls */
    const int edmac_interrupts[] = {
        0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x6D, 0xC0, 0x00, /* write channels 0..6, one unused position */
        0x5D, 0x5E, 0x5F, 0x6E, 0xC1, 0xC8, 0x00, 0x00, /* read channels 0..5, two unused positions */
        0xF9, 0x83, 0x8A, 0xCA, 0xCB, 0xD2, 0xD3, 0x00, /* write channels 7..13, one unused position */
        0x8B, 0x92, 0xE2, 0x95, 0x96, 0x97, 0x00, 0x00, /* read channels 6..11, two unused positions */
        0xDA, 0xDB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* write channels 14..15, 6 unused positions */
        0x9D, 0x9E, 0x9F, 0xA5, 0x00, 0x00, 0x00, 0x00, /* read channels 12..15, 4 unused positions */
    };

#if 0
    for (int i = 0; i < COUNT(edmac_interrupts); i++)
    {
        int isr = edmac_interrupts[i];
        if (isr)
        {
            fprintf(stderr, "    [0x%02X] = \"EDMAC#%d\",\n", isr, i);
        }
    }
    exit(1);
#endif

    assert(channel >= 0 && channel < COUNT(edmac_interrupts));
    assert(edmac_interrupts[channel]);
    
    eos_trigger_int(s, edmac_interrupts[channel], delay);
}

static int edmac_fix_off1(EOSState *s, int32_t off)
{
    /* the value is signed, but the number of bits is model-dependent */
    int off1_bits = (s->model->digic_version <= 4) ? 17 : 
                    (s->model->digic_version == 5) ? 19 : 0;
    assert(off1_bits);
    return off << (32-off1_bits) >> (32-off1_bits);
}

static int edmac_fix_off2(EOSState *s, int32_t off)
{
    /* the value is signed, but the number of bits is model-dependent */
    int off2_bits = (s->model->digic_version <= 4) ? 28 : 
                    (s->model->digic_version == 5) ? 32 : 0;
    assert(off2_bits);
    return off << (32-off2_bits) >> (32-off2_bits);
}

static char * edmac_format_size_3(
    int x, int off
)
{
    static char buf[32];
    snprintf(buf, sizeof(buf),
        off ? "%d, skip %d" : x ? "%d" : "",
        x, off
    );
    return buf;
}

static char * edmac_format_size_2(
    int y, int x,
    int off1, int off2
)
{
    static char buf[64]; buf[0] = 0;
    if (off1 == off2)
    {
        char * inner = edmac_format_size_3(x, off1);
        snprintf(buf, sizeof(buf),
            y == 0 ? "%s" : strchr(inner, ' ') ? "(%s) x %d" : "%sx%d",
            inner, y+1
        );
    }
    else
    {
        /* y may be executed never, once or many times */
        if (y)
        {
            char * inner1 = edmac_format_size_3(x, off1);
            snprintf(buf, sizeof(buf),
                y == 0 ? "%s" : strchr(inner1, ' ') ? "(%s) x %d" : "%sx%d",
                inner1, y
            );
        }
        
        /* y is executed once */
        char * inner2 = edmac_format_size_3(x, off2);
        STR_APPEND(buf, "%s%s", buf[0] && inner2[0] ? ", " : "", inner2);
    }
    return buf;
}

static char * edmac_format_size_1(
    int y, int xn, int xa, int xb,
    int off1a, int off1b, int off2, int off3
)
{
    static char buf[128]; buf[0] = 0;
    if (xa == xb && off1a == off1b && off2 == off3)
    {
        char * inner = edmac_format_size_2(y, xa, off1a, off2);
        snprintf(buf, sizeof(buf),
            xn == 0 ? "%s" : strchr(inner, ' ') ? "(%s) x %d" : "%sx%d",
            inner, xn+1
        );
    }
    else
    {
        /* xa may be executed never, once or many times */
        if (xn)
        {
            char * inner1 = edmac_format_size_2(y, xa, off1a, off2);
            snprintf(buf, sizeof(buf),
                xn == 1 ? "%s" : strchr(inner1, ' ') ? "(%s) x %d" : "%sx%d",
                inner1, xn
            );
        }
        
        /* xb is executed once */
        char * inner2 = edmac_format_size_2(y, xb, off1b, off3);
        STR_APPEND(buf, "%s%s",
            !(buf[0] && inner2[0]) ? "" :   /* no separator needed */
            strlen(buf) > 20 && strlen(inner2) > 20 ? ",\n  " : ", ",   /* newline for long strings */
            inner2
        );
    }
    return buf;
}

static char * edmac_format_size(
    int yn, int ya, int yb, int xn, int xa, int xb,
    int off1a, int off1b, int off2a, int off2b, int off3
)
{
#if 0
    const char * names[] = { "yn", "ya", "yb", "xn", "xa", "xb", "off1a", "off1b", "off2a", "off2b", "off3" };
    int values[] = { yn, ya, yb, xn, xa, xb, off1a, off1b, off2a, off2b, off3 };
    int len = 0;
    for (int i = 0; i < COUNT(values); i++)
        if (values[i])
            len += fprintf(stderr, "%s=%d, ", names[i], values[i]);
    fprintf(stderr, "\b\b: ");
    for (int i = 0; i < 45 - len; i++)
        fprintf(stderr, " ");
    if (len > 45)
        fprintf(stderr, "\n  ");
#endif

    static char buf[256]; buf[0] = 0;
    
    if (ya == yb && off2a == off2b)
    {
        char * inner = edmac_format_size_1(ya, xn, xa, xb, off1a, off1b, off2a, off3);
        snprintf(buf, sizeof(buf),
            yn == 0 ? "%s" : strchr(inner, ' ') ? "(%s) x %d" : "%sx%d",
            inner, yn+1
        );
    }
    else
    {
        /* ya may be executed never, once or many times */
        if (yn)
        {
            char * inner1 = edmac_format_size_1(ya, xn, xa, xb, off1a, off1b, off2a, off3);
            snprintf(buf, sizeof(buf),
                yn == 1 ? "%s" : strchr(inner1, ' ') ? "(%s) x %d" : "%sx%d",
                inner1, yn
            );
        }
        
        /* yb is executed once */
        /* setting the last offset to off1b usually simplifies the formula */
        char * inner2 = edmac_format_size_1(yb, xn, xa, xb, off1a, off1b, off2b, off3 ? off3 : off1b);
        STR_APPEND(buf, "%s%s",
            !(buf[0] && inner2[0]) ? "" :   /* no separator needed */
            strlen(buf) > 20 && strlen(inner2) > 20 ? ",\n  " : ", ",   /* newline for long strings */
            inner2
        );
    }
    return buf;
}

static void edmac_test_format_size(void)
{
    return;

    fprintf(stderr, "EDMAC format tests:\n");
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 0x1df, 0, 0, 0x2d0,      0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 0x1df, 0, 0, 0x2d0,      0, 100, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 0, 0x1df, 0x2d0, 0x2d0,  0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 0, 0x1000, 0x1000, 0,    0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 0, 0x12, 0x1000, 0xC00,  0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 0xfff, 0x7, 0x20, 0x20,  0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 0xb3f, 0x2, 0xf0, 0xf0,  0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 0, 1055, 3276, 32,       0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 10, 95, 3276, 3276,      0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(3, 0,  7, 95, 3276, 3276,      0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(33,0, 62, 10, 3276, 3276,      0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(5, 2,  3, 10, 3276, 3276,      0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(3, 6,  5, 10, 3276, 3276,      0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(3, 7,  5, 10, 3276, 3276,      0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(8, 9,  7, 10, 3276, 3276,      0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(3, 28, 8, 10, 3276, 3276,      0, 0, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(3, 28, 8, 10, 3276, 3276,      0, 100, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(3, 28, 8, 10, 3276, 3276,      44, 100, 0, 0, 0));
    fprintf(stderr, "%s\n", edmac_format_size(3, 28, 8, 10, 3276, 1638,      44, 100, 172, 196, 1236));
    fprintf(stderr, "%s\n", edmac_format_size(0, 0, 3839, 2, 768, 768,       0x2a00, 0x2a00, 0, 0xfd5d2d00, 0));
    fprintf(stderr, "%s\n", edmac_format_size(137, 7, 7, 15, 320, 320,      -320,-320,-320,-320,-320));
    fprintf(stderr, "%s\n", edmac_format_size(479, 0, 0, 9, 40, 40,          360, 360, 32, 32, 32));
    exit(1);
}

/* 1 on success, 0 = no data available yet (should retry) */
static int edmac_do_transfer(EOSState *s, int channel)
{
    /* not fully implemented */
    fprintf(stderr, "[EDMAC#%d] Starting transfer %s 0x%X %s ", channel,
        (channel & 8) ? "from" : "to",
        s->edmac.ch[channel].addr,
        (channel & 8) ? "to" : "from"
    );
    
    uint32_t conn = 0;
    
    if (channel & 8)
    {
        /* read channel */
        for (int c = 0; c < COUNT(s->edmac.read_conn); c++)
        {
            if (s->edmac.read_conn[c] == channel)
            {
                /* can a read operation have multiple destinations? */
                /* if yes, we don't handle that case yet */
                assert(conn == 0);
                conn = c;
            }
        }
    }
    else
    {
        conn = s->edmac.write_conn[channel];
    }
    
    fprintf(stderr, "<%d>, ", conn);

    /* Hypothesis
     * ==========
     * 
     * (
     *    ((xa, skip off1a) * ya, xa, skip off2a) * xn
     *     (xb, skip off1b) * ya, xb, skip off3
     * ) * yn,
     * 
     * (
     *    ((xa, skip off1a) * yb, xa, skip off2b) * xn
     *     (xb, skip off1b) * yb, xb, skip off3
     * )
     * 
     */

    int xa = s->edmac.ch[channel].xa;
    int ya = s->edmac.ch[channel].ya;
    int xb = s->edmac.ch[channel].xb;
    int yb = s->edmac.ch[channel].yb;
    int xn = s->edmac.ch[channel].xn;
    int yn = s->edmac.ch[channel].yn;
    int off1a = edmac_fix_off1(s, s->edmac.ch[channel].off1a);
    int off1b = edmac_fix_off1(s, s->edmac.ch[channel].off1b);
    int off2a = edmac_fix_off2(s, s->edmac.ch[channel].off2a);
    int off2b = edmac_fix_off2(s, s->edmac.ch[channel].off2b);
    int off3  = edmac_fix_off2(s, s->edmac.ch[channel].off3);
    int flags = edmac_fix_off2(s, s->edmac.ch[channel].flags);
    
    fprintf(stderr, "%s, ", edmac_format_size(yn, ya, yb, xn, xa, xb, off1a, off1b, off2a, off2b, off3));

    fprintf(stderr, "flags=0x%X\n", flags);

    /* actual amount of data transferred */
    uint32_t transfer_data_size =
        (xa * (ya+1) * xn + xb * (ya+1)) * yn +
        (xa * (yb+1) * xn + xb * (yb+1));
    
    /* total size covered, including offsets */
    uint32_t transfer_data_skip_size =
        (((xa + off1a) * ya + xa + off2a) * xn +
         ((xb + off1b) * ya + xb + off3)) * yn +
        (((xa + off1a) * yb + xa + off2b) * xn +
          (xb + off1b) * yb + xb + off3);

    /* we must have some valid address configured */
    assert(s->edmac.ch[channel].addr);

    if (channel & 8)
    {
        /* from memory to image processing modules */
        uint32_t src = s->edmac.ch[channel].addr;
        
        /* repeated transfers will append to existing buffer */
        uint32_t old_size = s->edmac.conn_data[conn].data_size;
        uint32_t new_size = old_size + transfer_data_size;
        if (s->edmac.conn_data[conn].buf)
        {
            fprintf(stderr, "[EDMAC] <%d>: data size %d -> %d.\n",
                conn, old_size, new_size
            );
        }
        s->edmac.conn_data[conn].buf = realloc(s->edmac.conn_data[conn].buf, new_size );
        void * dst = s->edmac.conn_data[conn].buf + old_size;
        s->edmac.conn_data[conn].data_size = new_size;
        
        for (int jn = 0; jn <= yn; jn++)
        {
            int y     = (jn < yn) ? ya    : yb;
            int off2  = (jn < yn) ? off2a : off2b;
            for (int in = 0; in <= xn; in++)
            {
                int x     = (in < xn) ? xa    : xb;
                int off1  = (in < xn) ? off1a : off1b;
                int off23 = (in < xn) ? off2  : off3;
                for (int j = 0; j <= y; j++)
                {
                    int off = (j < y) ? off1 : off23;
                    eos_mem_read(s, src, dst, x);
                    src += x + off;
                    dst += x;
                }
            }
        }
        fprintf(stderr, "[EDMAC#%d] %d bytes read from %X-%X.\n", channel, transfer_data_size, s->edmac.ch[channel].addr, s->edmac.ch[channel].addr + transfer_data_skip_size);
    }
    else
    {
        /* from image processing modules to memory */
        uint32_t dst = s->edmac.ch[channel].addr;
        
        if (conn == 0 || conn == 35)
        {
            /* sensor data? */
            s->edmac.conn_data[conn].buf = malloc(transfer_data_size);
            assert(s->edmac.conn_data[conn].buf);
            s->edmac.conn_data[conn].data_size = transfer_data_size;

            if (xb == 0x20)
            {
                /* not sure what's up with that - just zero it out */
                fprintf(stderr, "[CAPTURE] FIXME: what should we do here?\n");
                memset(s->edmac.conn_data[conn].buf, 0, transfer_data_size);
            }
            else
            {
                /* assume either "Simplest WxH" or "xa, xb, xn" - http://www.magiclantern.fm/forum/index.php?topic=18315.0 */
                int raw_width = xb * 8/14;
                int raw_height = yb ? yb + 1 : xn + 1;
                assert(raw_width * raw_height * 14/8 == transfer_data_size);
                load_fullres_14bit_raw(s, s->edmac.conn_data[conn].buf, raw_width, raw_height);
            }
        }
        else if (conn == 6 || conn == 7)
        {
            /* pass-through; wait for data sent by some other channel on the same connection */
            /* nothing to do here */
        }
        else
        {
            fprintf(stderr, "[ENGINE] FIXME: returning dummy data on <%d>\n", conn);
            s->edmac.conn_data[conn].buf = malloc(transfer_data_size);
            assert(s->edmac.conn_data[conn].buf);
            s->edmac.conn_data[conn].data_size = transfer_data_size;
            memset(s->edmac.conn_data[conn].buf, 0, transfer_data_size);
        }

        if (s->edmac.conn_data[conn].data_size < transfer_data_size)
        {
            fprintf(stderr, "[EDMAC#%d] Data %s; will try again later.\n", channel,
                s->edmac.conn_data[conn].data_size ? "incomplete" : "unavailable"
            );
            return 0;
        }
        assert(s->edmac.conn_data[conn].buf);

        void * src = s->edmac.conn_data[conn].buf;

        for (int jn = 0; jn <= yn; jn++)
        {
            int y     = (jn < yn) ? ya    : yb;
            int off2  = (jn < yn) ? off2a : off2b;
            for (int in = 0; in <= xn; in++)
            {
                int x     = (in < xn) ? xa    : xb;
                int off1  = (in < xn) ? off1a : off1b;
                int off23 = (in < xn) ? off2  : off3;
                for (int j = 0; j <= y; j++)
                {
                    int off = (j < y) ? off1 : off23;
                    eos_mem_write(s, dst, src, x);
                    src += x;
                    dst += x + off;
                }
            }
        }
        assert(src - s->edmac.conn_data[conn].buf == transfer_data_size);
        assert(dst - s->edmac.ch[channel].addr == transfer_data_skip_size);

        uint32_t old_size = s->edmac.conn_data[conn].data_size;
        uint32_t new_size = old_size - transfer_data_size;

        if (new_size)
        {
            /* only copied some of the data to memory;
             * shift the remaining data for use with subsequent transfers
             * (a little slow, kinda reinventing a FIFO)
             */
            memmove(s->edmac.conn_data[conn].buf, s->edmac.conn_data[conn].buf + transfer_data_size, new_size);
            s->edmac.conn_data[conn].buf = realloc(s->edmac.conn_data[conn].buf, new_size);
            s->edmac.conn_data[conn].data_size = new_size;
            fprintf(stderr, "[EDMAC] <%d>: data size %d -> %d.\n",
                conn, old_size, new_size
            );
        }
        else
        {
            free(s->edmac.conn_data[conn].buf);
            s->edmac.conn_data[conn].buf = 0;
            s->edmac.conn_data[conn].data_size = 0;
        }
        
        fprintf(stderr, "[EDMAC#%d] %d bytes written to %X-%X.\n", channel, transfer_data_size, s->edmac.ch[channel].addr, s->edmac.ch[channel].addr + transfer_data_skip_size);
    }

    /* return end address when reading back the register */
    s->edmac.ch[channel].addr += transfer_data_skip_size;

    /* assume 200 MB/s transfer speed */
    int delay = transfer_data_size * 1e6 / (200*1024*1024) / 0x100;

    if (channel & 8)
    {
        /* additional delay for read channels */
        /* required by to pass the memory benchmark (?!) */
        delay++;
    }

    fprintf(stderr, "[EDMAC#%d] transfer delay %d x 256 us.\n", channel, delay);

    edmac_trigger_interrupt(s, channel, delay);
    return 1;
}

static void prepro_execute(EOSState *s)
{
    if (s->prepro.adkiz_intr_en)
    {
        /* appears to use data from connections 8 and 15 */
        /* are these hardcoded? */
        if (s->edmac.conn_data[8].buf && s->edmac.conn_data[15].buf)
        {
            fprintf(stderr, "[ADKIZ] Dummy operation.\n");
            
            /* "consume" the data from those two connections */
            assert(s->edmac.conn_data[8].buf);
            assert(s->edmac.conn_data[15].buf);

            /* free it to allow the next operation */
            free(s->edmac.conn_data[8].buf);
            s->edmac.conn_data[8].buf = 0;
            s->edmac.conn_data[8].data_size = 0;
            
            free(s->edmac.conn_data[15].buf);
            s->edmac.conn_data[15].buf = 0;
            s->edmac.conn_data[15].data_size = 0;
            
            eos_trigger_int(s, 0x65, 1);
        }
        else
        {
            fprintf(stderr, "[ADKIZ] Data unavailable; will try again later.\n");
        }
    }
    
    if (s->prepro.hiv_enb == 0 || s->prepro.hiv_enb == 1)
    {
        if (s->edmac.conn_data[15].buf)
        {
            fprintf(stderr, "[HIV] Dummy operation.\n");
            assert(s->edmac.conn_data[15].buf);
            free(s->edmac.conn_data[15].buf);
            s->edmac.conn_data[15].buf = 0;
            s->edmac.conn_data[15].data_size = 0;
        }
        else
        {
            fprintf(stderr, "[HIV] Data unavailable; will try again later.\n");
        }
    }

    if (s->prepro.def_enb == 1)
    {
        if (s->edmac.conn_data[1].buf)
        {
            int transfer_size = s->edmac.conn_data[1].data_size;
            int old_size = s->edmac.conn_data[16].data_size;
            int new_size = old_size + transfer_size;
            fprintf(stderr, "[DEF] Dummy operation (copy %d bytes from <1> to <16>).\n", transfer_size);
            if (old_size) fprintf(stderr, "[DEF] Data size %d -> %d.\n", old_size, new_size);
            s->edmac.conn_data[16].buf = realloc(s->edmac.conn_data[16].buf, new_size);
            s->edmac.conn_data[16].data_size = new_size;
            memcpy(s->edmac.conn_data[16].buf + old_size, s->edmac.conn_data[1].buf, transfer_size);
            free(s->edmac.conn_data[1].buf);
            s->edmac.conn_data[1].buf = 0;
            s->edmac.conn_data[1].data_size = 0;
        }
    }
}

unsigned int eos_handle_edmac ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    unsigned int ret = 0;
    int channel = (parm << 4) | ((address >> 8) & 0xF);
    assert(channel < COUNT(s->edmac.ch));
    
    switch(address & 0xFF)
    {
        case 0x00:
            msg = "control/status";
            if (value == 1)
            {
                if (channel & 8)
                {
                    /* read channel: data is always available */
                    assert(edmac_do_transfer(s, channel));
                    
                    /* any pending requests on other channels?
                     * data might be available now */
                    for (int ch = 0; ch < COUNT(s->edmac.pending); ch++)
                    {
                        if (s->edmac.pending[ch])
                        {
                            if (edmac_do_transfer(s, ch))
                            {
                                s->edmac.pending[ch] = 0;
                            }
                        }
                    }
                    
                    /* some image processing modules may now have input data */
                    prepro_execute(s);
                }
                else
                {
                    /* write channel: data may or may not be available right now */
                    if (!edmac_do_transfer(s, channel))
                    {
                        /* didn't work; schedule it for later */
                        s->edmac.pending[channel] = 1;
                    }
                }
            }
            break;

        case 0x04:
            msg = "flags";
            MMIO_VAR(s->edmac.ch[channel].flags);
            break;

        case 0x08:
            msg = "RAM address";
            MMIO_VAR(s->edmac.ch[channel].addr);
            break;

        case 0x0C:
            msg = "yn|xn";
            MMIO_VAR_2x16(s->edmac.ch[channel].xn, s->edmac.ch[channel].yn);
            break;

        case 0x10:
            msg = "yb|xb";
            MMIO_VAR_2x16(s->edmac.ch[channel].xb, s->edmac.ch[channel].yb);
            break;

        case 0x14:
            msg = "ya|xa";
            MMIO_VAR_2x16(s->edmac.ch[channel].xa, s->edmac.ch[channel].ya);
            break;

        case 0x18:
            msg = "off1b";
            MMIO_VAR(s->edmac.ch[channel].off1b);
            break;

        case 0x1C:
            msg = "off2b";
            MMIO_VAR(s->edmac.ch[channel].off2b);
            break;

        case 0x20:
            msg = "off1a";
            MMIO_VAR(s->edmac.ch[channel].off1a);
            break;

        case 0x24:
            msg = "off2a";
            MMIO_VAR(s->edmac.ch[channel].off2a);
            break;

        case 0x28:
            msg = "off3";
            MMIO_VAR(s->edmac.ch[channel].off3);
            break;

        case 0x40:
            msg = "off40";
            MMIO_VAR(s->edmac.ch[channel].off40);
            break;

        case 0x30:
            msg = "interrupt reason?";
            if(type & MODE_WRITE)
            {
            }
            else
            {
                /* read channels:
                 *   0x02 = normal?
                 *   0x10 = abort?
                 * write channels:
                 *   0x01 = normal? (used with PackMem)
                 *   0x02 = normal?
                 *   0x04 = pop?
                 *   0x10 = abort?
                 */
                int pop_request = (s->edmac.ch[channel].flags & 0xF) == 6;
                int abort_request = (s->edmac.ch[channel].off34 == 3);
                ret = (abort_request) ? 0x10 :
                      (pop_request)   ? 0x04 :
                      (channel & 8)   ? 0x02 :
                                        0x01 ;
            }
            break;

        case 0x34:
            msg = "off34";
            MMIO_VAR(s->edmac.ch[channel].off34);

            if(type & MODE_WRITE)
            {
                if (s->edmac.ch[channel].off34 == 3)
                {
                    msg = "abort request";
                    edmac_trigger_interrupt(s, channel, 1);
                }
            }
            break;
    }
    
    char name[32];
    snprintf(name, sizeof(name), "EDMAC#%d", channel);
    io_log(name, s, address, type, value, ret, msg, 0, 0);
    return ret;
}

/* EDMAC channel switch (connections) */
unsigned int eos_handle_edmac_chsw ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    int msg_arg1 = 0;
    int msg_arg2 = 0;
    unsigned int ret = 0;
    
    /* fixme: reads not implemented */
    if (!(type & MODE_WRITE))
    {
        /* apparently these reads are just bugs in Canon code */
        msg = KLRED"unexpected read"KRESET;
        // assert(0);
        goto end;
    }

    if (value == 0x80000000)
    {
        /* ?! used on M3 */
        goto end;
    }

    /* 0xC0F05020 - 0xC0F050E0: read edmac connections */
    /* 0xC0F05000 - 0xC0F0501C: write channel connections for channels 0-6, then 16 */
    /* 0xC0F05200 - 0xC0F05240: write channel connections for channels > 16 */
    switch(address & 0xFFF)
    {
        case 0x020 ... 0x0E0:
        {
            /* read channels  8...13 =>  0...5 */
            /* read channels 24...29 =>  6...11 */
            /* read channels 40...43 => 12...15 */
            uint32_t conn = ((address & 0xFF) - 0x20) >> 2;
            uint32_t ch = 
                (value <=  5) ? value + 8      :
                (value <= 11) ? value + 16 + 2 :
                (value <= 15) ? value + 32 - 4 : -1 ;
            assert(conn < COUNT(s->edmac.conn_data));
            assert(ch < COUNT(s->edmac.ch));
            s->edmac.read_conn[conn] = ch;
            
            /* make sure this mapping is unique */
            /* (not sure how it's supposed to work, but...) */
            for (int c = 0; c < COUNT(s->edmac.read_conn); c++)
            {
                if (c != conn && s->edmac.read_conn[c] == ch)
                {
                    fprintf(stderr, "[CHSW] Warning: disabling RD#%d -> <%d>.\n", ch, c);
                    s->edmac.read_conn[c] = 0;
                }
            }
            
            msg = "RAM -> RD#%d -> <%d>";
            msg_arg1 = ch;
            msg_arg2 = conn;
            break;
        }

        case 0x000 ... 0x01C:
        {
            uint32_t conn = value;
            uint32_t ch = (address & 0x1F) >> 2;
            if (ch == 7) ch = 16;
            assert(conn < COUNT(s->edmac.conn_data));
            assert(ch < COUNT(s->edmac.ch));
            s->edmac.write_conn[ch] = conn;
            msg = "<%d> -> WR#%d -> RAM";
            msg_arg1 = conn;
            msg_arg2 = ch;
            break;
        }

        case 0x200 ... 0x240:
        {
            /* write channels 17 ... 22: pos 0...5 */
            /* write channels 32 ... 33: pos 6...7 */
            uint32_t conn = value;
            uint32_t pos = (address & 0x3F) >> 2;
            uint32_t ch =
                (pos <= 5) ? pos + 16 + 1
                           : pos + 32 - 6 ;
            assert(conn < COUNT(s->edmac.conn_data));
            assert(ch < COUNT(s->edmac.ch));
            s->edmac.write_conn[ch] = conn;
            msg = "<%d> -> WR#%d -> RAM";
            msg_arg1 = conn;
            msg_arg2 = ch;
            break;
        }
    }

end:
    io_log("CHSW", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

static const char * prepro_reg_name(unsigned int addr)
{
    /* http://magiclantern.wikia.com/wiki/Register_Map#Image_PreProcessing */
    switch(addr)
    {
        case 0xC0F08000: return "DARK_ENB";
        case 0xC0F08004: return "DARK_MODE";
        case 0xC0F08008: return "DARK_SETUP";
        case 0xC0F0800C: return "DARK_LIMIT";
        case 0xC0F08010: return "DARK_SETUP_14_12";
        case 0xC0F08014: return "DARK_LIMIT_14_12";
        case 0xC0F08018: return "DARK_SAT_LIMIT";

        case 0xC0F08020: return "SHAD_ENB";
        case 0xC0F08024: return "SHAD_MODE";
        case 0xC0F08028: return "SHADE_PRESETUP";
        case 0xC0F0802C: return "SHAD_POSTSETUP";
        case 0xC0F08030: return "SHAD_GAIN";
        case 0xC0F08034: return "SHAD_PRESETUP_14_12";
        case 0xC0F08038: return "SHAD_POSTSETUP_14_12";

        case 0xC0F08040: return "TWOADD_ENB";
        case 0xC0F08044: return "TWOADD_MODE";
        case 0xC0F08048: return "TWOADD_SETUP";
        case 0xC0F0804C: return "TWOADD_LIMIT";
        case 0xC0F08050: return "TWOADD_SETUP_14_12";
        case 0xC0F08054: return "TWOADD_LIMIT_14_12";
        case 0xC0F08058: return "TWOADD_SAT_LIMIT";

        case 0xC0F08060: return "DSUNPACK_ENB";
        case 0xC0F08064: return "DSUNPACK_MODE";

        case 0xC0F08070: return "UNPACK24_ENB";
        case 0xC0F08074: return "UNPACK24_MODE ";

        case 0xC0F08080: return "ADUNPACK_ENB";
        case 0xC0F08084: return "ADUNPACK_MODE";

        case 0xC0F08090: return "PACK32_ENB";
        case 0xC0F08094: return "PACK32_MODE";

        case 0xC0F080A0: return "DEF_ENB";
        case 0xC0F080A4: return "DEF_unk1";
        case 0xC0F080A8: return "DEF_DEF_MODE";
        case 0xC0F080AC: return "DEF_DEF_CTRL";
        case 0xC0F080B0: return "DEF_YB_XB";
        case 0xC0F080B4: return "DEF_YN_XN";
        case 0xC0F080BC: return "DEF_YA_XA?";
        case 0xC0F080C0: return "DEF_BUF_NUM";
        case 0xC0F080C4: return "DEF_INTR_BE";
        case 0xC0F080C8: return "DEF_INTR_AE";
        case 0xC0F080D0: return "DEF_INTR_NUM/DEF_INTR_EN?";
        case 0xC0F080D4: return "DEF_HOSEI";

        case 0xC0F08100: return "CCDSEL";
        case 0xC0F08104: return "DS_SEL";
        case 0xC0F08108: return "OBWB_ISEL";
        case 0xC0F0810C: return "PROC24_ISEL";
        case 0xC0F08110: return "DPCME_ISEL";
        case 0xC0F08114: return "PACK32_ISEL";

        case 0xC0F08120: return "PACK16_ENB";
        case 0xC0F08124: return "PACK16_MODE";

        case 0xC0F08130: return "DEFM_ENB";
        case 0xC0F08134: return "DEFM_unk1";
        case 0xC0F08138: return "DEFM_MODE";
        case 0xC0F08140: return "DEFM_INTR_NUM";
        case 0xC0F0814C: return "DEFM_GRADE";
        case 0xC0F08150: return "DEFM_DAT_TH";
        case 0xC0F08154: return "DEFM_INTR_CLR";
        case 0xC0F08158: return "DEFM_INTR_EN";
        case 0xC0F0815C: return "DEFM_14_12_SEL";
        case 0xC0F08160: return "DEFM_DAT_TH_14_12";
        case 0xC0F0816C: return "DEFM_X2MODE";

        case 0xC0F08180: return "HIV_ENB";
        case 0xC0F08184: return "HIV_V_SIZE";
        case 0xC0F08188: return "HIV_H_SIZE";
        case 0xC0F0818C: return "HIV_POS_V_OFST";
        case 0xC0F08190: return "HIV_POS_H_OFST";
        case 0xC0F08194: return "HIV_unk1";
        case 0xC0F08198: return "HIV_unk2";
        case 0xC0F0819C: return "HIV_POST_SETUP";
        case 0xC0F081C0: return "HIV_H_SWS_ENB";
        case 0xC0F08214: return "HIV_PPR_EZ";
        case 0xC0F08218: return "HIV_IN_SEL";

        case 0xC0F08220: return "ADKIZ_unk1";
        case 0xC0F08224: return "ADKIZ_THRESHOLD";
        case 0xC0F08234: return "ADKIZ_TOTAL_SIZE";
        case 0xC0F08238: return "ADKIZ_INTR_CLR";
        case 0xC0F0823C: return "ADKIZ_INTR_EN";
        case 0xC0F08240: return "ADMERG_INTR_EN";
        case 0xC0F08244: return "ADMERG_TOTAL_SIZE";
        case 0xC0F08248: return "ADMERG_MergeDefectsCount";
        case 0xC0F08250: return "ADMERG_2_IN_SE";
        case 0xC0F0825C: return "ADKIZ_THRESHOLD_14_12";

        case 0xC0F08260: return "UNPACK24_DM_EN";
        case 0xC0F08264: return "PACK32_DM_EN";
        case 0xC0F08268: return "PACK32_MODE_H";
        case 0xC0F0826C: return "unk_MODE_H";
        case 0xC0F08270: return "DEFC_X2MODE";
        case 0xC0F08274: return "DSUNPACK_DM_EN";
        case 0xC0F08278: return "ADUNPACK_DM_EN";
        case 0xC0F0827C: return "PACK16_CCD2_DM_EN";

        case 0xC0F08280: return "SHAD_CBIT";
        case 0xC0F08284: return "SHAD_C8MODE";
        case 0xC0F08288: return "SHAD_C12MODE";
        case 0xC0F0828C: return "SHAD_RMODE";
        case 0xC0F08290: return "SHAD_COF_SEL";

        case 0xC0F082A0: return "DARK_KZMK_SAV_A";
        case 0xC0F082A4: return "DARK_KZMK_SAV_B";
        case 0xC0F082A8: return "SHAD_KZMK_SAV";
        case 0xC0F082AC: return "TWOA_KZMK_SAV_A";
        case 0xC0F082B0: return "TWOA_KZMK_SAV_B";
        case 0xC0F082B4: return "DEFC_DET_MODE";
        case 0xC0F082B8: return "PACK16_DEFM_ON";
        case 0xC0F082BC: return "PACK32_DEFM_ON";
        case 0xC0F082C4: return "HIV_DEFMARK_CANCEL";

        case 0xC0F082D0: return "PACK16_ISEL";
        case 0xC0F082D4: return "WDMAC32_ISEL";
        case 0xC0F082D8: return "WDMAC16_ISEL";
        case 0xC0F082DC: return "OBINTG_ISEL";
        case 0xC0F082E0: return "AFFINE_ISEL";
        case 0xC0F08390: return "OBWB_ISEL2";
        case 0xC0F08394: return "PROC24_ISEL2";
        case 0xC0F08398: return "PACK32_ISEL2";
        case 0xC0F0839C: return "PACK16_ISEL2";
        case 0xC0F083A0: return "TAIWAN_ISEL";

        case 0xC0F08420: return "HIV_BASE_OFST";
        case 0xC0F08428: return "HIV_GAIN_DIV";
        case 0xC0F0842C: return "HIV_PATH";

        case 0xC0F08540: return "RSHD_ENB";

        case 0xC0F085B0: return "PACK32_ILIM";
        case 0xC0F085B4: return "PACK16_ILIM";
    }
    
    return NULL;
}

unsigned int eos_handle_prepro ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = prepro_reg_name(address);
    unsigned int ret = 0;

    switch (address & 0xFFF)
    {
        case 0x240:     /* ADMERG_INTR_EN */
            MMIO_VAR(s->prepro.adkiz_intr_en);
            break;
        
        case 0x0C8:     /* DEF_INTR_AE */
            if(type & MODE_WRITE)
            {
            }
            else
            {
                int AdKizDet_flag = (s->model->digic_version == 4) ? 0x20 : 
                                    (s->model->digic_version == 5) ? 0x10 : 0;
                assert(AdKizDet_flag);
                ret = AdKizDet_flag;   /* Interruppt AdKizDet */
            }
            break;
        
        case 0x180:     /* HIV_ENB */
            MMIO_VAR(s->prepro.hiv_enb);
            break;

        case 0x120:     /* PACK16_ENB */
            MMIO_VAR(s->prepro.pack16_enb);
            break;

        case 0x060:     /* DSUNPACK_ENB */
            MMIO_VAR(s->prepro.dsunpack_enb);
            break;

        case 0x0A0:     /* DEF_ENB */
            MMIO_VAR(s->prepro.def_enb);
            break;
    }

    io_log("PREPRO", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_jpcore( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * module_name = 0;
    const char * msg = 0;
    unsigned int ret = 0;

    switch (parm)
    {
        case 0:
        {
            /* used for JPEG and old-style lossless compression (TTJ) */
            module_name = "JP51";
            break;
        }

        case 1:
        {
            /* used for H.264 */
            module_name = "JP62";
            break;
        }

        case 2:
        {
            /* used for new-style lossless compression (TTL) */
            module_name = "JP57";
            break;
        }
    }

    /* common */
    switch(address & 0xFFFF)
    {
        case 0x0000:
            msg = "control/status?";
            if(type & MODE_WRITE)
            {
                if (value & 1)
                {
                    msg = "Start JPCORE";
                    eos_trigger_int(s, 0x64, 10);
                }
            }
            else
            {
                /* EOSM: this value starts JPCORE, but fails the DCIM test */
                //ret = 0x1010000;

                if (strcmp(s->model->name, "1300D") == 0)
                {
                    /* 1300D requires it */
                    ret = 0x1010000;
                }
            }
            break;

        case 0x0004:
            msg = "mode? (encode, decode etc)";
            break;

        case 0x000C:
            msg = "operation mode?";
            break;

        case 0x0010 ... 0x001C:
            msg = "JPEG tags, packed";
            break;

        case 0x0024:
            msg = "output size";
            break;

        case 0x0030:
            msg = "JPEGIC status?";
            ret = 0x1FF;
            break;

        case 0x0040:
            msg = "set to 0x600";
            break;

        case 0x0044:
            msg = "interrupt status? (70D loop)";
            ret = rand();

            if (strcmp(s->model->name, "1300D") == 0)
            {
                ret = 0x400;
            }
            break;

        case 0x0080:
            msg = "slice size";
            break;

        case 0x0084:
            msg = "number of components and bit depth";
            break;

        case 0x008C:
            msg = "component subsampling";
            break;

        case 0x0094:
            msg = "sample properties? for SOS header?";
            break;

        case 0x00E8:
            msg = "slice width";
            break;

        case 0x0EC:
            msg = "slice height";
            break;

        case 0x0928:
            msg = "JpCoreFirm";
            break;

        case 0x092C:
            msg = "JpCoreExtStream";
            break;
    }

    io_log(module_name, s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_head ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = prepro_reg_name(address);
    unsigned int ret = 0;
    
    uint32_t bases[] = { 0, 0xC0F07048, 0xC0F0705C, 0xC0F07134, 0xC0F07148 };
    uint32_t base = bases[parm];
    
    uint32_t interrupts[] = { 0, 0x6A, 0x6B, 0xD9, 0xE0 };
    
    switch (address - base)
    {
        case 0:
            msg = "Enable?";
            break;
        case 4:
            msg = "Start?";
            if(type & MODE_WRITE)
            {
                if (value == 0xC)
                {
                    eos_trigger_int(s, interrupts[parm], 50);
                }
            }
            break;
        case 8:
            msg = "Timer ticks";
            break;
    }

    char name[32];
    snprintf(name, sizeof(name), "HEAD%d", parm);
    io_log(name, s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_engio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    io_log("ENGIO", s, address, type, value, 0, 0, 0, 0);
    return 0;
}

void engine_init(void)
{
    edmac_test_format_size();
}
