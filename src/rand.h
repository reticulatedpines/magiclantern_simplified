#ifndef __RAND_H__
#define __RAND_H__


/**
 * @brief basic random function that fills given number of 32 bit words
 * @param buffer pointer to a uint32_t based buffer
 * @param length number of uint32_t words to fill with random numbers
 */
void rand_fill(uint32_t *buffer, uint32_t length);

/**
 * @brief seed the RNG with given number
 * @param seed number used to initialize/update RNG
 */
void rand_seed(uint32_t seed);


#endif