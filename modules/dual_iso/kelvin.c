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

/*
   All matrices are from Adobe DNG Converter unless otherwise noted.
 */
void CLASS adobe_coeff (const char *make, const char *model)
{
  extern const struct {
      const char *prefix;
      short black, maximum, trans[12];
  } table[];

  double cam_xyz[4][3];
  int i, j;

  for (i=0; table[i].prefix; i++)
    if(strcmp(model, table[i].prefix) == 0) {
      if (table[i].black)   black   = (ushort) table[i].black;
      if (table[i].maximum) maximum = (ushort) table[i].maximum;
      if (table[i].trans[0]) {
        for (j=0; j < 12; j++)
          cam_xyz[j/3][j%3] = table[i].trans[j] / 10000.0;    /* original: cam_xyz[0][j] = table[i].trans[j] / 10000.0; */
        cam_xyz_coeff (cam_xyz);
      }
      return;
    }

  printf("Matrix not found: %s\n", model);
}
