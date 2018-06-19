
#define __trace_c__


#include <stdint.h>
#include <string.h>

#include <module.h>
#include <dryos.h>
#include <bmp.h>


#include "trace.h"

static trace_entry_t trace_contexts[TRACE_MAX_CONTEXT];

extern tsc_t get_us_clock();

/* general selection of allocation method */
static void *trace_alloc(uint32_t size)
{
    return malloc(size);
}

static void trace_free(void *data)
{
    free(data);
}

static void trace_task(volatile trace_entry_t *ctx)
{
    char *write_buf = trace_alloc(ctx->buffer_size);
    ctx->task_state = TRACE_TASK_STATE_RUNNING;

    while((ctx->task_state == TRACE_TASK_STATE_RUNNING) || (ctx->buffer_read_pos != ctx->buffer_write_pos))
    {
        /* when shutdown requested, try to save buffer */
        if(ml_shutdown_requested)
        {
            ctx->task_state = TRACE_TASK_STATE_SHUTDOWN;
        }
        
        if(ctx->buffer_read_pos != ctx->buffer_write_pos)
        {
            uint32_t used = 0;
            uint32_t wr_pos = ctx->buffer_write_pos;
            
            if(wr_pos > ctx->buffer_read_pos)
            {
                used = wr_pos - ctx->buffer_read_pos;
            }
            else
            {
                used = ctx->buffer_size - ctx->buffer_read_pos;
            }
            memcpy(write_buf, &ctx->buffer[ctx->buffer_read_pos], used);
          
            ctx->buffer_read_pos += used;
            ctx->buffer_read_pos %= ctx->buffer_size;

            uint32_t total = 0;
            while(total < used)
            {
                /* write as much as possible at once */
                uint32_t written = FIO_WriteFile(ctx->file_handle, &write_buf[total], used - total);

                /* check for writing errors */
                if(written > 0)
                {
                    total += written;
                }
                else
                {
                    FIO_CloseFile(ctx->file_handle);
                    trace_free(ctx->buffer);
                    trace_free(write_buf);
                    ctx->task_state = TRACE_TASK_STATE_DEAD;
                    return;
                }
            }
        }
        else
        {
            uint32_t dummy = 0;
            msg_queue_receive(ctx->queue, &dummy, ctx->sleep_time);
        }
    }

    /* we are doing the cleanup in this task */
    FIO_CloseFile(ctx->file_handle);
    trace_free(ctx->buffer);
    trace_free(write_buf);
    
    ctx->file_handle = NULL;
    ctx->buffer = NULL;
    ctx->task_state = TRACE_TASK_STATE_DEAD;
    ctx->used = 0;
}

uint32_t trace_start(char *name, char *file_name)
{
    uint32_t pos = 0;
    
    /* small atomic lock here to stay thread safe */
    uint32_t old_stat = cli();
    for(pos = 0; pos < TRACE_MAX_CONTEXT; pos++)
    {
        if(!trace_contexts[pos].used)
        {
            trace_contexts[pos].used = 1;
            break;
        }
    }
    sei(old_stat);
    
    if(pos >= TRACE_MAX_CONTEXT)
    {
        return TRACE_ERROR;
    }
    trace_entry_t *ctx = &trace_contexts[pos];

    /* init fields */
    ctx->format = TRACE_FMT_DEFAULT;
    ctx->separator = TRACE_SEPARATOR_DEFAULT;
    ctx->sleep_time = TRACE_SLEEP_TIME;
    ctx->buffer_size = TRACE_BUFFER_SIZE;
    ctx->buffer_read_pos = 0;
    ctx->buffer_write_pos = 0;
    ctx->buffer_written = 0;
    ctx->max_entries = 1000000;
    ctx->cur_entries = 0;

    /* copy strings */
    strncpy((char *)ctx->name, name, sizeof(ctx->name));
    strncpy((char *)ctx->file_name, file_name, sizeof(ctx->file_name));

    /* start worker task */
    ctx->queue = msg_queue_create("trace_wake", 1);
    ctx->task_state = TRACE_TASK_STATE_DEAD;
    ctx->task = (uint32_t)task_create("trace_task", 0x18, 0x1000, &trace_task, (void *)ctx);
    
    /* create trace file */
    FIO_RemoveFile(ctx->file_name);
    ctx->file_handle = FIO_CreateFile(ctx->file_name);
    if (!ctx->file_handle)
    {
        ctx->used = 0;
        return TRACE_ERROR;
    }

    /* allocate write buffer */
    ctx->buffer = trace_alloc(ctx->buffer_size);
    if(!ctx->buffer)
    {
        FIO_CloseFile(ctx->file_handle);
        ctx->used = 0;
        return TRACE_ERROR;
    }

    ctx->start_tsc = get_us_clock();
    ctx->last_tsc = ctx->start_tsc;

    /* make sure task is running */
    for(uint32_t loops = 0; loops < 250; loops++)
    {
        if(ctx->task_state != TRACE_TASK_STATE_DEAD)
        {
            return pos;
        }
        msleep(20);
    }

    /* timed out, return gracefully */
    ctx->used = 0;
    FIO_CloseFile(ctx->file_handle);
    trace_free(ctx->buffer);
    ctx->file_handle = NULL;
    ctx->buffer = NULL;

    return TRACE_ERROR;
}

uint32_t trace_stop(uint32_t context, uint32_t wait)
{
    trace_entry_t *ctx = &trace_contexts[context];

    if(context >= TRACE_MAX_CONTEXT || !ctx->used)
    {
        return TRACE_ERROR;
    }

    ctx->task_state = TRACE_TASK_STATE_SHUTDOWN;

    if(!wait)
    {
        return TRACE_OK;
    }

    /* wait for task to terminate */
    for(uint32_t loops = 0; loops < 500; loops++)
    {
        if(ctx->task_state == TRACE_TASK_STATE_DEAD)
        {
            return TRACE_OK;
        }
        msleep(20);
    }

    /* try to clean up as much as possible */
    FIO_CloseFile(ctx->file_handle);
    trace_free(ctx->buffer);

    ctx->file_handle = NULL;
    ctx->buffer = NULL;
    ctx->used = 0;

    /* we cannot handle this. is the task dead or not? */
    return TRACE_ERROR;
}

/* setup some custom format options. when separator is a null byte, it will be omitted */
uint32_t trace_format(uint32_t context, uint32_t format, char separator)
{
    trace_entry_t *ctx = &trace_contexts[context];

    if(context >= TRACE_MAX_CONTEXT || !ctx->used)
    {
        return TRACE_ERROR;
    }

    ctx->format = format;
    ctx->separator = separator;

    return TRACE_OK;
}

uint32_t trace_set_flushrate(uint32_t context, uint32_t timeout)
{
    trace_entry_t *ctx = &trace_contexts[context];

    if(context >= TRACE_MAX_CONTEXT || !ctx->used)
    {
        return TRACE_ERROR;
    }

    ctx->sleep_time = timeout;

    return TRACE_OK;
}

uint32_t trace_flush(uint32_t context)
{
    trace_entry_t *ctx = &trace_contexts[context];

    if(context >= TRACE_MAX_CONTEXT || !ctx->used)
    {
        return TRACE_ERROR;
    }

    msg_queue_post(ctx->queue, (uint32_t)ctx);
    
    return TRACE_OK;
}


static uint32_t trace_write_timestamp(trace_entry_t *ctx, uint32_t type, tsc_t tsc, char *timestamp, uint32_t *timestamp_pos)
{
    uint32_t pos = *timestamp_pos;

    switch(type)
    {
        case TRACE_FMT_TIME_CTR:
        {
            /* write raw timestamp counter "000015255" */
            char tmp[32];

            snprintf(tmp, sizeof(tmp), "%08X%08X", (uint32_t)(tsc >> 32), (uint32_t)tsc);
            memcpy(&timestamp[pos], tmp, 17);
            pos += 16;
            break;
        }
        case TRACE_FMT_TIME_ABS:
        {
            /* write time since measurement start "0:0:36.309" */
            char tmp[32];
            
            uint32_t usec = tsc % 1000000ULL;
            uint32_t sec_total = tsc / 1000000ULL;
            uint32_t sec = sec_total % 60;
            uint32_t min = (sec_total / 60) % 60;
            uint32_t hrs = (sec_total / 3600) % 60;

            snprintf(tmp, sizeof(tmp), "%02d:%02d:%02d.%06d", hrs, min, sec, usec);
            strcpy((char *)&timestamp[pos], tmp);
            pos += 15;
            break;
        }
        case TRACE_FMT_TIME_DATE:
        {
            break;
        }
    }
    
    /* add separator */
    timestamp[pos] = ctx->separator;
    pos++;
    timestamp[pos] = 0;
    
    *timestamp_pos = pos;
    
    return TRACE_OK;
}

uint32_t trace_vwrite(uint32_t context, tsc_t tsc, char *string, va_list ap)
{
    uint32_t linebuffer_pos = 0;
    trace_entry_t *ctx = &trace_contexts[context];

    /* make sure ctx is valid, this ctx is used and the writer thread is running */
    if(context >= TRACE_MAX_CONTEXT || !ctx->used || ctx->task_state != TRACE_TASK_STATE_RUNNING)
    {
        return TRACE_OK;
    }
    
    /* build timestamp string */
    uint32_t max_len = TRACE_MAX_LINE_LENGTH;
    char *linebuffer = malloc(max_len + 1);
    linebuffer[linebuffer_pos] = 0;
    
    if(ctx->format & TRACE_FMT_COMMENT)
    {
        linebuffer[linebuffer_pos++] = '/';
        linebuffer[linebuffer_pos++] = '*';
        linebuffer[linebuffer_pos++] = ' ';
        linebuffer[linebuffer_pos] = '\000';
    }  
    if(ctx->format & TRACE_FMT_TIME_CTR)
    {
        trace_write_timestamp(ctx, TRACE_FMT_TIME_CTR, tsc, linebuffer, &linebuffer_pos);
    }
    if(ctx->format & TRACE_FMT_TIME_CTR_REL)
    {
        tsc_t delta = tsc - ctx->start_tsc;
        trace_write_timestamp(ctx, TRACE_FMT_TIME_CTR, delta, linebuffer, &linebuffer_pos);
    }
    if(ctx->format & TRACE_FMT_TIME_CTR_DELTA)
    {
        tsc_t delta = tsc - ctx->last_tsc;
        trace_write_timestamp(ctx, TRACE_FMT_TIME_CTR, delta, linebuffer, &linebuffer_pos);
    }
    if(ctx->format & TRACE_FMT_TIME_ABS)
    {
        trace_write_timestamp(ctx, TRACE_FMT_TIME_ABS, tsc, linebuffer, &linebuffer_pos);
    }
    if(ctx->format & TRACE_FMT_TIME_REL)
    {
        tsc_t delta = tsc - ctx->start_tsc;
        trace_write_timestamp(ctx, TRACE_FMT_TIME_ABS, delta, linebuffer, &linebuffer_pos);
    }
    if(ctx->format & TRACE_FMT_TIME_DELTA)
    {
        tsc_t delta = tsc - ctx->last_tsc;
        trace_write_timestamp(ctx, TRACE_FMT_TIME_ABS, delta, linebuffer, &linebuffer_pos);
    }
    if(ctx->format & TRACE_FMT_TIME_DATE)
    {
        trace_write_timestamp(ctx, TRACE_FMT_TIME_DATE, tsc, linebuffer, &linebuffer_pos);
    }
    if(ctx->format & TRACE_FMT_TASK_ID)
    {
        char tmp[32];
        
        linebuffer[linebuffer_pos++] = ' ';
        snprintf(tmp, sizeof(tmp), "%d", (current_task->taskId & 0xFF));
        memcpy(&linebuffer[linebuffer_pos], tmp, strlen(tmp));
        linebuffer_pos += strlen(tmp);
        linebuffer[linebuffer_pos++] = ' ';
        linebuffer[linebuffer_pos] = '\000';
    }
    if(ctx->format & TRACE_FMT_TASK_NAME)
    {
        char *name = get_current_task_name();
        
        linebuffer[linebuffer_pos++] = ' ';
        memcpy(&linebuffer[linebuffer_pos], name, strlen(name));
        linebuffer_pos += strlen(name);
        linebuffer[linebuffer_pos++] = ' ';
        linebuffer[linebuffer_pos] = '\000';
    }
    if(ctx->format & TRACE_FMT_COMMENT)
    {
        linebuffer[linebuffer_pos++] = ' ';
        linebuffer[linebuffer_pos++] = '*';
        linebuffer[linebuffer_pos++] = '/';
        linebuffer[linebuffer_pos++] = ' ';
        linebuffer[linebuffer_pos] = '\000';
    }  

    /* update last tsc */
    ctx->last_tsc = tsc;
    
    /* now fill that into the line buffer */
    uint32_t len = 0;
    
    if(string)
    {
        len = vsnprintf(&linebuffer[linebuffer_pos], max_len - linebuffer_pos, string, ap);
    }
    
    if(len > 0)
    {
        linebuffer_pos += len;
    }
    
    /* attach a newline */
    linebuffer[linebuffer_pos++] = '\n';
    
    /* check and reserve memory in ringbuffer - use interrupt disabling as mutex */
    uint32_t old_int = cli();
    uint32_t read_pos = ctx->buffer_read_pos;
    uint32_t write_pos = ctx->buffer_write_pos;
    uint32_t available = 0;

    /*  */
    if(write_pos > read_pos)
    {
        available = ctx->buffer_size - (write_pos - read_pos) - 1;
    }
    else
    {
        available = read_pos - write_pos - 1;
    }
    
    if(linebuffer_pos < available)
    {
        ctx->buffer_write_pos += linebuffer_pos;
        ctx->buffer_write_pos %= ctx->buffer_size;
        ctx->buffer_written += linebuffer_pos;
    }
    
    /* seems the string was too long */
    if(linebuffer_pos > available)
    {
        /* abort trace as data will be lost */
        ctx->task_state = TRACE_TASK_STATE_SHUTDOWN;
        sei(old_int);
        free(linebuffer);
        return TRACE_ERROR;
    }

    /* successful, commit the buffer content */
    uint32_t commit_size = 0;
    
    if(write_pos > read_pos)
    {
        commit_size = ctx->buffer_size - write_pos;
    }
    else
    {
        commit_size = read_pos - write_pos - 1;
    }
    commit_size = MIN(linebuffer_pos,commit_size);
    
    memcpy(&ctx->buffer[write_pos], linebuffer, commit_size);
    
    if(commit_size < linebuffer_pos)
    {
        memcpy(ctx->buffer, &linebuffer[commit_size], linebuffer_pos - commit_size);
    }
    
    sei(old_int);
    
    /* wake up writer if buffer is getting full */
    if(ctx->buffer_written > ctx->buffer_size / 2)
    {
        ctx->buffer_written = 0;
        msg_queue_post(ctx->queue, (uint32_t)ctx);
    }
    
    /* reached the maximum allowed number of entries? */
    ctx->cur_entries++;
    if(ctx->cur_entries >= ctx->max_entries)
    {
        /* finish trace */
        ctx->task_state = TRACE_TASK_STATE_SHUTDOWN;
        msg_queue_post(ctx->queue, (uint32_t)ctx);
    }

    free(linebuffer);
    return TRACE_OK;
}

/* write some string into specified trace */
uint32_t trace_write(uint32_t context, char *string, ...)
{
    va_list ap;
    
    va_start(ap, string);
    uint32_t ret = trace_vwrite(context, get_us_clock(), string, ap);
    va_end(ap);
    
    return ret;
}

uint32_t trace_write_tsc(uint32_t context, tsc_t tsc, char *string, ...)
{
    va_list ap;
    
    va_start(ap, string);
    uint32_t ret = trace_vwrite(context, tsc, string, ap);
    va_end(ap);
    
    return ret;
}

/* write some binary data into specified trace with an variable length field in front */
uint32_t trace_write_binary(uint32_t context, uint8_t *buffer, uint32_t length)
{
    tsc_t tsc = get_us_clock();
    trace_entry_t *ctx = &trace_contexts[context];

    /* make sure ctx is valid, this ctx is used and the writer thread is running */
    if(context >= TRACE_MAX_CONTEXT || !ctx->used || ctx->task_state != TRACE_TASK_STATE_RUNNING)
    {
        return TRACE_ERROR;
    }

    /* var length is max 4 bytes, tsc 8 bytes. check for enough free space */
    if((ctx->buffer_write_pos + length + 4 + sizeof(tsc)) >= ctx->buffer_size)
    {
        /* abort trace as data will be lost */
        ctx->task_state = TRACE_TASK_STATE_SHUTDOWN;
        return TRACE_ERROR;
    }

    /* the main task is dealing with our pointers, so we have to lock this section */
    uint32_t old_stat = cli();

    /* first copy TSC */
    memcpy(&ctx->buffer[ctx->buffer_write_pos], &tsc, sizeof(tsc));
    ctx->buffer_write_pos += 4;

    /* next is variable length */
    trace_write_varlength(context, length);

    /* finally put data */
    memcpy(&ctx->buffer[ctx->buffer_write_pos], buffer, length);
    ctx->buffer_write_pos += length;

    sei(old_stat);

    return TRACE_ERROR;
}

/* internal */
static uint32_t trace_write_varlength(uint32_t context, uint32_t length)
{
    trace_entry_t *ctx = &trace_contexts[context];

    uint32_t var_length = 0;
    if(length >= 0x10)
    {
        var_length = 1;
    }
    else if(length >= 0x1000)
    {
        var_length = 2;
    }
    else if(length >= 0x100000)
    {
        var_length = 3;
    }

    uint8_t data[4];

    /* build variable length buffer which is copied only partially */
    data[0] = (var_length << 4) | (length & 0x0F);
    data[1] = (length >> 4) & 0xFF;
    data[2] = (length >> 12) & 0xFF;
    data[3] = (length >> 20) & 0xFF;

    /* skip the zero bytes */
    memcpy(&ctx->buffer[ctx->buffer_write_pos], data, var_length + 1);
    ctx->buffer_write_pos += 4;

    return var_length + 1;
}

uint32_t trace_available()
{
    return 1;
}

static unsigned int trace_init()
{
    /* don't initialize anything as other modules may use this module during their init, these could be ran before ours */
    
    return 0;
}

static unsigned int trace_deinit()
{
    for(uint32_t pos = 0; pos < TRACE_MAX_CONTEXT; pos++)
    {
        if(trace_contexts[pos].used)
        {
            trace_stop(pos, 1);
        }
    }
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(trace_init)
    MODULE_DEINIT(trace_deinit)
MODULE_INFO_END()

