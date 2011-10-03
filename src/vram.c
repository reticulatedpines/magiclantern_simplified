/** \file
 * Common functions for image buffers
 */

#include "dryos.h"
#include "propvalues.h"

/*
int* lut_bm2lv_x = 0;
int* lut_lv2bm_x = 0;

int* lut_lv2hd_x = 0;
int* lut_hd2lv_x = 0;

int* lut_bm2hd_x = 0;
int* lut_hd2bm_x = 0;

int* lut_bm2lv_y = 0;
int* lut_lv2bm_y = 0;

int* lut_lv2hd_y = 0;
int* lut_hd2lv_y = 0;

int* lut_bm2hd_y = 0;
int* lut_hd2bm_y = 0;

void lut_realloc(int** buf, int size)
{
	if (*buf) FreeMemory(*buf);
	*buf = AllocateMemory(size);
}

void lut_init()
{
	lut_realloc(&lut_bm2lv_x, SL.BM.W);
	lut_realloc(&lut_bm2lv_y, SL.BM.H);

	lut_realloc(&lut_lv2bm_x, SL.LV.W);
	lut_realloc(&lut_lv2bm_y, SL.LV.H);

	lut_realloc(&lut_lv2bm);
}*/

#include "bmp.h"

void* get_write_422_buf()
{
	switch (YUV422_LV_BUFFER_DMA_ADDR)
	{
		case YUV422_LV_BUFFER:
			return YUV422_LV_BUFFER;
		case YUV422_LV_BUFFER_2:
			return YUV422_LV_BUFFER_2;
		case YUV422_LV_BUFFER_3:
			return YUV422_LV_BUFFER_3;
	}
	return YUV422_LV_BUFFER; // fall back to default
}

static int fastrefresh_direction = 0;

void guess_fastrefresh_direction() {
	static int old_pos = YUV422_LV_BUFFER;
	if (old_pos == YUV422_LV_BUFFER_DMA_ADDR) return;
	if (old_pos == YUV422_LV_BUFFER && YUV422_LV_BUFFER_DMA_ADDR == YUV422_LV_BUFFER_2) fastrefresh_direction = 1;
	if (old_pos == YUV422_LV_BUFFER && YUV422_LV_BUFFER_DMA_ADDR == YUV422_LV_BUFFER_3) fastrefresh_direction = 0;
	old_pos = YUV422_LV_BUFFER_DMA_ADDR;
}


void* get_fastrefresh_422_buf()
{
	if (fastrefresh_direction) {
		switch (YUV422_LV_BUFFER_DMA_ADDR)
		{
			case YUV422_LV_BUFFER:
				return YUV422_LV_BUFFER_2;
			case YUV422_LV_BUFFER_2:
				return YUV422_LV_BUFFER_3;
			case YUV422_LV_BUFFER_3:
				return YUV422_LV_BUFFER;
		}
		return YUV422_LV_BUFFER; // fall back to default
	} else {
		switch (YUV422_LV_BUFFER_DMA_ADDR)
		{
			case YUV422_LV_BUFFER:
				return YUV422_LV_BUFFER_3;
			case YUV422_LV_BUFFER_2:
				return YUV422_LV_BUFFER;
			case YUV422_LV_BUFFER_3:
				return YUV422_LV_BUFFER_2;
		}
		return YUV422_LV_BUFFER; // fall back to default

	}
}


void* get_422_hd_idle_buf() // the one which is not updated by DMA
{
	switch (YUV422_HD_BUFFER_DMA_ADDR)
	{
		case YUV422_HD_BUFFER:
			return YUV422_HD_BUFFER_2;
		case YUV422_HD_BUFFER_2:
			return YUV422_HD_BUFFER;
	}
	return YUV422_HD_BUFFER; // fall back to default
}


struct vram_info * get_yuv422_vram()
{
	static struct vram_info _vram_info;
	_vram_info.vram = get_fastrefresh_422_buf();
	if (gui_state == GUISTATE_PLAYMENU) _vram_info.vram = (void*) YUV422_LV_BUFFER_DMA_ADDR;

	//~ _vram_info.width = SL.LV.W;
	//~ _vram_info.height = SL.LV.H;

	if (hdmi_code == 5)
	{
		_vram_info.width = 1920;
		_vram_info.height = 1080;
	}
	else if (ext_monitor_rca)
	{
		_vram_info.width = 512;
		_vram_info.height = 512;
	}
	else
	{
		_vram_info.width = 720;
		_vram_info.height = 480;
	}
	
	_vram_info.pitch = _vram_info.width * 2;

	return &_vram_info;
}
