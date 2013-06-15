
#define __trace_c__

#include <module.h>
#include <dryos.h>

#include "trace.h"

static trace_entry_t trace_contexts[TRACE_MAX_CONTEXT];

/* general selection of allocation method */
void *trace_alloc(int size)
{
    return alloc_dma_memory(size);
}

void trace_free(void *data)
{
    free_dma_memory(data);
}

static void trace_task(trace_entry_t *ctx)
{
    ctx->task_state = TRACE_TASK_STATE_RUNNING;

    while((ctx->task_state == TRACE_TASK_STATE_RUNNING) || (ctx->buffer_read_pos < ctx->buffer_write_pos))
    {
        if(ctx->buffer_read_pos < ctx->buffer_write_pos)
        {
            /* write as much as possible at once */
            int written = FIO_WriteFile(ctx->file_handle, &ctx->buffer[ctx->buffer_read_pos], ctx->buffer_write_pos - ctx->buffer_read_pos);

            /* check for writing errors */
            if(written > 0)
            {
                /* locked section to update pointers */
                ctx->buffer_read_pos += written;

                int old_stat = cli();
                if(ctx->buffer_read_pos == ctx->buffer_write_pos)
                {
                    ctx->buffer_read_pos = 0;
                    ctx->buffer_write_pos = 0;
                }
                sei(old_stat);
            }
            else
            {
                FIO_CloseFile(ctx->file_handle);
                trace_free(ctx->buffer);
                ctx->task_state = TRACE_TASK_STATE_DEAD;
                return;
            }
        }
        else
        {
            msleep(ctx->sleep_time);
        }
    }

    /* we are doing the cleanup in this task */
    FIO_CloseFile(ctx->file_handle);
    trace_free(ctx->buffer);
    ctx->file_handle = NULL;
    ctx->buffer = NULL;
    ctx->task_state = TRACE_TASK_STATE_DEAD;
    ctx->used = 0;
}

unsigned int trace_start(char *name, char *file_name)
{
    int pos = 0;
    for(pos = 0; pos < TRACE_MAX_CONTEXT; pos++)
    {
        if(!trace_contexts[pos].used)
        {
            trace_entry_t *ctx = &trace_contexts[pos];

            /* init fields */
            ctx->used = 1;
            ctx->format = TRACE_FMT_DEFAULT;
            ctx->separator = TRACE_SEPARATOR_DEFAULT;
            ctx->sleep_time = TRACE_SLEEP_TIME;
            ctx->buffer_size = TRACE_BUFFER_SIZE;
            ctx->buffer_read_pos = 0;
            ctx->buffer_write_pos = 0;

            /* copy strings */
            strncpy(ctx->name, name, sizeof(ctx->name));
            strncpy(ctx->file_name, file_name, sizeof(ctx->file_name));

            /* start worker task */
            ctx->task_state = TRACE_TASK_STATE_DEAD;
            ctx->task = (unsigned int)task_create("trace_task", 0x1e, 0x1000, &trace_task, (void *)ctx);

            /* create trace file */
            FIO_RemoveFile(ctx->file_name);
            ctx->file_handle = FIO_CreateFileEx(ctx->file_name);
            if(ctx->file_handle == INVALID_PTR)
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

            ctx->start_tsc = get_ms_clock_value();
            ctx->last_tsc = ctx->start_tsc;

            /* make sure task is running */
            for(int loops = 0; loops < 250; loops++)
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
    }

    return TRACE_ERROR;
}

unsigned int trace_stop(unsigned int context, int wait)
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
    for(int loops = 0; loops < 500; loops++)
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
unsigned int trace_format(unsigned int context, unsigned int format, unsigned char separator)
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


/* write some string into specified trace */
unsigned int trace_write(unsigned int context, char *string, ...)
{
    unsigned int tsc = get_ms_clock_value();
    char timestamp[64];
    int timestamp_pos = 0;
	va_list ap;
    trace_entry_t *ctx = &trace_contexts[context];

    if(context >= TRACE_MAX_CONTEXT || !ctx->used)
    {
        return TRACE_OK;
    }

    /* build timestamp string */
    timestamp[0] = 0;
    if(ctx->format & TRACE_FMT_TIME_CTR)
    {
        /* write raw timestamp counter "000015255" */
        char tmp[16];

        snprintf(tmp, sizeof(tmp), "%09d", tsc);
        strncpy(&timestamp[timestamp_pos], tmp, sizeof(timestamp) - timestamp_pos);
        timestamp_pos += strlen(tmp);

        /* add separator */
        timestamp[timestamp_pos] = ctx->separator;
        timestamp_pos++;
    }
    if(ctx->format & TRACE_FMT_TIME_CTR_REL)
    {
        /* write raw timestamp counter delta to starting time "000015255" */
        char tmp[16];

        unsigned int delta = tsc - ctx->start_tsc;
        snprintf(tmp, sizeof(tmp), "%09d", delta);
        strncpy(&timestamp[timestamp_pos], tmp, sizeof(timestamp) - timestamp_pos);
        timestamp_pos += strlen(tmp);

        /* add separator */
        timestamp[timestamp_pos] = ctx->separator;
        timestamp_pos++;
    }
    if(ctx->format & TRACE_FMT_TIME_REL)
    {
        /* write time since measurement start "0:0:36.309" */
        char tmp[16];
        unsigned int delta = tsc - ctx->start_tsc;
        unsigned int msec = delta % 1000;
        unsigned int sec_total = delta / 1000;
        unsigned int sec = sec_total % 60;
        unsigned int min = (sec_total / 60) % 60;
        unsigned int hrs = (sec_total / 3600) % 60;

        snprintf(tmp, sizeof(tmp), "%02d:%02d:%02d.%03d", hrs, min, sec, msec);
        strncpy(&timestamp[timestamp_pos], tmp, sizeof(timestamp) - timestamp_pos);
        timestamp_pos += strlen(tmp);

        /* add separator */
        timestamp[timestamp_pos] = ctx->separator;
        timestamp_pos++;
    }

    /* get the minimum length that is needed. string may get longer, but we cannot predict that */
    int min_len = strlen(string) + timestamp_pos + 1;

    /* yes, we are disabling interrupts here. this is quite bad - should we use a per-context vsprintf-buffer? */
    int old_stat = cli();

    /* check if the buffer has enough space */
    if((ctx->buffer_write_pos + min_len) >= ctx->buffer_size)
    {
        sei(old_stat);
        return TRACE_ERROR;
    }

    int max_len = ctx->buffer_size - ctx->buffer_write_pos;
    va_start( ap, string );

    strncpy(&ctx->buffer[ctx->buffer_write_pos], timestamp, max_len);
    int len = vsnprintf(&ctx->buffer[ctx->buffer_write_pos + timestamp_pos], max_len - timestamp_pos, string, ap);

    /* buffer filled until the end. seems the string was too long */
    if(len >= max_len - timestamp_pos)
    {
        sei(old_stat);
        return TRACE_ERROR;
    }

    /* attach a newline */
    ctx->buffer[ctx->buffer_write_pos + timestamp_pos + len] = '\n';

    /* successful, commit the buffer content */
    ctx->buffer_write_pos += (len + timestamp_pos + 1);

    sei(old_stat);

    return TRACE_OK;
}

/* write some binary data into specified trace with an variable length field in front */
unsigned int trace_write_binary(unsigned int context, unsigned char *buffer, unsigned int length)
{
    unsigned int tsc = get_ms_clock_value();
    trace_entry_t *ctx = &trace_contexts[context];

    if(context >= TRACE_MAX_CONTEXT || !ctx->used)
    {
        return TRACE_ERROR;
    }

    /* var length is max 4 bytes, tsc also. check for enough free space */
    if((ctx->buffer_write_pos + length + 4 + 4) >= ctx->buffer_size)
    {
        return TRACE_ERROR;
    }

    /* the main task is dealing with our pointers, so we have to lock this section */
    int old_stat = cli();

    /* first copy TSC */
    memcpy(&ctx->buffer[ctx->buffer_write_pos], &tsc, 4);
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
static unsigned int trace_write_varlength(unsigned int context, unsigned int length)
{
    trace_entry_t *ctx = &trace_contexts[context];

    int var_length = 0;
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

    unsigned char data[4];

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


static unsigned int trace_init()
{
    /* don't initialize anything as other modules may use this module during their init, these could be ran before ours */
    return 0;
}

static unsigned int trace_deinit()
{
    for(int pos = 0; pos < TRACE_MAX_CONTEXT; pos++)
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

MODULE_STRINGS_START()
    MODULE_STRING("Description", "Trace library")
    MODULE_STRING("License", "GPLv2")
    MODULE_STRING("Author", "g3gg0")
MODULE_STRINGS_END()

