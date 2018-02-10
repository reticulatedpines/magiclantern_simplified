/*
 * Copyright (C) 2014 David Milligan
 * Copyright (C) 2017 Magic Lantern Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <raw.h>
#include "../mlv.h"

#include "dng.h"
#include "dng_tag_codes.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "camera_id.h"

#include "../raw_proc/pixel_proc.h"
#include "../raw_proc/stripes.h"
#include "../raw_proc/patternnoise.h"
#include "../raw_proc/histogram.h"

#define IFD0_COUNT 41
#define EXIF_IFD_COUNT 11
#define PACK(a) (((uint16_t)a[1] << 16) | ((uint16_t)a[0]))
#define PACK2(a,b) (((uint16_t)b << 16) | ((uint16_t)a))
#define STRING_ENTRY(a,b,c) (uint32_t)(strlen(a) + 1), add_string(a, b, c)
#define RATIONAL_ENTRY(a,b,c,d) (d/2), add_array(a, b, c, d)
#define RATIONAL_ENTRY2(a,b,c,d) 1, add_rational(a, b, c, d)
#define ARRAY_ENTRY(a,b,c,d) d, add_array(a, b, c, d)
#define HEADER_SIZE 1536
#define COUNT(x) ((int)(sizeof(x)/sizeof((x)[0])))

#define SOFTWARE_NAME "MLV_DUMP CDNG"
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define ROR32(v,a) ((v) >> (a) | (v) << (32-(a)))
#define ROL32(v,a) ((v) << (a) | (v) >> (32-(a)))
#define ROR16(v,a) ((v) >> (a) | (v) << (16-(a)))
#define ROL16(v,a) ((v) << (a) | (v) >> (16-(a)))
#define log2(x) log((float)(x))/log(2.)

enum { IMG_SIZE_AUTO, IMG_SIZE_MAX };

//MLV WB modes
enum
{
    WB_AUTO         = 0,
    WB_SUNNY        = 1,
    WB_SHADE        = 8,
    WB_CLOUDY       = 2,
    WB_TUNGSTEN     = 3,
    WB_FLUORESCENT  = 4,
    WB_FLASH        = 5,
    WB_CUSTOM       = 6,
    WB_KELVIN       = 9
};

/* distinguish crop_rec 720 (retun value 1) from regular mv720 (return value 0) mode for focus pixel affected cameras */
static int check_mv720_vs_croprec720(struct frame_info * frame_info)
{
    switch(frame_info->idnt_hdr.cameraModel)
    {
        /* focus pixel affected cameras */
        case 0x80000331: // EOSM
        case 0x80000346: // 100D
        case 0x80000301: // 650D
        case 0x80000326: // 700D
            /* if resoutioon is regular mv720 or crop_rec 720 */
            if(frame_info->rawi_hdr.raw_info.width == 1808 && frame_info->rawi_hdr.raw_info.height < 900)
            {
                /* if RAWC block exists */
                if(frame_info->rawc_hdr.blockType[0])
                {
                    int sampling_x = frame_info->rawc_hdr.binning_x + frame_info->rawc_hdr.skipping_x;
                    int sampling_y = frame_info->rawc_hdr.binning_y + frame_info->rawc_hdr.skipping_y;
                    
                    /* check whether it's crop_rec 720 or regular mv720 mode
                       ATM crop_rec sampling for focus pixel affected cameras is 3x3
                       but let's be future proof and include 1x1 sampling as well */
                    if( !(sampling_y == 5 && sampling_x == 3) )
                    {
                        return 1; // it is really crop_rec 720 mode
                    }
                }
            }
        /* all other cameras do not need this check */
        default:
            return 0;
    }

    return 0;
}

static void deflicker(struct frame_info * frame_info, int target, uint16_t * data, size_t size)
{
    uint16_t black = frame_info->rawi_hdr.raw_info.black_level;
    uint16_t white = (1 << frame_info->rawi_hdr.raw_info.bits_per_pixel) + 1;
    
    struct histogram * hist = hist_create(white);
    hist_add(hist, data + 1, (uint32_t)((size -  1) / 2), 1);
    uint16_t median = hist_median(hist);
    double correction = log2((double) (target - black) / (median - black));
    frame_info->rawi_hdr.raw_info.exposure_bias[0] = correction * 10000;
    frame_info->rawi_hdr.raw_info.exposure_bias[1] = 10000;
}

/*****************************************************************************************************
 * Kelvin/Green to RGB Multipliers from UFRAW
 *****************************************************************************************************/

#define COLORS 3

/* Convert between Temperature and RGB.
 * Base on information from http://www.brucelindbloom.com/
 * The fit for D-illuminant between 4000K and 23000K are from CIE
 * The generalization to 2000K < T < 4000K and the blackbody fits
 * are my own and should be taken with a grain of salt.
 */
static const double XYZ_to_RGB[3][3] = {
    { 3.24071,    -0.969258,  0.0556352 },
    { -1.53726,    1.87599,    -0.203996 },
    { -0.498571,    0.0415557,  1.05707 }
};

static const double xyz_rgb[3][3] = {
    { 0.412453, 0.357580, 0.180423 },
    { 0.212671, 0.715160, 0.072169 },
    { 0.019334, 0.119193, 0.950227 }
};

static inline void temperature_to_RGB(double T, double RGB[3])
{
    int c;
    double xD, yD, X, Y, Z, max;
    // Fit for CIE Daylight illuminant
    if (T <= 4000)
    {
        xD = 0.27475e9 / (T * T * T) - 0.98598e6 / (T * T) + 1.17444e3 / T + 0.145986;
    }
    else if (T <= 7000)
    {
        xD = -4.6070e9 / (T * T * T) + 2.9678e6 / (T * T) + 0.09911e3 / T + 0.244063;
    }
    else
    {
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

static inline void pseudoinverse (double (*in)[3], double (*out)[3], int size)
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

static inline void cam_xyz_coeff (double cam_xyz[4][3], float pre_mul[4], float rgb_cam[3][4])
{
    double cam_rgb[4][3], inverse[4][3], num;
    int i, j, k;
    
    for (i=0; i < COLORS; i++)                /* Multiply out XYZ colorspace */
        for (j=0; j < 3; j++)
            for (cam_rgb[i][j] = k=0; k < 3; k++)
                cam_rgb[i][j] += cam_xyz[i][k] * xyz_rgb[k][j];
    
    for (i=0; i < COLORS; i++) {                /* Normalize cam_rgb so that */
        for (num=j=0; j < 3; j++)                /* cam_rgb * (1,1,1) is (1,1,1,1) */
            num += cam_rgb[i][j];
        for (j=0; j < 3; j++)
            cam_rgb[i][j] /= num;
        pre_mul[i] = 1 / num;
    }
    pseudoinverse (cam_rgb, inverse, COLORS);
    for (i=0; i < 3; i++)
        for (j=0; j < COLORS; j++)
            rgb_cam[i][j] = inverse[j][i];
}


static void kelvin_green_to_multipliers(double temperature, double green, double chanMulArray[3], camera_id_t * cam_id)
{
    float pre_mul[4], rgb_cam[3][4];
    double cam_xyz[4][3];
    double rgbWB[3];
    double cam_rgb[3][3];
    double rgb_cam_transpose[4][3];
    int c, cc, i, j;
    
    for (i = 0; i < 9; i++)
    {
        cam_xyz[i/3][i%3] = (double)cam_id->ColorMatrix2[i*2] / (double)cam_id->ColorMatrix2[i*2 + 1];
    }
    
    for (i = 9; i < 12; i++)
    {
        cam_xyz[i/3][i%3] = 0;
    }
    
    cam_xyz_coeff (cam_xyz, pre_mul, rgb_cam);
    
    for (i = 0; i < 4; i++) for (j = 0; j < 3; j++)
    {
        rgb_cam_transpose[i][j] = rgb_cam[j][i];
    }
    
    pseudoinverse(rgb_cam_transpose, cam_rgb, 3);
    
    temperature_to_RGB(temperature, rgbWB);
    rgbWB[1] = rgbWB[1] / green;
    
    for (c = 0; c < 3; c++)
    {
        double chanMulInv = 0;
        for (cc = 0; cc < 3; cc++)
            chanMulInv += 1 / pre_mul[c] * cam_rgb[c][cc] * rgbWB[cc];
        chanMulArray[c] = 1 / chanMulInv;
    }
    
    /* normalize green multiplier */
    chanMulArray[0] /= chanMulArray[1];
    chanMulArray[2] /= chanMulArray[1];
    chanMulArray[1] = 1;
}

static void get_white_balance(mlv_wbal_hdr_t wbal_hdr, int32_t *wbal, camera_id_t * cam_id)
{
    if(wbal_hdr.wb_mode == WB_CUSTOM)
    {
        wbal[0] = wbal_hdr.wbgain_r; wbal[1] = wbal_hdr.wbgain_g;
        wbal[2] = wbal_hdr.wbgain_g; wbal[3] = wbal_hdr.wbgain_g;
        wbal[4] = wbal_hdr.wbgain_b; wbal[5] = wbal_hdr.wbgain_g;
    }
    else
    {
        double kelvin = 5500;
        double green = 1.0;
        
        //TODO: G/M shift, not sure how this relates to "green" parameter
        if(wbal_hdr.wb_mode == WB_AUTO || wbal_hdr.wb_mode == WB_KELVIN)
        {
            kelvin = wbal_hdr.kelvin;
        }
        else if(wbal_hdr.wb_mode == WB_SUNNY)
        {
            kelvin = 5500;
        }
        else if(wbal_hdr.wb_mode == WB_SHADE)
        {
            kelvin = 7000;
        }
        else if(wbal_hdr.wb_mode == WB_CLOUDY)
        {
            kelvin = 6000;
        }
        else if(wbal_hdr.wb_mode == WB_TUNGSTEN)
        {
            kelvin = 3200;
        }
        else if(wbal_hdr.wb_mode == WB_FLUORESCENT)
        {
            kelvin = 4000;
        }
        else if(wbal_hdr.wb_mode == WB_FLASH)
        {
            kelvin = 5500;
        }
        double chanMulArray[3];
        kelvin_green_to_multipliers(kelvin, green, chanMulArray, cam_id);
        wbal[0] = 1000000; wbal[1] = (int32_t)(chanMulArray[0] * 1000000);
        wbal[2] = 1000000; wbal[3] = (int32_t)(chanMulArray[1] * 1000000);
        wbal[4] = 1000000; wbal[5] = (int32_t)(chanMulArray[2] * 1000000);
    }
}

/*****************************************************************************************************/


static uint16_t tiff_header[] = { byteOrderII, magicTIFF, 8, 0};

struct directory_entry {
    uint16_t tag;
    uint16_t type;
    uint32_t count;
    uint32_t value;
};

/* CDNG tag codes */
enum
{
    tcTimeCodes             = 51043,
    tcFrameRate             = 51044,
    tcTStop                 = 51058,
    tcReelName              = 51081,
    tcCameraLabel           = 51105,
};

static uint32_t add_array(int32_t * array, uint8_t * buffer, uint32_t * data_offset, size_t length)
{
    uint32_t result = *data_offset;
    memcpy(buffer + result, array, length * sizeof(int32_t));
    *data_offset += length * sizeof(int32_t);
    return result;
}

static uint32_t add_string(char * str, uint8_t * buffer, uint32_t * data_offset)
{
    uint32_t result = 0;
    size_t length = strlen(str) + 1;
    if(length <= 4)
    {
        //we can fit in 4 bytes, so just pack the string into result
        memcpy(&result, str, length);
    }
    else
    {
        result = *data_offset;
        memcpy(buffer + result, str, length);
        *data_offset += length;
        //align to 2 bytes
        if(*data_offset % 2) *data_offset += 1;
    }
    return result;
}

static uint32_t add_rational(int32_t numerator, int32_t denominator, uint8_t * buffer, uint32_t * data_offset)
{
    uint32_t result = *data_offset;
    memcpy(buffer + *data_offset, &numerator, sizeof(int32_t));
    *data_offset += sizeof(int32_t);
    memcpy(buffer + *data_offset, &denominator, sizeof(int32_t));
    *data_offset += sizeof(int32_t);
    return result;
}

static inline uint8_t to_tc_byte(int value)
{
    return (((value / 10) << 4) | (value % 10));
}

static uint32_t add_timecode(double framerate, int frame, uint8_t * buffer, uint32_t * data_offset)
{
    uint32_t result = *data_offset;
    memset(buffer + *data_offset, 0, 8);
    int hours, minutes, seconds, frames;
    
    /*
    //use drop frame if the framerate is close to 30 / 1.001
    int drop_frame = round((1.0 - framerate / ceil(framerate)) * 1000) == 1 && ceil(framerate) == 30;
    
    if (drop_frame)
    {
        int d = frame / 17982;
        int m = frame % 17982;
        int f = frame + 18 * d + 2 * (MAX(0, m - 2) / 1798);
        
        hours = ((f / 30) / 60) / 60;
        minutes = ((f / 30) / 60) % 60;
        seconds = (f / 30) % 60;
        frames = f % 30;
    }
    */
    
    //round the framerate to the next largest integral framerate
    //tc will not match real world time if framerate is not an integer
    double time = framerate == 0 ? 0 : frame / (framerate > 1 ? round(framerate) : framerate);
    hours = (int)floor(time / 3600);
    minutes = ((int)floor(time / 60)) % 60;
    seconds = ((int)floor(time)) % 60;
    frames = framerate > 1 ? (frame % ((int)round(framerate))) : 0;
    
    buffer[*data_offset] = to_tc_byte(frames) & 0x3F;
    //if(drop_frame) buffer[*data_offset] = buffer[*data_offset] | (1 << 7); //set the drop frame bit
    buffer[*data_offset + 1] = to_tc_byte(seconds) & 0x7F;
    buffer[*data_offset + 2] = to_tc_byte(minutes) & 0x7F;
    buffer[*data_offset + 3] = to_tc_byte(hours) & 0x3F;
    
    *data_offset += 8;
    return result;
}

static void add_ifd(struct directory_entry * ifd, uint8_t * header, size_t * position, int count, uint32_t next_ifd_offset)
{
    *(uint16_t*)(header + *position) = count;
    *position += sizeof(uint16_t);
    memcpy(header + *position, ifd, count * sizeof(struct directory_entry));
    *position += count * sizeof(struct directory_entry);
    memcpy(header + *position, &next_ifd_offset, sizeof(uint32_t));
    *position += sizeof(uint32_t);
}

static char * format_datetime(char * datetime, struct frame_info * frame_info)
{
    uint32_t seconds = frame_info->rtci_hdr.tm_sec + (uint32_t)((frame_info->vidf_hdr.timestamp - frame_info->rtci_hdr.timestamp) / 1000000);
    uint32_t minutes = frame_info->rtci_hdr.tm_min + seconds / 60;
    uint32_t hours = frame_info->rtci_hdr.tm_hour + minutes / 60;
    uint32_t days = frame_info->rtci_hdr.tm_mday + hours / 24;
    //TODO: days could also overflow in the month, but this is no longer simple modulo arithmetic like with hr:min:sec
    sprintf(datetime, "%04d:%02d:%02d %02d:%02d:%02d",
            1900 + frame_info->rtci_hdr.tm_year,
            frame_info->rtci_hdr.tm_mon + 1,
            days,
            hours % 24,
            minutes % 60,
            seconds % 60);
    return datetime;
}

/* returns the size of uncompressed image data. does not include header
   frame_info - pointer to the struct of MLV blocks associated with the frame
   size_mode - returns 16 bit image size or bit-packed image size depending on actual bits per pixel
*/
static size_t dng_get_image_size(struct frame_info * frame_info, int size_mode)
{
    if(!size_mode) // IMG_SIZE_AUTO
    {
        return (size_t)(frame_info->rawi_hdr.xRes * frame_info->rawi_hdr.yRes * frame_info->rawi_hdr.raw_info.bits_per_pixel / 8);
    }
    else // IMG_SIZE_MAX
    {
        return frame_info->rawi_hdr.xRes * frame_info->rawi_hdr.yRes * 2;
    }
}

/* generates the CDNG header. The result is written into dng_data struct
   frame_info - pointer to the struct of MLV blocks associated with the frame
   dng_data - pointer to the struct of DNG related data buffers and their sizes
*/
static void dng_fill_header(struct frame_info * frame_info, struct dng_data * dng_data)
{
    uint8_t * header = dng_data->header_buf;
    size_t position = 0;
    if(header)
    {
        memset(header, 0 , HEADER_SIZE);
        memcpy(header + position, tiff_header, sizeof(tiff_header));
        position += sizeof(tiff_header);
        
        uint32_t exif_ifd_offset = (uint32_t)(position + sizeof(uint16_t) + IFD0_COUNT * sizeof(struct directory_entry) + sizeof(uint32_t));
        uint32_t data_offset = exif_ifd_offset + sizeof(uint16_t) + EXIF_IFD_COUNT * sizeof(struct directory_entry) + sizeof(uint32_t);

        /* 'Make' Tag */
        char make[32];
        char * model = (char*)frame_info->idnt_hdr.cameraName;
        if(!model) model = "???";
        //make is usually the first word of cameraName
        strncpy(make, model, 32);
        char * space = strchr(make, ' ');
        if(space) *space = 0x0;
        
        /* 'Camera Serial Number' Tag */
        char serial[33];
        memcpy(serial, frame_info->idnt_hdr.cameraSerial, 32);
        serial[32] = 0x0; //make sure we are null terminated
        
        /* Get camera_id[] array element number for current camera */
        int current_cam = camera_id_get_current_cam(frame_info->idnt_hdr.cameraModel);

        /* 'Unique Camera Model' Tag */
        char unique_model[33];
        if (camera_id[current_cam].cameraModel)
            memcpy(unique_model, camera_id[current_cam].cameraName[UNIQ], 32);
        else
            memcpy(unique_model, frame_info->idnt_hdr.cameraName, 32);
        unique_model[32] = 0x0;

        /* Focal resolution stuff */
        int32_t focal_resolution_x[2] = {camera_id[current_cam].focal_resolution_x[0], camera_id[current_cam].focal_resolution_x[1]};
        int32_t focal_resolution_y[2] = {camera_id[current_cam].focal_resolution_y[0], camera_id[current_cam].focal_resolution_y[1]};
        int32_t par[4] = {1,1,1,1};

        /* If RAWC block present calculate aspect ratio from binning/skipping values */
        if(frame_info->rawc_hdr.blockType[0])
        {
            int sampling_x = frame_info->rawc_hdr.binning_x + frame_info->rawc_hdr.skipping_x;
            int sampling_y = frame_info->rawc_hdr.binning_y + frame_info->rawc_hdr.skipping_y;

            par[2] = sampling_y; par[3] = sampling_x;
            focal_resolution_x[1] = focal_resolution_x[1] * sampling_x;
            focal_resolution_y[1] = focal_resolution_y[1] * sampling_y;
        }
        else // use old method to calculate aspect ratio and detect crop_rec
        {
            double rawW = frame_info->rawi_hdr.raw_info.active_area.x2 - frame_info->rawi_hdr.raw_info.active_area.x1;
            double rawH = frame_info->rawi_hdr.raw_info.active_area.y2 - frame_info->rawi_hdr.raw_info.active_area.y1;
            double aspect_ratio = rawW / rawH;
            //check the aspect ratio of the original raw buffer, if it's > 2 and we're not in crop mode, then this is probably squeezed footage
            if(aspect_ratio > 2.0 && rawH <= 720 && !frame_info->crop_rec)
            {
                // 5x3 line skpping
                par[2] = 5; par[3] = 3;
                focal_resolution_x[1] = focal_resolution_x[1] * 3;
                focal_resolution_y[1] = focal_resolution_y[1] * 5;
            }
            //if the width is larger than 2000, we're probably not in crop mode
            else if(rawW < 2000)
            {
                focal_resolution_x[1] = focal_resolution_x[1] * 3;
                focal_resolution_y[1] = focal_resolution_y[1] * 3;
            }
        }

        //we get the active area of the original raw source, not the recorded data, so overwrite the active area if the recorded data does
        //not contain the OB areas
        if(frame_info->rawi_hdr.xRes < frame_info->rawi_hdr.raw_info.active_area.x2 ||
           frame_info->rawi_hdr.yRes < frame_info->rawi_hdr.raw_info.active_area.y2)
        {
            frame_info->rawi_hdr.raw_info.active_area.x1 = 0;
            frame_info->rawi_hdr.raw_info.active_area.y1 = 0;
            frame_info->rawi_hdr.raw_info.active_area.x2 = frame_info->rawi_hdr.xRes;
            frame_info->rawi_hdr.raw_info.active_area.y2 = frame_info->rawi_hdr.yRes;
        }
        
        /* FPS stuff*/
        int32_t frame_rate[2] = {frame_info->file_hdr.sourceFpsNom, frame_info->file_hdr.sourceFpsDenom};
        if(frame_info->fps_override > 0)
        {
            frame_rate[0] = (int32_t)frame_info->fps_override;
            frame_rate[1] = 1000;
        }
        double frame_rate_f = frame_rate[1] == 0 ? 0 : (double)frame_rate[0] / (double)frame_rate[1];
        
        /* Date */
        char datetime[255];

        /* Baseline exposure stuff */
        int32_t basline_exposure[2] = {frame_info->rawi_hdr.raw_info.exposure_bias[0],frame_info->rawi_hdr.raw_info.exposure_bias[1]};
        if(basline_exposure[1] == 0)
        {
            basline_exposure[0] = 0;
            basline_exposure[1] = 1;
        }

        /* Time code stuff */
        //number of frames since midnight
        int tc_frame = (int)frame_info->vidf_hdr.frameNumber;// + (uint64_t)((frame_info->rtci_hdr.tm_hour * 3600 + frame_info->rtci_hdr.tm_min * 60 + frame_info->rtci_hdr.tm_sec) * frame_info->file_hdr.sourceFpsNom) / (uint64_t)frame_info->file_hdr.sourceFpsDenom;
        
        /* White balance stuff */
        int32_t wbal[6];
        get_white_balance(frame_info->wbal_hdr, wbal, &camera_id[current_cam]);

        /* tcReelName */
        #ifdef _WIN32
        char * reel_name = strrchr(frame_info->mlv_filename, '\\');
        #else
        char * reel_name = strrchr(frame_info->mlv_filename, '/');
        #endif
        (!reel_name) ? (reel_name = frame_info->mlv_filename) : ++reel_name;

        /* Fill up IFD structs */
        struct directory_entry IFD0[IFD0_COUNT] =
        {
            {tcNewSubFileType,              ttLong,     1,      sfMainImage},
            {tcImageWidth,                  ttLong,     1,      frame_info->rawi_hdr.xRes},
            {tcImageLength,                 ttLong,     1,      frame_info->rawi_hdr.yRes},
            {tcBitsPerSample,               ttShort,    1,      (!frame_info->raw_state && !frame_info->pack_bits) ? 16 : frame_info->rawi_hdr.raw_info.bits_per_pixel},
            {tcCompression,                 ttShort,    1,      (!(frame_info->raw_state % 2)) ? ccUncompressed : ccJPEG},
            {tcPhotometricInterpretation,   ttShort,    1,      piCFA},
            {tcFillOrder,                   ttShort,    1,      1},
            {tcMake,                        ttAscii,    STRING_ENTRY(make, header, &data_offset)},
            {tcModel,                       ttAscii,    STRING_ENTRY(model, header, &data_offset)},
            {tcStripOffsets,                ttLong,     1,      (uint32_t)HEADER_SIZE},
            {tcOrientation,                 ttShort,    1,      1},
            {tcSamplesPerPixel,             ttShort,    1,      1},
            {tcRowsPerStrip,                ttShort,    1,      frame_info->rawi_hdr.yRes},
            {tcStripByteCounts,             ttLong,     1,      (!frame_info->raw_state && frame_info->pack_bits) ? dng_data->image_size_bitpacked : dng_data->image_size},
            {tcPlanarConfiguration,         ttShort,    1,      pcInterleaved},
            {tcSoftware,                    ttAscii,    STRING_ENTRY(SOFTWARE_NAME, header, &data_offset)},
            {tcDateTime,                    ttAscii,    STRING_ENTRY(format_datetime(datetime,frame_info), header, &data_offset)},
            {tcCFARepeatPatternDim,         ttShort,    2,      0x00020002}, //2x2
            {tcCFAPattern,                  ttByte,     4,      0x02010100}, //RGGB
            {tcExifIFD,                     ttLong,     1,      exif_ifd_offset},
            {tcDNGVersion,                  ttByte,     4,      0x00000401}, //1.4.0.0 in little endian
            {tcUniqueCameraModel,           ttAscii,    STRING_ENTRY(unique_model, header, &data_offset)},
            {tcBlackLevel,                  ttLong,     1,      frame_info->rawi_hdr.raw_info.black_level},
            {tcWhiteLevel,                  ttLong,     1,      frame_info->rawi_hdr.raw_info.white_level},
            {tcDefaultScale,                ttRational, RATIONAL_ENTRY(par, header, &data_offset, 4)},
            {tcDefaultCropOrigin,           ttShort,    2,      PACK(frame_info->rawi_hdr.raw_info.crop.origin)},
            {tcDefaultCropSize,             ttShort,    2,      PACK2((frame_info->rawi_hdr.raw_info.active_area.x2 - frame_info->rawi_hdr.raw_info.active_area.x1), (frame_info->rawi_hdr.raw_info.active_area.y2 - frame_info->rawi_hdr.raw_info.active_area.y1))},
            {tcColorMatrix1,                ttSRational,RATIONAL_ENTRY(camera_id[current_cam].ColorMatrix1, header, &data_offset, 18)},
            {tcColorMatrix2,                ttSRational,RATIONAL_ENTRY(camera_id[current_cam].ColorMatrix2, header, &data_offset, 18)},
            {tcAsShotNeutral,               ttRational, RATIONAL_ENTRY(wbal, header, &data_offset, 6)},
            {tcBaselineExposure,            ttSRational,RATIONAL_ENTRY(basline_exposure, header, &data_offset, 2)},
            {tcCameraSerialNumber,          ttAscii,    STRING_ENTRY(serial, header, &data_offset)},
            {tcCalibrationIlluminant1,      ttShort,    1,      lsStandardLightA},
            {tcCalibrationIlluminant2,      ttShort,    1,      lsD65},
            {tcActiveArea,                  ttLong,     ARRAY_ENTRY(frame_info->rawi_hdr.raw_info.dng_active_area, header, &data_offset, 4)},
            {tcForwardMatrix1,              ttSRational,RATIONAL_ENTRY(camera_id[current_cam].ForwardMatrix1, header, &data_offset, 18)},
            {tcForwardMatrix2,              ttSRational,RATIONAL_ENTRY(camera_id[current_cam].ForwardMatrix2, header, &data_offset, 18)},
            {tcTimeCodes,                   ttByte,     8,      add_timecode(frame_rate_f, tc_frame, header, &data_offset)},
            {tcFrameRate,                   ttSRational,RATIONAL_ENTRY(frame_rate, header, &data_offset, 2)},
            {tcReelName,                    ttAscii,    STRING_ENTRY(reel_name, header, &data_offset)},
            {tcBaselineExposureOffset,      ttSRational,RATIONAL_ENTRY2(0, 1, header, &data_offset)},
        };
        
        struct directory_entry EXIF_IFD[EXIF_IFD_COUNT] =
        {
            {tcExposureTime,                ttRational, RATIONAL_ENTRY2((int32_t)frame_info->expo_hdr.shutterValue/1000, 1000, header, &data_offset)},
            {tcFNumber,                     ttRational, RATIONAL_ENTRY2(frame_info->lens_hdr.aperture, 100, header, &data_offset)},
            {tcISOSpeedRatings,             ttShort,    1,      frame_info->expo_hdr.isoValue},
            {tcSensitivityType,             ttShort,    1,      stISOSpeed},
            {tcExifVersion,                 ttUndefined,4,      0x30333230},
            {tcSubjectDistance,             ttRational, RATIONAL_ENTRY2(frame_info->lens_hdr.focalDist, 1, header, &data_offset)},
            {tcFocalLength,                 ttRational, RATIONAL_ENTRY2(frame_info->lens_hdr.focalLength, 1, header, &data_offset)},
            {tcFocalPlaneXResolutionExif,   ttRational, RATIONAL_ENTRY(focal_resolution_x, header, &data_offset, 2)},
            {tcFocalPlaneYResolutionExif,   ttRational, RATIONAL_ENTRY(focal_resolution_y, header, &data_offset, 2)},
            {tcFocalPlaneResolutionUnitExif,ttShort,    1,      camera_id[current_cam].focal_unit}, //inches
            {tcLensModelExif,               ttAscii,    STRING_ENTRY((char*)frame_info->lens_hdr.lensName, header, &data_offset)},
        };
        
        /* update the StripOffsets to the correct location
           the image data starts where our extra data ends */
        IFD0[9].value = data_offset;

        add_ifd(IFD0, header, &position, IFD0_COUNT, 0);
        add_ifd(EXIF_IFD, header, &position, EXIF_IFD_COUNT, 0);
        
        /* set real header size */
        dng_data->header_size = data_offset;
    }
}

/* unpack bits to 16 bit little endian
   input_buffer - a buffer containing the packed imaged data
   output_buffer - the buffer where the result will be written
   max_size - the size in bytes to write into the output_buffer (unpacked 16bit)
   bpp - raw data bits per pixel
*/
void dng_unpack_image_bits(uint16_t * input_buffer, uint16_t * output_buffer, size_t max_size, uint32_t bpp)
{
    uint32_t pixel_end = (uint32_t)(max_size / 2);
    uint32_t mask = (1 << bpp) - 1;

    uint16_t *packed_bits = input_buffer;
    uint16_t *unpacked_bits = output_buffer;

    for (uint32_t pixel_index = 0; pixel_index < pixel_end; pixel_index++)
    {
        uint32_t bits_offset = pixel_index * bpp;
        uint32_t bits_address = bits_offset / 16;
        uint32_t bits_shift = bits_offset % 16;

        /* fetch two 16 bit words into a 32 bit register and correct it plus shift it as needed.
        after the 32 bit fetch, the two 16 bit words will be swapped, so use a ROR to align them correctly.
        ROR by 16 to swap 16 bit words plus the bits needed to put the needed pixel bits to right position */
        uint32_t rotate_value = 16 + ((32 - bpp) - bits_shift);
        uint32_t uncorrected_data = *((uint32_t *)&packed_bits[bits_address]);
        uint32_t data = ROR32(uncorrected_data, rotate_value);

        unpacked_bits[pixel_index] = (uint16_t)(data & mask);
    }
}

/* pack bits to 16 bit little endian and convert to big endian (raw payload DNG spec)
   input_buffer - a buffer containing the unpacked imaged data
   output_buffer - the buffer where the result will be written
   max_size - the size in bytes to get from the input_buffer (unpacked 16bit)
   bpp - raw data bits per pixel 
*/
void dng_pack_image_bits(uint16_t * input_buffer, uint16_t * output_buffer, size_t max_size, uint32_t bpp)
{
    uint32_t pixel_end = (uint32_t)(max_size / 2);
    uint32_t bits_free = 16 - bpp;

    uint16_t *unpacked_bits = input_buffer;
    uint16_t *packed_bits = output_buffer;

    packed_bits[0] = unpacked_bits[0] << bits_free;
    for (uint32_t pixel_index = 1; pixel_index < pixel_end; pixel_index++)
    {
        uint32_t bits_offset = (pixel_index * bits_free) % 16;
        uint32_t bits_to_rol = bits_free + bits_offset + (bits_offset > 0) * 16;

        /* increment pointer by two bytes but fetch 32 bit words from input and outbut buffers.
        after the 32 bit fetch, the two 16 bit words will be swapped, so use a ROL by 16 to swap
        16 bit words plus shift to the left to put the needed pixel bits to right position.
        mask/zero high 16 bits of 32 bit word of packed buffer and do logical OR to ROLed unpacked one.
        make current packed 16 bit word big endian to satisfy DNG spec */
        uint32_t data = ROL32((uint32_t)unpacked_bits[pixel_index], bits_to_rol);
        *(uint32_t *)packed_bits = (*(uint32_t *)packed_bits & 0x0000FFFF) | data;

        if(bits_offset > 0 && bits_offset <= bpp)
        {
            *(uint16_t *)packed_bits = ROL16(*(uint16_t *)packed_bits, 8);
            packed_bits++;
        }
    }
}

/* changes endianness of the 16 bit buffer values
   DNG spec: 10/12/14bit raw should be big endian, 8/16/32bit raw can be little endian
   input_buffer - pointer to the buffer
   max_size - the size of the buffer in bytes
*/
static void dng_reverse_byte_order(uint16_t * input_buffer, size_t max_size)
{
    uint32_t pixel_end = (uint32_t)(max_size / 2);

    for (uint32_t pixel_index = 0; pixel_index < pixel_end; pixel_index++)
    {
        input_buffer[pixel_index] = ROL16(input_buffer[pixel_index], 8);
    }
}

/* fill DNG header buffer */
void dng_init_header(struct frame_info * frame_info, struct dng_data * dng_data)
{
    static int first_time = 1;
    
    if (first_time)
    {
        dng_data->header_size = HEADER_SIZE;
        dng_data->header_buf = (uint8_t*)malloc(dng_data->header_size);
        first_time = 0;
    }

    dng_fill_header(frame_info, dng_data);
}

/* fill DNG image data buffer */
void dng_init_data(struct frame_info * frame_info, struct dng_data * dng_data)
{
    static int first_time = 1;
    
    if (first_time)
    {
        dng_data->image_size = dng_get_image_size(frame_info, IMG_SIZE_MAX);
        dng_data->image_buf = (uint16_t*)malloc(dng_data->image_size);
    
        dng_data->image_size_bitpacked = dng_get_image_size(frame_info, IMG_SIZE_AUTO);
        dng_data->image_buf_bitpacked = (uint16_t*)malloc(dng_data->image_size_bitpacked);

        /* backup size and pointer of the original image buffer
           it might change later if raw compression is enabled 
           restoring from backup takes place in dng_save() routine */
        dng_data->image_size_bak = dng_data->image_size;
        dng_data->image_buf_bak = dng_data->image_buf;        
        
        first_time = 0;
    }
    
    if(frame_info->rawi_hdr.raw_info.bits_per_pixel < 16)
    {
        dng_unpack_image_bits(frame_info->rawi_hdr.raw_info.buffer, dng_data->image_buf, dng_data->image_size, frame_info->rawi_hdr.raw_info.bits_per_pixel);
    }
    else
    {
        /* if raw data 16 bit already (-b 16) */
        dng_data->image_buf = frame_info->rawi_hdr.raw_info.buffer;
        frame_info->pack_bits = 0;

    }
}

/* all raw processing takes place here */
void dng_process_data(struct frame_info * frame_info, struct dng_data * dng_data)
{
    static int first_time = 1;

    /* deflicker RAW data */
    if (frame_info->deflicker_target)
    {
        if (first_time && frame_info->show_progress) 
        {
            printf("\nPer-frame exposure compensation: 'ON'\nDeflicker target: '%d'\n", frame_info->deflicker_target);
        }
        deflicker(frame_info, frame_info->deflicker_target, dng_data->image_buf, dng_data->image_size);
    }

    /* fix pattern noise */
    if (frame_info->pattern_noise)
    {
        if (first_time && frame_info->show_progress) 
        {
            printf("\nFixing pattern noise...\n");
        }
        fix_pattern_noise((int16_t *)dng_data->image_buf, frame_info->rawi_hdr.xRes, frame_info->rawi_hdr.yRes, frame_info->rawi_hdr.raw_info.white_level, 0);
    }

    /* set crop_rec flag from MLV or CLI */
    int crop_rec = check_mv720_vs_croprec720(frame_info);
    /* if crop_rec 720 mode is not detected try to set crop_rec flag from CLI */
    if(!crop_rec) crop_rec = frame_info->crop_rec;
    /* detect if raw data is restricted to 8-12bit lossless */
    int restricted_lossless = ( (frame_info->file_hdr.videoClass & MLV_VIDEO_CLASS_FLAG_LJ92) && (frame_info->rawi_hdr.raw_info.white_level < 15000) );
    /* set parameters for bad/focus pixel processing */
    struct parameter_list par = 
    {
        frame_info->mlv_filename,
        frame_info->dual_iso,
        (frame_info->bad_pixels == 2),
        frame_info->save_bpm,
        frame_info->show_progress,
        frame_info->fpi_method,
        frame_info->bpi_method,
        crop_rec,
        (!restricted_lossless ? 0 : 5),
        frame_info->idnt_hdr.cameraModel,
        frame_info->rawi_hdr.xRes,
        frame_info->rawi_hdr.yRes,
        frame_info->vidf_hdr.panPosX,
        frame_info->vidf_hdr.panPosY,
        frame_info->rawi_hdr.raw_info.width,
        frame_info->rawi_hdr.raw_info.height,
        frame_info->rawi_hdr.raw_info.black_level
    };
    /* fix focus pixels */
    if (frame_info->focus_pixels)
    {
        fix_focus_pixels(dng_data->image_buf, par);
    }

    /* fix bad pixels */
    if (frame_info->bad_pixels)
    {
        fix_bad_pixels(dng_data->image_buf, par);
    }

    /* do chroma smoothing */
    if (frame_info->chroma_smooth)
    {
        if (first_time && frame_info->show_progress) 
        {
            printf("\nUsing chroma smooth method: '%dx%d'\n", frame_info->chroma_smooth, frame_info->chroma_smooth);
        }
        chroma_smooth(dng_data->image_buf, frame_info->rawi_hdr.xRes, frame_info->rawi_hdr.yRes, frame_info->rawi_hdr.raw_info.black_level, frame_info->rawi_hdr.raw_info.white_level, frame_info->chroma_smooth);
    }

    /* fix vertical stripes */
    if (frame_info->vertical_stripes)
    {
        fix_vertical_stripes(dng_data->image_buf, 0, dng_data->image_size / 2, &(frame_info->rawi_hdr.raw_info), frame_info->rawi_hdr.xRes, frame_info->rawi_hdr.yRes, frame_info->vertical_stripes, frame_info->show_progress);
    }

    first_time = 0;
}

/* save DNG file */
int dng_save(struct frame_info * frame_info, struct dng_data * dng_data)
{
    static uint32_t frame_count = 0;

    FILE* dngf = fopen(frame_info->dng_filename, "wb");
    if (!dngf)
    {
        return 0;
    }
    
    /* write DNG header */
    if (fwrite(dng_data->header_buf, dng_data->header_size, 1, dngf) != 1)
    {
        fclose(dngf);
        return 0;
    }
    
    /* write DNG image data */
    /* if raw is uncompressed and 16 bit unpacked DNGs are not requested with "--no-bitpack" */
    if(frame_info->raw_state == UNCOMPRESSED_RAW && frame_info->pack_bits)
    {
        /* pack bits and make raw data big endian before saving to the dng file */
        dng_pack_image_bits(dng_data->image_buf, dng_data->image_buf_bitpacked, dng_data->image_size, frame_info->rawi_hdr.raw_info.bits_per_pixel);

        if (fwrite(dng_data->image_buf_bitpacked, dng_data->image_size_bitpacked, 1, dngf) != 1)
        {
            fclose(dngf);
            return 0;
        }
    }
    else // a) when "--no-bitpack" specified, b) when passing through uncompressed/lossless raw, c) when raw is compressed by "-c"
    {
        if(frame_info->raw_state == UNCOMPRESSED_ORIG && (frame_info->rawi_hdr.raw_info.bits_per_pixel != 16))
        {
            dng_reverse_byte_order(dng_data->image_buf, dng_data->image_size);
        }

        if (fwrite(dng_data->image_buf, dng_data->image_size, 1, dngf) != 1)
        {
            fclose(dngf);
            return 0;
        }
    }

    fclose(dngf);

    /* show writing progress */
    if (frame_info->show_progress)
    {
        if (!frame_count)
        {
            switch (frame_info->raw_state)
            {
                case UNCOMPRESSED_RAW:
                    printf("\nWriting uncompressed frames...\n");
                    break;
                case UNCOMPRESSED_ORIG:
                    printf("\nPassing through original uncompressed raw...\n");
                    break;
                case COMPRESSED_ORIG:
                    printf("\nPassing through original lossless raw...\n");
                    break;
            }
        }
        if (frame_info->raw_state != COMPRESSED_RAW) printf("\rCurrent frame '%s' (frames saved: %d)", frame_info->dng_filename, frame_count+1);
    }
    
    /* restore size and pointer of the original uncompressed image buffer from backup made in dng_init_data() */
    if(dng_data->image_buf != dng_data->image_buf_bak)
    {
        dng_data->image_size = dng_data->image_size_bak;
        dng_data->image_buf = dng_data->image_buf_bak;
    }

    frame_count++;
    return 1;
}

/* free all buffers used for DNG creation and RAW processing */
void dng_free_data(struct dng_data * dng_data)
{
    if(dng_data->header_buf) free(dng_data->header_buf);
    if(dng_data->image_buf) free(dng_data->image_buf);
    if(dng_data->image_buf_bitpacked) free(dng_data->image_buf_bitpacked);
    free_pixel_maps();
}
