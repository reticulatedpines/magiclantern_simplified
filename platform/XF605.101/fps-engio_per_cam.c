#include "dryos.h"
#include "fps-engio_per_cam.h"

int get_fps_register_a(void)
{
//    return shamem_read(FPS_REGISTER_A);
    return 1;
}

int get_fps_register_a_default(void)
{
//    return shamem_read(FPS_REGISTER_A + 4);
    return 1;
}

int get_fps_register_b(void)
{
//    return shamem_read(FPS_REGISTER_B);
    return 1;
}
