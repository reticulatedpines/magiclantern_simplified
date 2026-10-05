#ifndef _edmac_per_cam_h_
#define _edmac_per_cam_h_

/*Read 0x19 0x0D 0x0B 0x0A(82MB/S)*/
/* Write 0x5 0x6 0x4 (LV) */
//5 zoom, 6 not - improved performance (no HDMI related tearing)

#define EDMAC_READ_CHAN 0xa
#define EDMAC_WRITE_CHAN 0x6

#endif
