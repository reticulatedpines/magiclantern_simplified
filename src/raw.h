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

#ifndef _raw_h_
#define _raw_h_

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

/* call this before performing any raw image analysis */
/* returns 1=success, 0=failed */
int raw_update_params();

/* get a red/green/blue pixel near the specified coords (approximate) */
int raw_red_pixel(int x, int y);
int raw_green_pixel(int x, int y);
int raw_blue_pixel(int x, int y);

/* get/set the pixel at specified coords (exact, but you can get whatever color happens to be there) */
int raw_get_pixel(int x, int y);
int raw_set_pixel(int x, int y, int value);

/* input: 0 - 16384 (valid range: from black level to white level) */
/* output: -14 ... 0 */
float raw_to_ev(int raw);

/* save a DNG file; all parameters are taken from raw_info */
int save_dng(char* filename);

/* raw image info (geometry, calibration levels, color, DR etc); parts of this were copied from CHDK */
struct raw_info {
    void* buffer;               // points to image data
    
    int height, width, pitch;
    int frame_size;
    int bits_per_pixel;         // 14

    int black_level;            // autodetected
    int white_level;            // somewhere around 13000 - 16000, varies with camera, settings etc
                                // would be best to autodetect it, but we can't do this reliably yet
    union                       // DNG JPEG info
    {
        struct
        {
            int x, y;           // DNG JPEG top left corner
            int width, height;  // DNG JPEG size
        } jpeg;
        struct
        {
            int origin[2];
            int size[2];
        } crop;
    };
    union                       // DNG active sensor area (Y1, X1, Y2, X2)
    {
        struct
        {
            int y1, x1, y2, x2;
        } active_area;
        int dng_active_area[4];
    };
    int exposure_bias[2];       // DNG Exposure Bias (idk what's that)
    int cfa_pattern;            // stick to 0x02010100 (RGBG) if you can
    int calibration_illuminant1;
    int color_matrix1[18];      // DNG Color Matrix
    
    int dynamic_range;          // EV x10, from DxO
    int underexposure_level;    // raw level just under the dynamic_range
};

extern struct raw_info raw_info;

#endif
