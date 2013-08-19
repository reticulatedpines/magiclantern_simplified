// misc functions specific to 600D/102

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

// some dummy stubs
void* AllocateMemory(size_t size) // this won't throw ERR70
{
	return (void*) AllocateMemory_do(*(int*)0x3070, size);
}
