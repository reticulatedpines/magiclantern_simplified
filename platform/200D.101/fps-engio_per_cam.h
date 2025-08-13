#ifndef _fps_engio_per_cam_h_
#define _fps_engio_per_cam_h_

#define FPS_REGISTER_A 0xd0006008
#define FPS_REGISTER_B 0xd0006014
#define FPS_REGISTER_CONFIRM_CHANGES 0xd0006000

// needed for NEW_FPS_METHOD to build
//
// Haven't got this working yet.  The table data looks different.
// Still seems to hold u16 command values, but it's also sparser,
// and I think larger.  Unsure if related to the problems I see,
// but worth checking how pos is used to index into it, does it still
// make sense?
//
//#define SENSOR_TIMING_TABLE MEM(0xdf20)
//#define SENSOR_TIMING_TABLE_SIZE 350 // SJE FIXME: just so it builds for now

int get_fps_register_a(void);
int get_fps_register_a_default(void);
int get_fps_register_b(void);

#endif
