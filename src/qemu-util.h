#ifndef _qemu_util_h
#define _qemu_util_h

int qprintf(const char * fmt, ...); // prints in the QEMU console

void qemu_cam_init();
void qemu_hello();
void qemu_menu_screenshots();

#endif
