#include "../../dryos.h"
#include "../../lens.h"
#include "../../property.h"
#include "../../bmp.h"
#include "../../config.h"

extern int lv_focus_done;

void
lens_focus(
	unsigned		mode,
	int			step
)
{
	if (!lv_drawn()) return;
	if (is_manual_focus()) return;

	while (lens_info.job_state) msleep(100);
	lens_focus_wait();
	lv_focus_done = 0;
	
	struct prop_focus focus = {
		.active		= 1,
		.mode		= 7,
		.step_hi	= (step >> 8) & 0xFF,
		.step_lo	= (step >> 0) & 0xFF,
		.unk		= 0,
	};

	prop_request_change( PROP_LV_FOCUS, &focus, sizeof(focus) );
	if (get_zoom_overlay_mode()==2) zoom_overlay_set_countdown(300);
}

/*
void lens_focus_ex(unsigned mode, int step, int active)
{
	struct prop_focus focus = {
		.active		= active,
		.mode		= mode,
		.step_hi	= (step >> 8) & 0xFF,
		.step_lo	= (step >> 0) & 0xFF,
	};

	prop_request_change( PROP_LV_FOCUS, &focus, sizeof(focus) );
}*/


