#ifndef _timer_h
#define _timer_h

/*
 * DryOS Timers.
 *
 * Return codes of Set*Timer* calls:
 * ---------------------------------
 * (retVal & 1) -> Generic error, code retVal
 * 0x05 -> NOT_ENOUGH_MEMORY
 * 0x15 -> OVERRUN
 * 0x17 -> NOT_INITIALIZED
 * (Otherwise) -> timerId
 */

/**
 * A timer callback function.
 * It gets executed in the timer interrupt context
 * The first argument is the timestamp in ms/us when the timer fired
 * The second arguments is the one passed to Set* functions
 */
typedef void(*timerCbr_t)(int, void*);

/* 
 * Slow Timers
 *
 * SetTimerAfter -> Run after delayMs has passed (calls the CBR).
 * CancelTimer -> stop a running timer (will no longer call the CBR).
 * 
 * Example:
 *      timer_id = SetTimerAfter(1000, timer_cbr, timer_cbr, 0);
 * 
 * If you change your mind and want to stop the timer:
 *      CancelTimer(timer_id);
 * 
 * From a timer CBR, don't you are in an interrupt context.
 * You can send messages to a message queue, you can give a semaphore,
 * but you can't take a semaphore (so you can't call NotifyBox).
 *
 */
extern int SetTimerAfter(int delay_ms, timerCbr_t timer_cbr, timerCbr_t overrun_cbr, void* priv);
extern void CancelTimer(int timer_id);

/*
 * High Precision Timers
 *
 * Examples:
 * --------
 * SetHPTimerAfterNow(0xAA0, timer_cbr, overrun_cbr, 0); // Fire after 0xAA0 uS
 * SetHPTimerNextTick() is to be called e.g. from the cbr to setup the next timer
 * 
 * The microsecond timer wraps around after 1048576 (20 bits).
 */
extern int SetHPTimerAfterNow(int delay_us, timerCbr_t timer_cbr, timerCbr_t overrun_cbr, void* priv);
extern int SetHPTimerNextTick(int last_expiry, int offset, timerCbr_t timer_cbr, timerCbr_t overrun_cbr, void *priv);

#endif //_timer_h
