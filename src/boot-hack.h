#ifndef __BOOT_HACK_H_
#define __BOOT_HACK_H_

int magic_is_off();
void _disable_ml_startup();

/* Blocks execution until config is read */
void hold_your_horses();

#endif // __BOOT_HACK_H
