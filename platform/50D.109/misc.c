// misc functions specific to 50D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

// some dummy stubs
int new_LiveViewApp_handler = 0xff123456;

// 50D does not have audio
int sound_recording_enabled_canon() { return 0; }
int sound_recording_enabled() { return 0; }
int audio_meters_are_drawn() { return 0; }
void volume_up(){};
void volume_down(){};
void out_volume_up(){};
void out_volume_down(){};
