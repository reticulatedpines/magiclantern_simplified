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

#endif
