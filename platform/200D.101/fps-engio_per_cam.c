#include "dryos.h"
#include "fps-engio_per_cam.h"

int get_fps_register_a(void)
{
    extern int _get_fps_register_a(void);
    return _get_fps_register_a();
}

int get_fps_register_a_default(void)
{
// TODO - given the changes in reading Reg A, does this still work?
// Test if we can read anything from reg or shamem reg.
// No way found so far...
//    return shamem_read(FPS_REGISTER_A + 4);

    // Don't know a way of reading the value from cam yet,
    // may not be possible?
    // For now, we return a valid value logged from real cam.
    // I *think* the shift is because these are u16s really,
    // but this one is not at a word boundary, and old cams
    // couldn't easily read from unaligned values.
    // So we read the wrong address and shift it, every time
    // we need it...  awesome.  Hence, here, we shift
    // in order to correct for that later correction.  Yuck.
    return 1122 << 16;
}

int get_fps_register_b(void)
{
    extern int _get_fps_register_b(void);
    return _get_fps_register_b();
}
