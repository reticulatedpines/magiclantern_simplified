// some dummy stubs
#include "dryos.h"

uint32_t shamem_read(uint32_t addr) { return 0; } // or maybe return MEM(addr)
void _EngDrvOut(int addr, int value) { MEM(addr) = value; }

int new_LiveViewApp_handler = 0xff123456;

void free_space_show_photomode(){}

int audio_meters_are_drawn() { return 0; }
void volume_up(){};
void volume_down(){};
void out_volume_up(){};
void out_volume_down(){};

int is_mvr_buffer_almost_full() { return 0; }
void movie_indicators_show(){}
void bitrate_mvr_log(){}

int time_indic_x =  720 - 160;
int time_indic_y = 0;

void free_space_show(){};
void fps_show(){};

struct memChunk * GetNextMemoryChunk(struct memSuite * suite, struct memChunk * chunk) { return 0; }
