#ifndef _hdr_h_
#define _hdr_h_

/* HDR video & related */
/* todo: rename to hdr-video.c/h? */

int hdr_video_enabled();
void hdr_get_iso_range(int* iso_low, int* iso_high);
int get_effective_hdr_iso_for_display(int raw_iso);


#endif
