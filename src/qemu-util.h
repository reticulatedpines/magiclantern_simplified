#ifndef _qemu_util_h
#define _qemu_util_h

/* print messages to the QEMU console */
int qprintf(const char * fmt, ...);

/* print non-formatted messages */
/* (useful for large strings without risking stack overflow) */
int qprint(const char * msg);

void qemu_cam_init();
void qemu_hello();
void qemu_menu_screenshots();
void qemu_hptimer_test();

/** Some small engio API **/
#define REG_PRINT_CHAR 0xCF123000
#define REG_SHUTDOWN   0xCF123004
#define REG_DUMP_VRAM  0xCF123008
#define REG_PRINT_NUM  0xCF12300C
#define REG_BMP_VRAM   0xCF123014
#define REG_IMG_VRAM   0xCF123018
#define REG_RAW_BUFF   0xCF12301C
#define REG_DISP_TYPE  0xCF123020

#endif
