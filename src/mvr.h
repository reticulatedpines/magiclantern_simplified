#ifdef CONFIG_PLUGIN
// do not include camera-specific constants in plugins
#elif defined(CONFIG_550D)
#include "../platform/550D.109/mvr.h"
#elif defined(CONFIG_60D)
#include "../platform/60D.111/mvr.h"
#elif defined(CONFIG_600D)
#include "../platform/600D.102/mvr.h"
#elif defined(CONFIG_50D)
#include "../platform/50D.109/mvr.h"
#elif defined(CONFIG_500D)
#include "../platform/500D.111/mvr.h"
#elif defined(CONFIG_1100D)
#include "../platform/1100D.105/mvr.h"
#elif defined(CONFIG_5D2)
#include "../platform/5D2.212/mvr.h"
#elif defined(CONFIG_5D3)
#include "../platform/5D3.113/mvr.h"
#elif defined(CONFIG_7D)
#include "../platform/7D.203/mvr.h"
#elif defined(CONFIG_EOSM)
#include "../platform/EOSM.100/mvr.h"
#else

#error This file does not contain this camera model. Please add it.

#endif
