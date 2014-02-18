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

/* macros from: http://gareus.org/wiki/embedding_resources_in_executables */
/**
 * @brief macros to access resource data that has been added using objcopy or ld
 */
#define EXTLD(NAME) \
  extern const unsigned char _binary_ ## NAME ## _start[]; \
  extern const unsigned char _binary_ ## NAME ## _end[]
#define LDVAR(NAME) _binary_ ## NAME ## _start
#define LDLEN(NAME) ((_binary_ ## NAME ## _end) - (_binary_ ## NAME ## _start))
  
#endif
