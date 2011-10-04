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
		case YUV422_LV_BUFFER_1:
			return YUV422_LV_BUFFER_1;
		case YUV422_LV_BUFFER_2:
			return YUV422_LV_BUFFER_2;
		case YUV422_LV_BUFFER_3:
			return YUV422_LV_BUFFER_3;
	}
	return YUV422_LV_BUFFER_1; // fall back to default
}

static int fastrefresh_direction = 0;

void guess_fastrefresh_direction() {
	static int old_pos = YUV422_LV_BUFFER_1;
	if (old_pos == YUV422_LV_BUFFER_DMA_ADDR) return;
	if (old_pos == YUV422_LV_BUFFER_1 && YUV422_LV_BUFFER_DMA_ADDR == YUV422_LV_BUFFER_2) fastrefresh_direction = 1;
	if (old_pos == YUV422_LV_BUFFER_1 && YUV422_LV_BUFFER_DMA_ADDR == YUV422_LV_BUFFER_3) fastrefresh_direction = 0;
	old_pos = YUV422_LV_BUFFER_DMA_ADDR;
}


void* get_fastrefresh_422_buf()
{
	if (fastrefresh_direction) {
		switch (YUV422_LV_BUFFER_DMA_ADDR)
		{
			case YUV422_LV_BUFFER_1:
				return YUV422_LV_BUFFER_2;
			case YUV422_LV_BUFFER_2:
				return YUV422_LV_BUFFER_3;
			case YUV422_LV_BUFFER_3:
				return YUV422_LV_BUFFER_1;
		}
		return YUV422_LV_BUFFER_1; // fall back to default
	} else {
		switch (YUV422_LV_BUFFER_DMA_ADDR)
		{
			case YUV422_LV_BUFFER_1:
				return YUV422_LV_BUFFER_3;
			case YUV422_LV_BUFFER_2:
				return YUV422_LV_BUFFER_1;
			case YUV422_LV_BUFFER_3:
				return YUV422_LV_BUFFER_2;
		}
		return YUV422_LV_BUFFER_1; // fall back to default

	}
}


// YUV422_HD_BUFFER_DMA_ADDR returns many possible values, but usually cycles between last two
// This function returns the value which was used just before the current one
// That buffer is not updated by DMA (and should contain a silent picture without horizontal cut)
void* get_422_hd_idle_buf()
{
	static int idle_buf = YUV422_HD_BUFFER_1;
	static int current_buf = YUV422_HD_BUFFER_2;

	int hd = YUV422_HD_BUFFER_DMA_ADDR;
	if (IS_HD_BUFFER(hd))
	{
		if (hd != current_buf)
		{
			idle_buf = current_buf;
			current_buf = hd;
		}
	}
	return (void*)idle_buf;
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
