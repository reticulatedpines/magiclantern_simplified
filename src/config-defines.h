/**
 *  Conditional defines for camera internals and ML features.
 */
#ifndef PLUGIN_CLIENT
#ifndef _config_defines_h_
#define _config_defines_h_

/** 
 * Enable these for early ports
 */

    /** If CONFIG_EARLY_PORT is defined, only a few things will be enabled (e.g. changing version string) */
    //~ #define CONFIG_EARLY_PORT

    /** Load fonts and print Hello World (disable CONFIG_EARLY_PORT); will not start any other ML tasks, handlers etc. */
    //~ #define CONFIG_HELLO_WORLD

/**
 * Some common stuff - you can override them in platform files
 */

    /** If something goes wrong (ERR70), we can save a crash log **/
    #define CONFIG_CRASH_LOG

    /** It's a good idea to back up ROM contents on the card - just in case **/
    #define CONFIG_AUTOBACKUP_ROM

    /** It's a good idea to run some automated tests **/
    #define CONFIG_STRESS_TEST
    #define CONFIG_BENCHMARKS

    /** You may want to disable this for troubleshooting **/
    #define CONFIG_CONFIG_FILE

    /** This may help discovering some cool new stuff - http://magiclantern.wikia.com/wiki/Register_Map/Brute_Force **/
    /** For developers only; can be dangerous **/
    //~ #define CONFIG_DIGIC_POKE

    /** A bunch of debug tools **/
    //~ #define CONFIG_DEBUGMSG

    /** Useful to test battery consumption without any other ML code running **/
    //~ #define CONFIG_BATTERY_TEST

/** What internals do we have on each camera? **/
#include "internals.h" // from platform directory

/** What features are enabled on each camera? **/
#include "features.h" // from platform directory

#endif
#endif
