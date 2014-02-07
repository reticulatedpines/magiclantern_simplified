#ifndef __UTIL_H__
#define __UTIL_H__



/**
 * @brief decrease variable at given address with interrupts disabled
 * @param value pointer to value to decrease
 */
void util_atomic_dec(uint32_t *value);

/**
 * @brief increase variable at given address with interrupts disabled
 * @param value pointer to value to increase
 */
void util_atomic_inc(uint32_t *value);

/**
 * @brief lock specified things of the user interface
 * @param what one of UILOCK_NONE, UILOCK_EVERYTHING, UILOCK_POWER_SW, etc etc. see property.h
 */
void util_uilock(int what);

#endif
