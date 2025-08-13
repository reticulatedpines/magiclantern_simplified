#ifndef _log_d678_h_
#define _log_d678_h_

#include "dryos.h"

/* Define LOG_EARLY_STARTUP to start logging before Canon's init_task.
 * Caveat: there's no malloc working at this stage.
 *
 * Previously, we have used statically allocated buffers, but these are very small.
 * The current codebase requires a large (32MB) hardcoded buffer for debug messages,
 * and two more hardcoded buffers for io_trace.
 *
 * Do NOT try to reuse the 80D addresses!
 * Rather, you will need to find out what addresses might be unused on your camera.
 * Refer to comments in log-d6.c (TLDR: enable CONFIG_MARK_UNUSED_MEMORY_AT_STARTUP).
 */
#ifdef CONFIG_80D
#define LOG_EARLY_STARTUP
#endif

#ifdef CONFIG_5D4
#define LOG_EARLY_STARTUP
#endif

void log_start();
void log_finish();

#endif // _log_d678_h_
