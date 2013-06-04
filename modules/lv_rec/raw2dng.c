#include "stdio.h"
#include "stdlib.h"
#include "string.h"

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

static int stripes_coeffs[8];
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

#define RAW_MUL(p, x) MIN((((int)(p) - raw_info.black_level) * (int)(x) / 1024) + raw_info.black_level, 16383)
#define FACTOR(a,b) ({ int A = ((int)(a) - raw_info.black_level); int B = ((int)(b) - raw_info.black_level); B > 32 ? (A * 1024 / B) : 1024; })

#define F2H(x) COERCE(x, 0, 2047)
#define H2F(x) (x)

static void detect_vertical_stripes_coeffs()
{
    /* could be a little more memory efficient if we limit coefficient range to something like 0.8 - 1.2 */
    static int hist[8][2048];
    
    memset(hist, 0, sizeof(hist));

    /* compute 8 little histograms */
    int n = 0;
    struct raw_pixblock * row;
    for (row = raw_info.buffer; (void*)row < (void*)raw_info.buffer + raw_info.frame_size; row += raw_info.pitch / sizeof(struct raw_pixblock))
    {
        struct raw_pixblock * p;
        for (p = row; (void*)p < (void*)row + raw_info.pitch; p++)
        {
            n++;
            hist[0][F2H(FACTOR(1,  1))]++;
            hist[1][F2H(FACTOR(1,  1))]++;
            hist[2][F2H(FACTOR(PA, PC))]++;
            hist[3][F2H(FACTOR(PB, PD))]++;
            hist[4][F2H(FACTOR(PA, PE))]++;
            hist[5][F2H(FACTOR(PB, PF))]++;
            hist[6][F2H(FACTOR(PA, PG))]++;
            hist[7][F2H(FACTOR(PB, PH))]++;
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
            if (t >= n/2)
            {
                int c = H2F(k);
                /* do we really need stripe correction, or it won't be noticeable? or maybe it's just computation error? */
                if (abs(c - 1024) <= 2)
                {
                    stripes_coeffs[j] = 0;
                }
                else
                {
                    stripes_coeffs[j] = c;
                    stripes_correction_needed = 1;
                }
                break;
            }
        }
    }
    
    if (stripes_correction_needed)
    {
        printf("\nVertical stripes correction:\n");
        for (j = 0; j < 8; j++)
        {
            if (stripes_coeffs[j])
                printf("  %.3f", (double)stripes_coeffs[j] / 1024.0);
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
