#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "lv_rec.h"
#include "../../src/raw.h"

lv_rec_file_footer_t lv_rec_footer;
struct raw_info raw_info;

#define FAIL(fmt,...) { fprintf(stderr, "Error: "); fprintf(stderr, fmt, ## __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }
#define CHECK(ok, fmt,...) { if (!ok) FAIL(fmt, ## __VA_ARGS__); }

static void fix_vertical_stripes();


int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf(
            "\n"
            "usage:\n"
            "\n"
            "%s file.raw [prefix]\n"
            "\n"
            " => will create prefix000000.dng, prefix0000001.dng and so on.\n"
            "\n",
            argv[0]
        );
        return 1;
    }
    
    FILE* fi = fopen(argv[1], "rb");
    CHECK(fi, "could not open %s", argv[1]);
    if (sizeof(lv_rec_file_footer_t) != 192) FAIL("sizeof(lv_rec_file_footer_t) = %d, should be 192", sizeof(lv_rec_file_footer_t));
    
    fseeko(fi, -192, SEEK_END);
    int r = fread(&lv_rec_footer, 1, sizeof(lv_rec_file_footer_t), fi);
    CHECK(r == sizeof(lv_rec_file_footer_t), "footer");
    raw_info = lv_rec_footer.raw_info;
    fseek(fi, 0, SEEK_SET);

    if (strncmp((char*)lv_rec_footer.magic, "RAWM", 4))
        FAIL("This ain't a lv_rec RAW file\n");
    
    if (raw_info.api_version != 1)
        FAIL("API version mismatch: %d\n", raw_info.api_version);
    
    /* override params here (e.g. when the footer is from some other file) */
    //~ lv_rec_footer.xRes=2048;
    //~ lv_rec_footer.yRes=1024;
    //~ lv_rec_footer.frameSize = lv_rec_footer.xRes * lv_rec_footer.yRes * 14/8;
    //~ lv_rec_footer.raw_info.white_level = 16383;
    
    printf("Resolution  : %d x %d\n", lv_rec_footer.xRes, lv_rec_footer.yRes);
    printf("Frames      : %d\n", lv_rec_footer.frameCount);
    printf("Frame size  : %d bytes\n", lv_rec_footer.frameSize);
    printf("Frame skip  : %d\n", lv_rec_footer.frameSkip);
    printf("FPS         : %d.%03d\n", lv_rec_footer.sourceFpsx1000/1000, lv_rec_footer.sourceFpsx1000%1000);
    
    char* raw = malloc(lv_rec_footer.frameSize);
    CHECK(raw, "malloc");
    
    /* override the resolution from raw_info with the one from lv_rec_footer, if they don't match */
    if (lv_rec_footer.xRes != raw_info.width)
    {
        raw_info.width = lv_rec_footer.xRes;
        raw_info.pitch = raw_info.width * 14/8;
        raw_info.active_area.x1 = 0;
        raw_info.active_area.x2 = raw_info.width;
        raw_info.jpeg.x = 0;
        raw_info.jpeg.width = raw_info.width;
    }

    if (lv_rec_footer.yRes != raw_info.height)
    {
        raw_info.height = lv_rec_footer.yRes;
        raw_info.active_area.y1 = 0;
        raw_info.active_area.y2 = raw_info.height;
        raw_info.jpeg.y = 0;
        raw_info.jpeg.height = raw_info.height;
    }
    
    raw_info.frame_size = lv_rec_footer.frameSize;
    
    char* prefix = argc > 2 ? argv[2] : "";

    int i;
    for (i = 0; i < lv_rec_footer.frameCount; i++)
    {
        printf("\rProcessing frame %d of %d...", i+1, lv_rec_footer.frameCount);
        fflush(stdout);
        int r = fread(raw, 1, lv_rec_footer.frameSize, fi);
        CHECK(r == lv_rec_footer.frameSize, "fread");
        raw_info.buffer = raw;
        
        /* uncomment if the raw file is recovered from a DNG with dd */
        //~ reverse_bytes_order(raw, lv_rec_footer.frameSize);
        
        char fn[100];
        snprintf(fn, sizeof(fn), "%s%06d.dng", prefix, i);
        fix_vertical_stripes();
        save_dng(fn);
    }
    fclose(fi);
    printf("\nDone.\n");
    printf("\nTo convert to jpg, you can try: \n");
    printf("    ufraw-batch --out-type=jpg %s*.dng\n", prefix);
    printf("\nTo get a mjpeg video: \n");
    printf("    ffmpeg -i %s%%6d.jpg -vcodec mjpeg -qscale 1 video.avi\n\n", prefix);
    return 0;
}

int raw_get_pixel(int x, int y) {
    struct raw_pixblock * p = (void*)raw_info.buffer + y * raw_info.pitch + (x/8)*14;
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

/**
 * Fix vertical stripes (banding) from 5D Mark III (and maybe others).
 * 
 * These stripes are periodic, they repeat every 8 pixels.
 * It looks like some columns have different luma amplification;
 * correction factors are somewhere around 0.98 - 1.02, maybe camera-specific, maybe depends on
 * certain settings, I have no idea. So, this fix compares luma values within one pixel block,
 * computes the correction factors (using median to reject outliers) and decides
 * whether to apply the correction or not.
 * 
 * For speed reasons:
 * - Correction factors are computed from the first frame only.
 * - Only channels with error greater than 0.2% are corrected.
 */

static int stripes_coeffs[8] = {0};
static int stripes_correction_needed = 0;

#define MIN(a,b) \
   ({ typeof ((a)+(b)) _a = (a); \
      typeof ((a)+(b)) _b = (b); \
     _a < _b ? _a : _b; })

#define MIN_DUMB(a,b) ((a) < (b) ? (a) : (b))

#define MAX(a,b) \
   ({ typeof ((a)+(b)) _a = (a); \
       typeof ((a)+(b)) _b = (b); \
     _a > _b ? _a : _b; })

#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))

#define PA ((int)(p->a))
#define PB ((int)(p->b_lo | (p->b_hi << 12)))
#define PC ((int)(p->c_lo | (p->c_hi << 10)))
#define PD ((int)(p->d_lo | (p->d_hi << 8)))
#define PE ((int)(p->e_lo | (p->e_hi << 6)))
#define PF ((int)(p->f_lo | (p->f_hi << 4)))
#define PG ((int)(p->g_lo | (p->g_hi << 2)))
#define PH ((int)(p->h))

#define SET_PA(x) { int v = (x); p->a = v; }
#define SET_PB(x) { int v = (x); p->b_lo = v; p->b_hi = v >> 12; }
#define SET_PC(x) { int v = (x); p->c_lo = v; p->c_hi = v >> 10; }
#define SET_PD(x) { int v = (x); p->d_lo = v; p->d_hi = v >> 8; }
#define SET_PE(x) { int v = (x); p->e_lo = v; p->e_hi = v >> 6; }
#define SET_PF(x) { int v = (x); p->f_lo = v; p->f_hi = v >> 4; }
#define SET_PG(x) { int v = (x); p->g_lo = v; p->g_hi = v >> 2; }
#define SET_PH(x) { int v = (x); p->h = v; }

#define RAW_MUL(p, x) MIN((((int)(p) - raw_info.black_level) * (int)(x) / 8192) + raw_info.black_level, 16383)
#define F2H(x) COERCE(x - (8192-1024), 0, 2047)
#define H2F(x) ((x) + (8192-1024))

static void add_pixel(int hist[8][2048], int num[8], int offset, int pa, int pb, int pc)
{
    int a = pa;
    int b = pb;
    int c = pc;
    
    if (MIN(MIN(a,b),c) < 8)
        return; /* too noisy */

    if (MAX(MAX(a,b),c) > raw_info.white_level / 1.5)
        return; /* too bright */
    
    /* a . b . x . x . c */
    /* assume the transition from a to b to c should be linear (in EV) */
    double gradient = log2((double)c/a);
    b = b * pow(2, -gradient/4);
    
    /* compute correction factor for b, that brings it back on the a-c line */
    int factor = a * 8192 / b;    
    
    if (factor < 8192-1024 || factor > 8192+1024)
        return; /* this ain't banding */

    /* add to histogram */
    int weight = log2(a);
    hist[offset][F2H(factor)] += weight;
    num[offset] += weight;
}


static void detect_vertical_stripes_coeffs()
{
    /* could be a little more memory efficient if we limit coefficient range to something like 0.8 - 1.2 */
    static int hist[8][2048];
    static int num[8];
    
    memset(hist, 0, sizeof(hist));
    memset(num, 0, sizeof(num));

    /* compute 8 little histograms */
    struct raw_pixblock * row;
    for (row = raw_info.buffer; (void*)row < (void*)raw_info.buffer + raw_info.frame_size; row += raw_info.pitch / sizeof(struct raw_pixblock))
    {
        struct raw_pixblock * p;
        for (p = row; (void*)p < (void*)row + raw_info.pitch - sizeof(struct raw_pixblock);)
        {
            int pa = PA - raw_info.black_level;
            int pb = PB - raw_info.black_level;
            int pc = PC - raw_info.black_level;
            int pd = PD - raw_info.black_level;
            int pe = PE - raw_info.black_level;
            int pf = PF - raw_info.black_level;
            int pg = PG - raw_info.black_level;
            int ph = PH - raw_info.black_level;
            p++;
            int pa2 = PA - raw_info.black_level;
            int pb2 = PB - raw_info.black_level;
            int pc2 = PC - raw_info.black_level;
            int pd2 = PD - raw_info.black_level;
            int pe2 = PE - raw_info.black_level;
            int pf2 = PF - raw_info.black_level;
            //~ int pg2 = PG - raw_info.black_level;
            //~ int ph2 = PH - raw_info.black_level;
            
            /* verification: introducing strong banding in one column
             * should not affect the coefficients from the other columns */

            //~ pe = pe * 1.1;
            //~ pe2 = pe2 * 1.1;
            
            add_pixel(hist, num, 2, pa, pc, pa2);
            add_pixel(hist, num, 3, pb, pd, pb2);
            add_pixel(hist, num, 4, pc, pe, pc2);
            add_pixel(hist, num, 5, pd, pf, pd2);
            add_pixel(hist, num, 6, pe, pg, pe2);
            add_pixel(hist, num, 7, pf, ph, pf2);
        }
    }
    
    /* compute the median correction factor (this will reject outliers) */
    int j,k;
    for (j = 0; j < 8; j++)
    {
        int t = 0;
        for (k = 0; k < 2048; k++)
        {
            t += hist[j][k];
            if (t >= num[j]/2)
            {
                int c = H2F(k);
                stripes_coeffs[j] = c;
                break;
            }
        }
    }

    /* make all the coefficients relative to x[0] and x[1] */
    
    stripes_coeffs[0] = 8192;
    stripes_coeffs[1] = 8192;
    
    /* 2 and 3 are already OK */
    
    stripes_coeffs[4] = stripes_coeffs[4] * stripes_coeffs[2] / 8192;
    stripes_coeffs[5] = stripes_coeffs[5] * stripes_coeffs[3] / 8192;

    stripes_coeffs[6] = stripes_coeffs[6] * stripes_coeffs[4] / 8192;
    stripes_coeffs[7] = stripes_coeffs[7] * stripes_coeffs[5] / 8192;

    /* do we really need stripe correction, or it won't be noticeable? or maybe it's just computation error? */
    stripes_correction_needed = 0;
    for (j = 0; j < 8; j++)
    {
        double c = (double)stripes_coeffs[j] / 8192.0;
        if (c < 0.998 || c > 1.002)
            stripes_correction_needed = 1;
    }
    
    if (stripes_correction_needed)
    {
        printf("\nVertical stripes correction:\n");
        for (j = 0; j < 8; j++)
        {
            if (stripes_coeffs[j])
                printf("  %.3f", (double)stripes_coeffs[j] / 8192.0);
            else
                printf("    1  ");
        }
        printf("\n");
    }
}

static void apply_vertical_stripes_correction()
{
    struct raw_pixblock * row;
    for (row = raw_info.buffer; (void*)row < (void*)raw_info.buffer + raw_info.frame_size; row += raw_info.pitch / sizeof(struct raw_pixblock))
    {
        struct raw_pixblock * p;
        for (p = row; (void*)p < (void*)row + raw_info.pitch; p++)
        {
            if (stripes_coeffs[0]) SET_PA(RAW_MUL(PA, stripes_coeffs[0]));
            if (stripes_coeffs[1]) SET_PB(RAW_MUL(PB, stripes_coeffs[1]));
            if (stripes_coeffs[2]) SET_PC(RAW_MUL(PC, stripes_coeffs[2]));
            if (stripes_coeffs[3]) SET_PD(RAW_MUL(PD, stripes_coeffs[3]));
            if (stripes_coeffs[4]) SET_PE(RAW_MUL(PE, stripes_coeffs[4]));
            if (stripes_coeffs[5]) SET_PF(RAW_MUL(PF, stripes_coeffs[5]));
            if (stripes_coeffs[6]) SET_PG(RAW_MUL(PG, stripes_coeffs[6]));
            if (stripes_coeffs[7]) SET_PH(RAW_MUL(PH, stripes_coeffs[7]));
        }
    }
}

static void fix_vertical_stripes()
{
    /* for speed: only detect correction factors from the first frame */
    static int first_time = 1;
    if (first_time)
    {
        detect_vertical_stripes_coeffs();
        first_time = 0;
    }
    
    /* only apply stripe correction if we need it, since it takes a little CPU time */
    if (stripes_correction_needed)
    {
        apply_vertical_stripes_correction();
    }
}
