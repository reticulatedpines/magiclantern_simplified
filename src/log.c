// Generic logger, using in-memory ring buffer, and periodic flushing to disk.
// User configures it, and can then log "infinite" amounts at high data rates.

#include "dryos.h"
#include "fio-ml.h"
#include "string.h"
#include "log.h"

#ifdef FEATURE_DISK_LOG

// This sem guards usage of the buf_written and buf_next pointers
static struct semaphore *log_mem_sem = NULL;
// This sem controls disk writing
struct semaphore *log_disk_sem = NULL;


static uint8_t *buf_start = NULL; // Start of the logging buffer.
                                  // Never changed once initialised.
static uint8_t *buf_end = NULL; // End of the logging buffer.
                                // Never changed once initialised.
static uint8_t *buf_written = NULL; // Some bytes to the left of this have been written to disk.
                                    // Bytes from this point and to the right have not.
static uint8_t *buf_next = NULL; // Where the next bytes to log will be written to buf.
static uint32_t buf_size = 0;

static FILE *log_fp = NULL;

static uint32_t is_log_enabled = 1;

void enable_logging(void)
{
    is_log_enabled = 1;
}

void disable_logging(void)
{
    is_log_enabled = 0;
}

// periodically writes buffer to disk
static void disk_write_task(void *unused)
{
    while(!ml_shutdown_requested)
    {
        take_semaphore(log_disk_sem, 0);
        // something woke us up, write out data

        // It's okay if buf_next moves while we're writing out,
        // since it will never advance past buf_written.
        // But we do want to make a copy to ensure we use a consistent
        // value here.
        uint8_t *next = buf_next;

        if (next == buf_written)
            continue;

        if (next > buf_written)
        {
            FIO_WriteFile(log_fp, buf_written, (next - buf_written));
        }
        else
        {
            FIO_WriteFile(log_fp, buf_written, (buf_end - buf_written));
            FIO_WriteFile(log_fp, buf_start, (next - buf_start));
        }

        take_semaphore(log_mem_sem, 0);
        buf_written = next;
        give_semaphore(log_mem_sem);
    }
    // ML is shutting down, close file.  There should be no more
    // data to write out, it's just happened above.
    FIO_CloseFile(log_fp);
}

// caller is responsible for providing buffer
// (allows use on different stages of ports, and different run time contexts)
int init_log(uint8_t *buf, uint32_t size, char *filename)
{
    if (buf != NULL
        && log_mem_sem == NULL
        && log_disk_sem == NULL
        && filename != NULL
        && size >= MIN_LOG_BUF_SIZE
        && buf_start == NULL)
    {
        buf_start = buf;
        buf_written = buf;
        buf_next = buf;
        buf_end = buf + size;
        buf_size = size;

        log_mem_sem = create_named_semaphore("log_mem_sem", SEM_CREATE_UNLOCKED);
        log_disk_sem = create_named_semaphore("log_disk_sem", SEM_CREATE_LOCKED);

        log_fp = FIO_CreateFile(filename);

        task_create("log_write", 0x1d, 0x600, disk_write_task, 0);
        return 0;
    }

    return -1;
}

// Send some data to be written to disk.
// Writes to disk happen periodically.
//
// Ordering of log data is not guaranteed to be the same as order of calls
// to send_log_data().  In practice it should be reliable unless you're really
// spamming it.  In which case, serialise it on your side.
//
// send_log_data() is blocking, and when it returns without error,
// the data has been written to memory buf.
//
// The requested data will either all be copied to log buf, or none.
// Any negative return value signifies no data was written.
int send_log_data(uint8_t *data, uint32_t size)
{
    if (is_log_enabled == 0)
        return 0;

    if (size > buf_size)
    {
        // Should we handle this case?  We could flush existing buf,
        // and write direct from supplied data to disk.
        return -1;
    }

    if (log_mem_sem == NULL)
        return -4; // probably didn't call init_log() successfully

    // We use take_semaphore_now() to allow logging in an interrupt handler.
    // This is common in EDMAC code, for example.
    int err = take_semaphore_now(log_mem_sem);
    if (err)
    { // Couldn't take sem (and maybe DryOS asserted).
        return -2;
    }

    // We have semaphore, and can manipulate "next" and "written" pointers.
    //
    // Note that we never allow the buffer to be completely full,
    // since that would place buf_next and buf_written at the same location
    // while filling, and disk_write_task() will do that while emptying.
    // Disallowing it on fill means we don't have to deal with ambiguity.
    int32_t ret = -3;

    uint32_t available_space = 0;
    if (buf_next >= buf_written)
    { // next mem writes occur between buf_next and buf_end, and can wrap
        available_space = (buf_end - buf_next)
                          + (buf_written - buf_start);
        if (available_space <= size)
        {
            goto cleanup;
        }

        uint32_t unwrap_space = buf_end - buf_next;
        if (unwrap_space > size)
        { // buf_next remains in valid space, before buf_end
            memcpy(buf_next, data, size);
            buf_next += size;
            ret = 0;
        }
        else
        { // buf_next must wrap
            memcpy(buf_next, data, unwrap_space);
            uint8_t *remaining_data = data + unwrap_space;
            uint32_t remaining_size = size - unwrap_space;
            if (remaining_size > 0) // can be exactly 0
            {
                memcpy(buf_start, remaining_data, remaining_size);
                buf_next = buf_start + remaining_size;
            }
            else
            {
                buf_next = buf_end;
            }
            ret = 0;
        }
    }
    else
    { // buf_next < buf_written, mem writes occur between
      // next and written
        available_space = buf_written - buf_next;
        if (available_space <= size)
        {
            goto cleanup;
        }

        memcpy(buf_next, data, size);
        buf_next += size;
        ret = 0;
    }

    // enable disk writes if sufficient queued
    uint32_t queued_write_size = 0;
    if (buf_next > buf_written)
    {
        queued_write_size = buf_next - buf_written;
    }
    else
    {
        queued_write_size = (buf_end - buf_written)
                            + (buf_next - buf_start);
    }

    if (queued_write_size >= MIN_LOG_WRITE_SIZE)
    {
        // wake up the write task
        give_semaphore(log_disk_sem);
    }

cleanup:
    give_semaphore(log_mem_sem);
    return ret;
}

// Must be used with null terminated strings only
int send_log_data_str(char *s)
{
    return send_log_data((uint8_t *)s, strlen(s));
}

#endif // FEATURE_DISK_LOG
