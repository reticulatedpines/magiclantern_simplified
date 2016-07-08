#ifdef CHROMA_SMOOTH_2X2
#define CHROMA_SMOOTH_FUNC chroma_smooth_2x2
#define CHROMA_SMOOTH_MAX_IJ 2
#define CHROMA_SMOOTH_FILTER_SIZE 5
#define CHROMA_SMOOTH_MEDIAN opt_med5
#elif defined(CHROMA_SMOOTH_3X3)
#define CHROMA_SMOOTH_FUNC chroma_smooth_3x3
#define CHROMA_SMOOTH_MAX_IJ 2
#define CHROMA_SMOOTH_FILTER_SIZE 9
#define CHROMA_SMOOTH_MEDIAN opt_med9
#else
#define CHROMA_SMOOTH_FUNC chroma_smooth_5x5
#define CHROMA_SMOOTH_MAX_IJ 4
#define CHROMA_SMOOTH_FILTER_SIZE 25
#define CHROMA_SMOOTH_MEDIAN opt_med25
#endif

static void CHROMA_SMOOTH_FUNC(uint32_t * inp, uint32_t * out, int* raw2ev, int* ev2raw)
{
    int w = raw_info.width;
    int h = raw_info.height;
    int x,y;

    for (y = 4; y < h-5; y += 2)
    {
        for (x = 4; x < w-4; x += 2)
        {
            int g1 = inp[x+1 +     y * w];
            int g2 = inp[x   + (y+1) * w];
            int ge = (raw2ev[g1] + raw2ev[g2]) / 2;
            
            /* looks ugly in darkness */
            if (ge < 2*EV_RESOLUTION) continue;

            int i,j;
            int k = 0;
            int med_r[CHROMA_SMOOTH_FILTER_SIZE];
            int med_b[CHROMA_SMOOTH_FILTER_SIZE];
            for (i = -CHROMA_SMOOTH_MAX_IJ; i <= CHROMA_SMOOTH_MAX_IJ; i += 2)
            {
                for (j = -CHROMA_SMOOTH_MAX_IJ; j <= CHROMA_SMOOTH_MAX_IJ; j += 2)
                {
                    #ifdef CHROMA_SMOOTH_2X2
                    if (ABS(i) + ABS(j) == 4)
                        continue;
                    #endif
                    
                    int r  = inp[x+i   +   (y+j) * w];
                    int g1 = inp[x+i+1 +   (y+j) * w];
                    int g2 = inp[x+i   + (y+j+1) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                    
                    int ge = (raw2ev[g1] + raw2ev[g2]) / 2;
                    med_r[k] = raw2ev[r] - ge;
                    med_b[k] = raw2ev[b] - ge;
                    k++;
                }
            }
            int dr = CHROMA_SMOOTH_MEDIAN(med_r);
            int db = CHROMA_SMOOTH_MEDIAN(med_b);

            if (ge + dr <= EV_RESOLUTION) continue;
            if (ge + db <= EV_RESOLUTION) continue;

            out[x   +     y * w] = ev2raw[COERCE(ge + dr, 0, 14*EV_RESOLUTION-1)];
            out[x+1 + (y+1) * w] = ev2raw[COERCE(ge + db, 0, 14*EV_RESOLUTION-1)];
        }
    }
}

#undef CHROMA_SMOOTH_FUNC
#undef CHROMA_SMOOTH_MAX_IJ
#undef CHROMA_SMOOTH_FILTER_SIZE
#undef CHROMA_SMOOTH_MEDIAN
