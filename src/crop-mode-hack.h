#ifndef __CROP_MODE_HACK_H_
#define __CROP_MODE_HACK_H_

/**
 * @defgroup FEATURE_CROP_MODE_HACK Video crop mode hack
 *
 * This hack brings video crop mode back to cameras that
 * initially didn't support it (e.g. 650D 700D 100D EOSM) 
 *
 * To make this feature work you should 
 * #define FEATURE_CROP_MODE_HACK
 * in the platform's features.h
 */

/**
 * @file crop-mode-hack.h
 * @ingroup FEATURE_CROP_MODE_HACK
 *
 * @brief Video crop mode hack public API
 */

/**
 * @brief Checks if the camera can enable video crop mode
 *
 * @retval 1 Video crop mode can be enabled
 * @retval 0 Otherwise
 */
unsigned int is_crop_hack_supported();

/**
 * @brief Enables video crop mode
 *
 * This function is safe, meaning it will
 * try to enable video crop mode only if
 * it is safe to do so.
 *
 * @retval 1 Video crop mode enabled
 * @retval 0 Otherwse
 */
unsigned int movie_crop_hack_enable();

/**
 * @brief Disables video crop mode
 *
 * This function is safe, meaning it will
 * try to disable video crop mode only if
 * it is safe to do so.
 *
 * @retval 1 Video crop mode disabled
 * @retval 0 Otherwise
 */
unsigned int movie_crop_hack_disable();

#endif // __CROP_MODE_HACK_H_
