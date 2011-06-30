#include "../../dryos.h"
#include "../../lens.h"
#include "../../property.h"
#include "../../bmp.h"
#include "../../config.h"

extern int lv_focus_done;

struct lens_control {
	// 0x00-0x1D4: not used
	                   uint32_t off_0x1D4;uint32_t off_0x1D8;uint32_t off_0x1DC; //1D4: amount of rotation, 1D8: step size (?)
	uint32_t off_0x1E0;uint32_t off_0x1E4;uint32_t off_0x1E8;uint32_t off_0x1EC;
	uint32_t off_0x1F0;uint32_t off_0x1F4;uint32_t off_0x1F8;uint32_t off_0x1FC;
	uint32_t off_0x200;uint32_t off_0x204;uint32_t off_0x208;uint32_t off_0x20C;
	uint32_t off_0x210;uint32_t off_0x214;uint32_t off_0x218;uint32_t off_0x21C;
	uint32_t off_0x220;uint32_t off_0x224;uint32_t off_0x228;
}__attribute__((packed));

struct lens_control lctr;

void
lens_focus(
	unsigned		mode,
	int			step
)
{
	if (!lv) return;
	if (is_manual_focus()) return;

	while (lens_info.job_state) msleep(100);
	lens_focus_wait();
	lv_focus_done = 0;

	lctr.off_0x228 = 0x1;
	if (step<0) { lctr.off_0x228 += 0x8000; step = -step; }

	float a = lens_info.lens_rotation/((float)step);
	float b = lens_info.lens_step;
	uint32_t* aconv = &a;
	uint32_t* bconv = &b;

	lctr.off_0x1D4 = SWAP_ENDIAN(*aconv); //0x00400F3C; // single floating point number: 0x008743286
	lctr.off_0x1D8 = SWAP_ENDIAN(*bconv); // single floating point number: 1
	lctr.off_0x1F4 = 0x08080000;
	lctr.off_0x200 = 0x00003200;
	lctr.off_0x20C = 0xFF000000;
	lctr.off_0x210 = 0x000000FF;
	lctr.off_0x214 = 0xFFFFFF00;
	lctr.off_0x218 = 0x0000FFFF;
	
	AfCtrl_SetLensParameterRemote(((char*)&lctr)-0x1D4);

	if (get_zoom_overlay_mode()==2) zoom_overlay_set_countdown(300);
}

