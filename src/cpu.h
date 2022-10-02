#ifndef _cpu_h_
#define _cpu_h_

// A file for code that manipulates CPU functionality.

#if defined(CONFIG_DUAL_CORE)
void suspend_cpu1(void);
int wait_for_cpu1_to_suspend(int32_t timeout);
#endif // CONFIG_DUAL_CORE

#endif // _cpu_h_
