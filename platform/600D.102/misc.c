// misc functions specific to 600D/102

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

// some dummy stubs
int lcd_release_running = 0;
void lcd_release_step() {};
int get_lcd_sensor_shortcuts() { return 0; }
void display_lcd_remote_icon(int x0, int y0) {}

void* AllocateMemory(size_t size) // this won't throw ERR70
{
	return (void*) AllocateMemory_do(*(int*)0x3070, size);
}
