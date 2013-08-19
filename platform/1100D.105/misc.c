// misc functions specific to 60D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>


extern void* AllocateMemory_do(void* memory_pool, size_t size);


void* AllocateMemory(size_t size) // this won't throw ERR70
{
	return (void*) AllocateMemory_do((void*)(*(int*)0x2B48), size);
}
