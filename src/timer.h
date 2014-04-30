#ifndef _timer_h
#define _timer_h

/*
 * DryOS Timers.
 *
 * TODO: Possbile Return codes of Set* calls
 * ---------------------
 * (retVal & 1) -> Generic error, code retVal
 * 0x05 -> NOT_ENOUGH_MEMORY
 * 0x15 -> OVERRUN
 * 0x17 -> NOT_INITIALIZED
 * (Otherwise) -> timerId
 */

/**
 * A timer callback function.
 * It gets executed in the timer interrupt context
 * The first argument is possibly the global timestamp expressed in ms
 * The second arguments is the one passed to Set* functions
 */
typedef void(*timerCbr_t)(int, void*);

/* 
 * Slow Timers
 *
 * Hypotesis: 
 * ----------
 * SetTimerWhen  -> Run when (timestampMs - get_current_time_in_ms()) reaches 0
 * SetTimerAfter -> Run after delayMs has passed
 *
 */
extern int SetTimerAfter(int timestampMs, timerCbr_t delayed_cbr, timerCbr_t overrun_cbr, void* priv);
extern int SetTimerWhen(int delayMs, timerCbr_t delayed_cbr, timerCbr_t overrun_cbr, void* priv);
extern void TimerCancel(int timerId);

/*
 * High Precision Timers
 *
 * Examples:
 * --------
 * SetHPTimerAfterNow(0xAA0, delayed_cbr, instant_cbr, 0); // Fire after 0xAA0 uS
 * SetHPTimerAfterTimeout((getDigicTime() + delay) & 0xFFFFF, cbr, 0); // Fire after delay 'ticks' using getDigicTime() as base
 */
extern int SetHPTimerAfterTimeout(int timer_base, timerCbr_t cbr, void* priv);
extern int SetHPTimerAfterNow(int delayUs, timerCbr_t delayed_cbr, timerCbr_t overrun_cbr, void* priv);



#endif //_timer_h
