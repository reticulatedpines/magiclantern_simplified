
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
#define TRACE_FMT_TASK_ID         0x0080 /* write the task id */
#define TRACE_FMT_TASK_NAME       0x0100 /* write the task name */
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
    uint32_t used;
    char name[TRACE_MAX_STRING];
    char file_name[TRACE_MAX_STRING];
    struct msg_queue *queue;
    
    /* format options */
    uint32_t format;
    char separator;
    uint32_t sleep_time;
    uint32_t max_entries;
    uint32_t cur_entries;
    
    /* runtime variables */
    FILE *file_handle;
    char *buffer;
    uint32_t buffer_read_pos;
    uint32_t buffer_write_pos;
    uint32_t buffer_size;
    uint32_t buffer_written;
    tsc_t start_tsc;
    tsc_t last_tsc;
    
    /* task status */
    uint32_t task_state;
    uint32_t task;
} trace_entry_t;


#if defined(TRACE_DISABLED)

#define trace_available()                            (void)0
#define trace_start(name, file_name)                 0
#define trace_stop(trace, wait)                      (void)0
#define trace_format(context, format, separator)     (void)0
#define trace_set_flushrate(context, timeout)        (void)0
#define trace_flush(context)                         (void)0
#define trace_write(context, string, ...)            (void)0
#define trace_write_tsc(context, tsc, string, ...)   (void)0
#define trace_vwrite(context, tsc, string, ap)       (void)0
#define trace_write_binary(context, buffer, length)  (void)0

#else

#if defined(MODULE)
/* check if the module is available */
uint32_t EXT_WEAK_FUNC(ret_0) trace_available();
/* create a new trace with given short name and filename */
uint32_t EXT_WEAK_FUNC(ret_0) trace_start(char *name, char *file_name);
/* free a previously created trace context */
uint32_t EXT_WEAK_FUNC(ret_0) trace_stop(uint32_t trace, uint32_t wait);
/* setup some custom format options. when separator is a null byte, it will be omitted */
uint32_t EXT_WEAK_FUNC(ret_0) trace_format(uint32_t context, uint32_t format, char separator);
uint32_t EXT_WEAK_FUNC(ret_0) trace_set_flushrate(uint32_t context, uint32_t timeout);
uint32_t EXT_WEAK_FUNC(ret_0) trace_flush(uint32_t context);
/* write some string into specified trace */
uint32_t EXT_WEAK_FUNC(ret_0) trace_write(uint32_t context, char *string, ...);
uint32_t EXT_WEAK_FUNC(ret_0) trace_write_tsc(uint32_t context, tsc_t tsc, char *string, ...);
uint32_t EXT_WEAK_FUNC(ret_0) trace_vwrite(uint32_t context, tsc_t tsc, char *string, va_list ap);
/* write some binary data into specified trace with an variable length field in front */
uint32_t EXT_WEAK_FUNC(ret_0) trace_write_binary(uint32_t context, uint8_t *buffer, uint32_t length);
#else
static uint32_t (*trace_available)() = MODULE_FUNCTION(trace_available);
static uint32_t (*trace_start)(char *name, char *file_name) = MODULE_FUNCTION(trace_start);
static uint32_t (*trace_stop)(uint32_t trace, uint32_t wait) = MODULE_FUNCTION(trace_stop);
static uint32_t (*trace_format)(uint32_t context, uint32_t format, char separator) = MODULE_FUNCTION(trace_format);
static uint32_t (*trace_set_flushrate)(uint32_t context, uint32_t timeout) = MODULE_FUNCTION(trace_set_flushrate);
static uint32_t (*trace_flush)(uint32_t context) = MODULE_FUNCTION(trace_flush);
static uint32_t (*trace_write)(uint32_t context, char *string, ...) = MODULE_FUNCTION(trace_write);
static uint32_t (*trace_write_tsc)(uint32_t context, uint64_t tsc, char *string, ...) = MODULE_FUNCTION(trace_write_tsc);
static uint32_t (*trace_vwrite)(uint32_t context, tsc_t tsc, char *string, va_list ap) = MODULE_FUNCTION(trace_vwrite);
static uint32_t (*trace_write_binary)(uint32_t context, uint8_t *buffer, uint32_t length) = MODULE_FUNCTION(trace_write_binary);

#endif
#endif

/* internal */
static uint32_t trace_write_varlength(uint32_t context, uint32_t length);



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
