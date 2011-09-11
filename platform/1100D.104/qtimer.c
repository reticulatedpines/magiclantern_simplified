#include "dryos.h"
#include "bmp.h"
#include "version.h"
#include "config.h"
#include "menu.h"
#include "property.h"
#include "gui.h"
#include "lens.h"
#include "qtimer.h"

static void qtimer_task( void )
{
	msleep(5000);
	int k;
	av_long_pressed = 0;
	while(1)
	{
		av_long_pressed = 0;
		msleep(100);
		if(get_av_pressed()) {
			k=0;
			while(get_av_pressed()) {
				msleep(100);
				if(k++ >= 2) {
					av_long_pressed = 1;
				}
			}
		}	
	}

}


TASK_CREATE( "qtimer_task", qtimer_task, 0, 0x1f, 0x1000 );
