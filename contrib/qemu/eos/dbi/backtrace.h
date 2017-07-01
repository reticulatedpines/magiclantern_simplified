#ifndef _backtrace_h_
#define _backtrace_h_

    /* similar to callstack, but does not require any instrumentation
     * it works by walking the stack backwards, with some code analysis
     * to figure out the offsets where LRs are stored */
    void eos_backtrace_rebuild(EOSState *s, char * buf, int size);

    /* stack walker configuration */
    #define BKT_ASSUME_TAIL_CALL_AFTER_POP_LR
    //#define BKT_HANDLE_UNLIKELY_CASES
    //#define RANDOM_BRANCHES             /* slow, minor improvement */

    /* self-tests */
    //#define BKT_CROSSCHECK_EXEC
    //#define BKT_CROSSCHECK_CALLSTACK

    #ifdef BKT_CROSSCHECK_CALLSTACK
    #define BKT_TRACK_STATS
    #endif

    /* debugging */
    //#define BKT_LOG_VERBOSE EOS_LOG_VERBOSE
    //#define BKT_LOG_DISAS EOS_LOG_VERBOSE
    #define BKT_LOG_VERBOSE 0
    #define BKT_LOG_DISAS 0

    /* internal, for BKT_CROSSCHECK_EXEC */
    void eos_bkt_log_exec(EOSState *s);

#endif
