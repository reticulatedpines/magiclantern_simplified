#ifndef _cpu_h_
#define _cpu_h_

// A file for code that manipulates CPU functionality.

#if defined(CONFIG_DUAL_CORE)
extern int cpu1_suspended;
void suspend_cpu1(void);
#endif // CONFIG_DUAL_CORE

#endif // _cpu_h_
