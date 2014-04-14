#include "string.h"
#include "stdio.h"

/* Convert between Temperature and RGB.
 * Base on information from http://www.brucelindbloom.com/
 * The fit for D-illuminant between 4000K and 23000K are from CIE
 * The generalization to 2000K < T < 4000K and the blackbody fits
 * are my own and should be taken with a grain of salt.
 */
static const double XYZ_to_RGB[3][3] = {
    { 3.24071,	-0.969258,  0.0556352 },
    { -1.53726,	1.87599,    -0.203996 },
    { -0.498571,	0.0415557,  1.05707 }
};

static void Temperature_to_RGB(double T, double RGB[3])
{
    int c;
    double xD, yD, X, Y, Z, max;
    // Fit for CIE Daylight illuminant
    if (T <= 4000) {
        xD = 0.27475e9 / (T * T * T) - 0.98598e6 / (T * T) + 1.17444e3 / T + 0.145986;
    } else if (T <= 7000) {
        xD = -4.6070e9 / (T * T * T) + 2.9678e6 / (T * T) + 0.09911e3 / T + 0.244063;
    } else {
        xD = -2.0064e9 / (T * T * T) + 1.9018e6 / (T * T) + 0.24748e3 / T + 0.237040;
    }
    yD = -3 * xD * xD + 2.87 * xD - 0.275;

    // Fit for Blackbody using CIE standard observer function at 2 degrees
    //xD = -1.8596e9/(T*T*T) + 1.37686e6/(T*T) + 0.360496e3/T + 0.232632;
    //yD = -2.6046*xD*xD + 2.6106*xD - 0.239156;

    // Fit for Blackbody using CIE standard observer function at 10 degrees
    //xD = -1.98883e9/(T*T*T) + 1.45155e6/(T*T) + 0.364774e3/T + 0.231136;
    //yD = -2.35563*xD*xD + 2.39688*xD - 0.196035;

    X = xD / yD;
    Y = 1;
    Z = (1 - xD - yD) / yD;
    max = 0;
    for (c = 0; c < 3; c++) {
        RGB[c] = X * XYZ_to_RGB[0][c] + Y * XYZ_to_RGB[1][c] + Z * XYZ_to_RGB[2][c];
        if (RGB[c] > max) max = RGB[c];
    }
    for (c = 0; c < 3; c++) RGB[c] = RGB[c] / max;
}

static void RGB_to_Temperature(double RGB[3], double *T, double *Green)
{
    double Tmax, Tmin, testRGB[3];
    Tmin = 2000;
    Tmax = 23000;
    for (*T = (Tmax + Tmin) / 2; Tmax - Tmin > 0.1; *T = (Tmax + Tmin) / 2) {
        Temperature_to_RGB(*T, testRGB);
        if (testRGB[2] / testRGB[0] > RGB[2] / RGB[0])
            Tmax = *T;
        else
            Tmin = *T;
    }
    *Green = (testRGB[1] / testRGB[0]) / (RGB[1] / RGB[0]);
    if (*Green < 0.2) *Green = 0.2;
    if (*Green > 2.5) *Green = 2.5;
}

static void pseudoinverse (double (*in)[3], double (*out)[3], int size);

void ufraw_kelvin_green_to_multipliers(double temperature, double green, double chanMulArray[3])
{
    /* color matrices from dcraw */
    extern float pre_mul[4], rgb_cam[3][4];
    double rgbWB[3];
    int c, cc, i, j;

    double cam_rgb[3][3];
    double rgb_cam_transpose[4][3];
    for (i = 0; i < 4; i++) for (j = 0; j < 3; j++)
            rgb_cam_transpose[i][j] = rgb_cam[j][i];

    pseudoinverse(rgb_cam_transpose, cam_rgb, 3);

    /* For uf_manual_wb we calculate chanMul from the temperature/green. */
    {
        Temperature_to_RGB(temperature, rgbWB);
        rgbWB[1] = rgbWB[1] / green;
        /* Suppose we shot a white card at some temperature:
         * rgbWB[3] = rgb_cam[3][4] * preMul[4] * camWhite[4]
         * Now we want to make it white (1,1,1), so we replace preMul
         * with chanMul, which is defined as:
         * chanMul[4][4] = cam_rgb[4][3] * (1/rgbWB[3][3]) * rgb_cam[3][4]
         *		* preMul[4][4]
         * We "upgraded" preMul, chanMul and rgbWB to diagonal matrices.
         * This allows for the manipulation:
         * (1/chanMul)[4][4] = (1/preMul)[4][4] * cam_rgb[4][3] * rgbWB[3][3]
         *		* rgb_cam[3][4]
         * We use the fact that rgb_cam[3][4] * (1,1,1,1) = (1,1,1) and get:
         * (1/chanMul)[4] = (1/preMul)[4][4] * cam_rgb[4][3] * rgbWB[3]
         */
        if (0) {
            /* If there is no color matrix it is simple */
            for (c = 0; c < 3; c++)
                chanMulArray[c] = pre_mul[c] / rgbWB[c];
        } else {
            for (c = 0; c < 3; c++) {
                double chanMulInv = 0;
                for (cc = 0; cc < 3; cc++)
                    chanMulInv += 1 / pre_mul[c] * cam_rgb[c][cc]
                                  * rgbWB[cc];
                chanMulArray[c] = 1 / chanMulInv;
            }
        }
        
        /* normalize green multiplier */
        chanMulArray[0] /= chanMulArray[1];
        chanMulArray[2] /= chanMulArray[1];
        chanMulArray[1] = 1;
    }
}

void ufraw_multipliers_to_kelvin_green(double chanMulArray[3], double* temperature, double* green)
{
    int c, cc;
    double rgbWB[3];

    /* color matrices from dcraw */
    extern float pre_mul[4], rgb_cam[3][4];

    /* (1/chanMul)[4] = (1/preMul)[4][4] * cam_rgb[4][3] * rgbWB[3]
     * Therefore:
     * rgbWB[3] = rgb_cam[3][4] * preMul[4][4] * (1/chanMul)[4]
     */
    if (0) {
        /* If there is no color matrix it is simple */
        for (c = 0; c < 3; c++) {
            rgbWB[c] = pre_mul[c] / chanMulArray[c];
        }
    } else {
        for (c = 0; c < 3; c++) {
            rgbWB[c] = 0;
            for (cc = 0; cc < 3; cc++)
                rgbWB[c] += rgb_cam[c][cc] * pre_mul[cc]
                            / chanMulArray[cc];
        }
    }
    /* From these values we calculate temperature, green values */
    RGB_to_Temperature(rgbWB, temperature, green);
}

/* Routines copied from dcraw */

#define CLASS

#if !defined(uchar)
#define uchar unsigned char
#endif
#if !defined(ushort)
#define ushort unsigned short
#endif

float cam_mul[4], pre_mul[4], cmatrix[3][4], rgb_cam[3][4];

const double xyz_rgb[3][3] = {                        /* XYZ from RGB */
  { 0.412453, 0.357580, 0.180423 },
  { 0.212671, 0.715160, 0.072169 },
  { 0.019334, 0.119193, 0.950227 } };

int black, maximum;
unsigned raw_color;
int colors = 3;


static void CLASS pseudoinverse (double (*in)[3], double (*out)[3], int size)
{
  double work[3][6], num;
  int i, j, k;

  for (i=0; i < 3; i++) {
    for (j=0; j < 6; j++)
      work[i][j] = j == i+3;
    for (j=0; j < 3; j++)
      for (k=0; k < size; k++)
        work[i][j] += in[k][i] * in[k][j];
  }
  for (i=0; i < 3; i++) {
    num = work[i][i];
    for (j=0; j < 6; j++)
      work[i][j] /= num;
    for (k=0; k < 3; k++) {
      if (k==i) continue;
      num = work[k][i];
      for (j=0; j < 6; j++)
        work[k][j] -= work[i][j] * num;
    }
  }
  for (i=0; i < size; i++)
    for (j=0; j < 3; j++)
      for (out[i][j]=k=0; k < 3; k++)
        out[i][j] += work[j][k+3] * in[i][k];
}

void CLASS cam_xyz_coeff (double cam_xyz[4][3])
{
  double cam_rgb[4][3], inverse[4][3], num;
  int i, j, k;

  for (i=0; i < colors; i++)                /* Multiply out XYZ colorspace */
    for (j=0; j < 3; j++)
      for (cam_rgb[i][j] = k=0; k < 3; k++)
        cam_rgb[i][j] += cam_xyz[i][k] * xyz_rgb[k][j];

  for (i=0; i < colors; i++) {                /* Normalize cam_rgb so that */
    for (num=j=0; j < 3; j++)                /* cam_rgb * (1,1,1) is (1,1,1,1) */
      num += cam_rgb[i][j];
    for (j=0; j < 3; j++)
      cam_rgb[i][j] /= num;
    pre_mul[i] = 1 / num;
  }
  pseudoinverse (cam_rgb, inverse, colors);
  for (raw_color = i=0; i < 3; i++)
    for (j=0; j < colors; j++)
      rgb_cam[i][j] = inverse[j][i];
}

/* fixme: duplicate code also in dcraw-bridge.c */

/*
   All matrices are from Adobe DNG Converter unless otherwise noted.
 */
void CLASS adobe_coeff (const char *make, const char *model)
{
  static const struct {
    const char *prefix;
    short black, maximum, trans[12];
  } table[] = {
    { "Canon EOS 5D Mark III", 0, 0x3c80,
        { 6722,-635,-963,-4287,12460,2028,-908,2162,5668 } },
    { "Canon EOS 5D Mark II", 0, 0x3cf0,
        { 4716,603,-830,-7798,15474,2480,-1496,1937,6651 } },
    { "Canon EOS 5D", 0, 0xe6c,
        { 6347,-479,-972,-8297,15954,2480,-1968,2131,7649 } },
    { "Canon EOS 6D", 0, 0x3c82,
        { 7034,-804,-1014,-4420,12564,2058,-851,1994,5758 } },
    { "Canon EOS 7D", 0, 0x3510,
        { 6844,-996,-856,-3876,11761,2396,-593,1772,6198 } },
    { "Canon EOS 10D", 0, 0xfa0,
        { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
    { "Canon EOS 20Da", 0, 0,
        { 14155,-5065,-1382,-6550,14633,2039,-1623,1824,6561 } },
    { "Canon EOS 20D", 0, 0xfff,
        { 6599,-537,-891,-8071,15783,2424,-1983,2234,7462 } },
    { "Canon EOS 30D", 0, 0,
        { 6257,-303,-1000,-7880,15621,2396,-1714,1904,7046 } },
    { "Canon EOS 40D", 0, 0x3f60,
        { 6071,-747,-856,-7653,15365,2441,-2025,2553,7315 } },
    { "Canon EOS 50D", 0, 0x3d93,
        { 4920,616,-593,-6493,13964,2784,-1774,3178,7005 } },
    { "Canon EOS 60D", 0, 0x2ff7,
        { 6719,-994,-925,-4408,12426,2211,-887,2129,6051 } },
    { "Canon EOS 300D", 0, 0xfa0,
        { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
    { "Canon EOS 350D", 0, 0xfff,
        { 6018,-617,-965,-8645,15881,2975,-1530,1719,7642 } },
    { "Canon EOS 400D", 0, 0xe8e,
        { 7054,-1501,-990,-8156,15544,2812,-1278,1414,7796 } },
    { "Canon EOS 450D", 0, 0x390d,
        { 5784,-262,-821,-7539,15064,2672,-1982,2681,7427 } },
    { "Canon EOS 500D", 0, 0x3479,
        { 4763,712,-646,-6821,14399,2640,-1921,3276,6561 } },
    { "Canon EOS 550D", 0, 0x3dd7,
        { 6941,-1164,-857,-3825,11597,2534,-416,1540,6039 } },
    { "Canon EOS 600D", 0, 0x3510,
        { 6461,-907,-882,-4300,12184,2378,-819,1944,5931 } },
    { "Canon EOS 650D", 0, 0x354d,
        { 6602,-841,-939,-4472,12458,2247,-975,2039,6148 } },
    { "Canon EOS 1000D", 0, 0xe43,
        { 6771,-1139,-977,-7818,15123,2928,-1244,1437,7533 } },
    { "Canon EOS 1100D", 0, 0x3510,
        { 6444,-904,-893,-4563,12308,2535,-903,2016,6728 } },
    { "Canon EOS M", 0, 0,
        { 6602,-841,-939,-4472,12458,2247,-975,2039,6148 } },
    { "Canon EOS-1Ds Mark III", 0, 0x3bb0,
        { 5859,-211,-930,-8255,16017,2353,-1732,1887,7448 } },
    { "Canon EOS-1Ds Mark II", 0, 0xe80,
        { 6517,-602,-867,-8180,15926,2378,-1618,1771,7633 } },
    { "Canon EOS-1D Mark IV", 0, 0x3bb0,
        { 6014,-220,-795,-4109,12014,2361,-561,1824,5787 } },
    { "Canon EOS-1D Mark III", 0, 0x3bb0,
        { 6291,-540,-976,-8350,16145,2311,-1714,1858,7326 } },
    { "Canon EOS-1D Mark II N", 0, 0xe80,
        { 6240,-466,-822,-8180,15825,2500,-1801,1938,8042 } },
    { "Canon EOS-1D Mark II", 0, 0xe80,
        { 6264,-582,-724,-8312,15948,2504,-1744,1919,8664 } },
    { "Canon EOS-1DS", 0, 0xe20,
        { 4374,3631,-1743,-7520,15212,2472,-2892,3632,8161 } },
    { "Canon EOS-1D X", 0, 0x3c4e,
        { 6847,-614,-1014,-4669,12737,2139,-1197,2488,6846 } },
    { "Canon EOS-1D", 0, 0xe20,
        { 6806,-179,-1020,-8097,16415,1687,-3267,4236,7690 } },
    { "Canon EOS", 0, 0,
        { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
  };
  double cam_xyz[4][3];
  char name[130];
  int i, j;

  sprintf (name, "%s %s", make, model);
  for (i=0; i < sizeof table / sizeof *table; i++)
    if (!strncmp (name, table[i].prefix, strlen(table[i].prefix))) {
      printf("Camera model    : %s\n", table[i].prefix);
      if (table[i].black)   black   = (ushort) table[i].black;
      if (table[i].maximum) maximum = (ushort) table[i].maximum;
      if (table[i].trans[0]) {
        for (j=0; j < 12; j++)
          cam_xyz[0][j] = table[i].trans[j] / 10000.0;
        cam_xyz_coeff (cam_xyz);
      }
      break;
    }
}
