#ifdef CHROMA_SMOOTH_2X2
#define CHROMA_SMOOTH_FUNC chroma_smooth_2x2
#define CHROMA_SMOOTH_MAX_XY_IJ 2
#define CHROMA_SMOOTH_FILTER_SIZE 5
#define CHROMA_SMOOTH_MEDIAN opt_med5
#elif defined(CHROMA_SMOOTH_3X3)
#define CHROMA_SMOOTH_FUNC chroma_smooth_3x3
#define CHROMA_SMOOTH_MAX_XY_IJ 2
#define CHROMA_SMOOTH_FILTER_SIZE 9
#define CHROMA_SMOOTH_MEDIAN opt_med9
#else
#define CHROMA_SMOOTH_FUNC chroma_smooth_5x5
#define CHROMA_SMOOTH_MAX_XY_IJ 4
#define CHROMA_SMOOTH_FILTER_SIZE 25
#define CHROMA_SMOOTH_MEDIAN opt_med25
#endif

#ifndef CHROMA_SMOOTH_TYPE
#define CHROMA_SMOOTH_TYPE uint16_t
#endif

static void CHROMA_SMOOTH_FUNC(int w, int h, CHROMA_SMOOTH_TYPE * inp, CHROMA_SMOOTH_TYPE * out, int* raw2ev, int* ev2raw, int black, int white)
{
    int x,y;

    for (y = 2+CHROMA_SMOOTH_MAX_XY_IJ; y < h-3-CHROMA_SMOOTH_MAX_XY_IJ; y += 2)
    {
        for (x = 2+CHROMA_SMOOTH_MAX_XY_IJ; x < w-2-CHROMA_SMOOTH_MAX_XY_IJ; x += 2)
        {
            /**
             * for each red pixel, compute the median value of red minus interpolated green at the same location
             * the median value is then considered the "true" difference between red and green
             * same for blue vs green
             * 
             *
             * each red pixel has 4 green neighbours, so we may interpolate as follows:
             * - mean or median(t,b,l,r)
             * - choose between mean(t,b) and mean(l,r) (idea from AHD)
             * 
             * same for blue; note that a RG/GB cell has 6 green pixels that we need to analyze
             * 2 only for red, 2 only for blue, and 2 shared
             *    g
             *   gRg
             *    gBg
             *     g
             *
             * choosing the interpolation direction seems to give cleaner results
             * the direction is choosen over the entire filtered area (so we do two passes, one for each direction, 
             * and at the end choose the one for which total interpolation error is smaller)
             * 
             * error = sum(abs(t-b)) or sum(abs(l-r))
             * 
             * interpolation in EV space (rather than linear) seems to have less color artifacts in high-contrast areas
             * 
             * we can use this filter for 3x3 RG/GB cells or 5x5
             */
            int i,j;
            int k = 0;
            int med_r[CHROMA_SMOOTH_FILTER_SIZE];
            int med_b[CHROMA_SMOOTH_FILTER_SIZE];
            
            /* first try to interpolate in horizontal direction */
            int eh = 0;
            for (i = -CHROMA_SMOOTH_MAX_XY_IJ; i <= CHROMA_SMOOTH_MAX_XY_IJ; i += 2)
            {
                for (j = -CHROMA_SMOOTH_MAX_XY_IJ; j <= CHROMA_SMOOTH_MAX_XY_IJ; j += 2)
                {
                    #ifdef CHROMA_SMOOTH_2X2
                    if (ABS(i) + ABS(j) == 4)
                        continue;
                    #endif
                    
                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                    int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                  //int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                    int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                  //int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1];
                    g2 = raw2ev[g2];
                    g3 = raw2ev[g3];
                  //g4 = raw2ev[g4];
                    g5 = raw2ev[g5];
                  //g6 = raw2ev[g6];
                    
                    int gr = (g1+g3)/2;
                    int gb = (g2+g5)/2;
                    eh += ABS(g1-g3) + ABS(g2-g5);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            /* difference from green, with horizontal interpolation */
            int drh = CHROMA_SMOOTH_MEDIAN(med_r);
            int dbh = CHROMA_SMOOTH_MEDIAN(med_b);
            
            /* next, try to interpolate in vertical direction */
            int ev = 0;
            k = 0;
            for (i = -CHROMA_SMOOTH_MAX_XY_IJ; i <= CHROMA_SMOOTH_MAX_XY_IJ; i += 2)
            {
                for (j = -CHROMA_SMOOTH_MAX_XY_IJ; j <= CHROMA_SMOOTH_MAX_XY_IJ; j += 2)
                {
                    #ifdef CHROMA_SMOOTH_2X2
                    if (ABS(i) + ABS(j) == 4)
                        continue;
                    #endif

                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                  //int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                    int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                  //int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                    int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1];
                    g2 = raw2ev[g2];
                  //g3 = raw2ev[g3];
                    g4 = raw2ev[g4];
                  //g5 = raw2ev[g5];
                    g6 = raw2ev[g6];
                    
                    int gr = (g2+g4)/2;
                    int gb = (g1+g6)/2;
                    ev += ABS(g2-g4) + ABS(g1-g6);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            /* difference from green, with vertical interpolation */
            int drv = CHROMA_SMOOTH_MEDIAN(med_r);
            int dbv = CHROMA_SMOOTH_MEDIAN(med_b);

            /* back to our filtered pixels (RG/GB cell) */
            int g1 = inp[x+1 +     y * w];
            int g2 = inp[x   + (y+1) * w];
            int g3 = inp[x-1 +   (y) * w];
            int g4 = inp[x   + (y-1) * w];
            int g5 = inp[x+2 + (y+1) * w];
            int g6 = inp[x+1 + (y+2) * w];
            
            g1 = raw2ev[g1];
            g2 = raw2ev[g2];
            g3 = raw2ev[g3];
            g4 = raw2ev[g4];
            g5 = raw2ev[g5];
            g6 = raw2ev[g6];

            /* which of the two interpolations will we choose? */
            int grv = (g2+g4)/2;
            int grh = (g1+g3)/2;
            int gbv = (g1+g6)/2;
            int gbh = (g2+g5)/2;
            int gr = ev < eh ? grv : grh;
            int gb = ev < eh ? gbv : gbh;
            int dr = ev < eh ? drv : drh;
            int db = ev < eh ? dbv : dbh;
            
            int r0 = inp[x   +     y * w];
            int b0 = inp[x+1 + (y+1) * w];

            /* if we are close to the noise floor, use both directions, beacuse otherwise it will affect the noise structure and introduce false detail */
            /* todo: smooth transition between the two methods? better thresholding condition? */
            int thr = 64;
            if (r0 < black+thr || b0 < black+thr || ABS(drv - drh) < thr || ABS(grv-grh) < thr || ABS(gbv-gbh) < thr)
            {
                dr = (drv+drh)/2;
                db = (dbv+dbh)/2;
                gr = (g1+g2+g3+g4)/4;
                gb = (g1+g2+g5+g6)/4;
            }

            /* replace red and blue pixels with filtered values, keep green pixels unchanged */
            /* don't touch overexposed areas */
            if (out[x   +     y * w] < white)
                out[x   +     y * w] = ev2raw[COERCE(gr + dr, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)];
            
            if (out[x+1  + (y+1)* w] < white)
                out[x+1 + (y+1) * w] = ev2raw[COERCE(gb + db, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)];
        }
    }
}

#undef CHROMA_SMOOTH_FUNC
#undef CHROMA_SMOOTH_MAX_XY_IJ
#undef CHROMA_SMOOTH_FILTER_SIZE
#undef CHROMA_SMOOTH_MEDIAN
