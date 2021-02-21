#include "compiler.h"

/* used only if patch manager is not included */
void __attribute__((weak)) sync_caches()
{
    _sync_caches();
}
