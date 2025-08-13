#include "dryos.h"
#include "fps-engio_per_cam.h"

int get_fps_register_a(void)
{
// TODO - 77D stubs need finding for reading these regs,
// see 200D
//    extern int _get_fps_register_a(void);
//    return _get_fps_register_a();
    return 1;
}

int get_fps_register_a_default(void)
{
// TODO - given the changes in reading Reg A, does this still work?
// Test if we can read anything from reg or shamem reg.
    return shamem_read(FPS_REGISTER_A + 4);
}

int get_fps_register_b(void)
{
//    extern int _get_fps_register_b(void);
//    return _get_fps_register_b();
    return 1;
}
