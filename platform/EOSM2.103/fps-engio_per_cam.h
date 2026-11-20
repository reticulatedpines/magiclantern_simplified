#ifndef _fps_engio_per_cam_h_
#define _fps_engio_per_cam_h_

#define FPS_REGISTER_A 0xC0F06008
#define FPS_REGISTER_B 0xC0F06014
#define FPS_REGISTER_CONFIRM_CHANGES 0xC0F06000

int get_fps_register_a(void);
int get_fps_register_a_default(void);
int get_fps_register_b(void);

#endif
