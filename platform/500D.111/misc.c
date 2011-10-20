// misc functions specific to 550D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

void display_shooting_info() // called from debug task
{
	if (lv) return;
	
	int bg = bmp_getpixel(314, 260);
	uint32_t fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 320, 260, "%5dK", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		fnt = FONT(FONT_LARGE, COLOR_FG_NONLV, bg);

		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, 435, 240, "%s%d ", ba > 0 ? "A" : "B", ABS(ba));
		else bmp_printf(fnt, 435, 240, "   ");

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, 435, 270, "%s%d ", gm > 0 ? "G" : "M", ABS(gm));
		else bmp_printf(fnt, 435, 270, "   ");
	}

	iso_refresh_display();

	bg = bmp_getpixel(15, 430);
	fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	
	extern int hdr_steps, hdr_stepsize;
	if (hdr_steps > 1)
		bmp_printf(fnt, 380, 450, "HDR %dx%dEV", hdr_steps, hdr_stepsize/8);
	else
		bmp_printf(fnt, 380, 450, "           ");

	//~ bmp_printf(fnt, 200, 450, "Flash:%s", 
		//~ strobo_firing == 0 ? " ON" : 
		//~ strobo_firing == 1 ? "OFF" : "Auto"
		//~ strobo_firing < 2 && flash_and_no_flash ? "/T" : "  "
		//~ );

	bmp_printf(fnt, 40, 460, get_mlu() ? "MLU" : "   ");

	display_lcd_remote_icon(480, 0);
	display_trap_focus_info();
}



// image buffers
// http://magiclantern.wikia.com/wiki/VRAM

PROP_INT(0x80030002, mvr_rec)

struct vram_info * get_yuv422_hd_vram()
{
	static struct vram_info _vram_info;
	_vram_info.vram = (uint8_t*)YUV422_HD_BUFFER_DMA_ADDR;
	_vram_info.width = recording ? (video_mode_resolution == 0 ? 1576 :
									video_mode_resolution == 1 ? 928 :
									video_mode_resolution == 2 ? 720 : 0)
	: lv_dispsize > 1 ? 944
	: shooting_mode != SHOOTMODE_MOVIE ? 928
	: (video_mode_resolution == 0 ? 1576 :
	   video_mode_resolution == 1 ? 928 :
	   video_mode_resolution == 2 ? 928 : 0);
	_vram_info.pitch = _vram_info.width << 1; 
	_vram_info.height = recording ? (video_mode_resolution == 0 ? 1048:
									 video_mode_resolution == 1 ? 616 :
									 video_mode_resolution == 2 ? 480 : 0)
	: lv_dispsize > 1 ? 632
	: shooting_mode != SHOOTMODE_MOVIE ? 616
	: (video_mode_resolution == 0 ? 1048 :
	   video_mode_resolution == 1 ? 616 :
	   video_mode_resolution == 2 ? 616 : 0);
	return &_vram_info;
}