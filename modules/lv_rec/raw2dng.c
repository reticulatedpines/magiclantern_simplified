#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "lv_rec.h"
#include "../../src/raw.h"

lv_rec_file_footer_t lv_rec_footer;
struct raw_info raw_info;

#define FAIL(fmt,...) { fprintf(stderr, "Error: "); fprintf(stderr, fmt, ## __VA_ARGS__); exit(1); }
#define CHECK(ok, fmt,...) { if (!ok) FAIL(fmt, ## __VA_ARGS__); }

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
    CHECK(fi, "%s", argv[1]);
    
    fseek(fi, -sizeof(lv_rec_file_footer_t), SEEK_END);
    int r = fread(&lv_rec_footer, 1, sizeof(lv_rec_file_footer_t), fi);
    CHECK(r == sizeof(lv_rec_file_footer_t), "footer");
    raw_info = lv_rec_footer.raw_info;
    fseek(fi, 0, SEEK_SET);

    if (!strcmp((char*)&lv_rec_footer.magic, "RAW"))
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


