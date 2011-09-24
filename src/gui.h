#define FAKE_BTN -123456
#define IS_FAKE(event) (event->arg == FAKE_BTN)

#ifdef CONFIG_550D
#include "../platform/550D.109/gui.h"
#endif

#ifdef CONFIG_60D
#include "../platform/60D.110/gui.h"
#endif

#ifdef CONFIG_600D
#include "../platform/600D.101/gui.h"
#endif

#ifdef CONFIG_50D
#include "../platform/50D.108/gui.h"
#endif

#ifdef CONFIG_1100D
#include "../platform/1100D.104/gui.h"
#endif
