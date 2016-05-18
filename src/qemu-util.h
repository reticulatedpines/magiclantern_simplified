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

#endif
