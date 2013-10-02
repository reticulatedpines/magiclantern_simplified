
#ifndef __trace_h__
#define __trace_h__

#ifdef __trace_c__
    #define EXT_WEAK_FUNC(f) 
#else
    #define EXT_WEAK_FUNC(f) WEAK_FUNC(f)
#endif

#define TRACE_MAX_STRING       64
#define TRACE_MAX_LINE_LENGTH 256
#define TRACE_MAX_CONTEXT      16

#define TRACE_FMT_TIME_CTR        0x0001 /* write the absolute TSC value as integer */
#define TRACE_FMT_TIME_CTR_REL    0x0002 /* write the relative TSC since last write value as integer */
#define TRACE_FMT_TIME_CTR_DELTA  0x0004 /* write the relative TSC since last write value as integer */
#define TRACE_FMT_TIME_ABS        0x0008 /* write the absolute time as hh:mm:ss.msec*/
#define TRACE_FMT_TIME_REL        0x0010 /* write the time since start as hh:mm:ss.msec*/
#define TRACE_FMT_TIME_DELTA      0x0020 /* write the relative time as hh:mm:ss.msec since last entry*/
#define TRACE_FMT_TIME_DATE       0x0040 /* write the time of day */
#define TRACE_FMT_COMMENT         0x1000 /* headers are C like comments */

#define TRACE_FMT_META            0x0100 /* on start and stop write some metadata (e.g. day, time, ...) */

#define TRACE_FMT_DEFAULT        (TRACE_FMT_META | TRACE_FMT_TIME_CTR_DELTA | TRACE_FMT_TIME_DELTA)

#define TRACE_SEPARATOR_DEFAULT ' '

#define TRACE_TASK_STATE_DEAD     0
#define TRACE_TASK_STATE_RUNNING  1
#define TRACE_TASK_STATE_SHUTDOWN 2

#define TRACE_SLEEP_TIME   250
#define TRACE_BUFFER_SIZE (512*1024)

#define TRACE_ERROR 0xFFFFFFFF
#define TRACE_OK    0

typedef unsigned long long tsc_t;

typedef struct
{
    int used;
    char name[TRACE_MAX_STRING];
    char file_name[TRACE_MAX_STRING];
    struct msg_queue *queue;
    
    /* format options */
    unsigned int format;
    unsigned char separator;
    unsigned int sleep_time;
    unsigned int max_entries;
    unsigned int cur_entries;
    
    /* runtime variables */
    FILE *file_handle;
    char *buffer;
    unsigned int buffer_read_pos;
    unsigned int buffer_write_pos;
    unsigned int buffer_size;
    unsigned int buffer_written;
    tsc_t start_tsc;
    tsc_t last_tsc;
    
    /* task status */
    unsigned int task_state;
    unsigned int task;
} trace_entry_t;

/* check if the module is available */
unsigned int EXT_WEAK_FUNC(ret_0) trace_available();
/* create a new trace with given short name and filename */
unsigned int EXT_WEAK_FUNC(ret_0) trace_start(char *name, char *file_name);
/* free a previously created trace context */
unsigned int EXT_WEAK_FUNC(ret_0) trace_stop(unsigned int trace, int wait);
/* setup some custom format options. when separator is a null byte, it will be omitted */
unsigned int EXT_WEAK_FUNC(ret_0) trace_format(unsigned int context, unsigned int format, unsigned char separator);
unsigned int EXT_WEAK_FUNC(ret_0) trace_set_flushrate(unsigned int context, unsigned int timeout);
/* write some string into specified trace */
unsigned int EXT_WEAK_FUNC(ret_0) trace_write(unsigned int context, char *string, ...);
unsigned int EXT_WEAK_FUNC(ret_0) trace_write_tsc(unsigned int context, tsc_t tsc, char *string, ...);
unsigned int EXT_WEAK_FUNC(ret_0) trace_vwrite(unsigned int context, tsc_t tsc, char *string, va_list ap);
/* write some binary data into specified trace with an variable length field in front */
unsigned int EXT_WEAK_FUNC(ret_0) trace_write_binary(unsigned int context, unsigned char *buffer, unsigned int length);

/* internal */
static unsigned int trace_write_varlength(unsigned int context, unsigned int length);



/* binary format:
 *
 *  TT TT TT TT VL [LL [LL [LL]]]
 *
 *  T = Time stamp counter (little endian)
 *  V = Variable length field: specifies how many extra length bytes follow (0-3)
 *  L = Length field in little endian
 *      e.g. 0x0000003 bytes are encoded: "03"
 *      e.g. 0x0000023 bytes are encoded: "13 02"
 *      e.g. 0x0006503 bytes are encoded: "23 50 06"
 *      e.g. 0xA1F7654 bytes are encoded: "34 65 F7 A1"
 */
 
 
#endif
