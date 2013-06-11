// misc functions specific to 60D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#include <version.h>

// gcc mempcy has odd alignment issues?
void
my_memcpy(
    void *       dest,
    const void *     src,
    size_t          len
)
{
    while( len-- > 0 )
        *(uint8_t*)dest++ = *(const uint8_t*)src++;
}
