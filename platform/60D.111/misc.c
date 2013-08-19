// misc functions specific to 60D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

// some dummy stubs
void* AllocateMemory(size_t size) // this won't throw ERR70
{
	return (void*) AllocateMemory_do(*(int*)0x2F80, size);
}
