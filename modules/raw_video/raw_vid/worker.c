#include "module.h"
#include "dryos.h"

#ifdef RAW_VID_LOG
#include "log.h"
#endif

#include "raw.h"
#include "edmac-memcpy.h"

#include "raw_vid.h"
#include "raw_vid_worker.h"
#include "raw_vid_event_pusher.h"

static struct msg_queue *event_q = NULL;
static FILE *raw_vid_fp = NULL;

// ensures we never attempt an edmac copy while the last is in progress
static struct semaphore *edmac_sem = NULL;

static enum recording_state recording_state = INACTIVE;

enum recording_state get_recording_state(void)
{
    return recording_state;
}

// lv_save_raw trick (which we trigger indirectly via raw.c, raw_lv_request(),
// has given us a buffer of whatever size DryOS wants for fullsize LV.  We can
// crop this down with copy_rectangle_non_blocking(), DMA based and fast enough
// to do each frame.
struct crop_buf 
{
    char *_buf; // Unaligned, underlying buffer, used with malloc/free.
    char *start; // Start of aligned buf, for DMA ops, etc.
    char *cur; // Position that data should write to.
    char *end; // Do not write to this location or later.
};

// Finds two large, aligned blocks, of equal size,
// for accumulating potentially cropped frame data in,
// before writing to disk.
static void alloc_crop_bufs(struct crop_buf crop_bufs[])
{
    if (crop_bufs[0]._buf != NULL
        || crop_bufs[1]._buf != NULL)
    {
        #ifdef RAW_VID_LOG
        send_log_data_str("ERROR: non-null _buf in alloc_crop_bufs()\n");
        #endif
        return;
    }
    
    uint32_t buf_size = INITIAL_CROP_BUF_SIZE + CROP_BUF_ALIGN;
    char *buf0 = NULL;
    char *buf1 = NULL;
    while (buf_size > MIN_CROP_BUF_SIZE)
    {
        buf0 = fio_malloc(buf_size);
        if (buf0 != NULL)
        {
            buf1 = fio_malloc(buf_size);
            if (buf1 != NULL)
            {
                break;
            }
            else
            {
                fio_free(buf0);
                buf0 = NULL;
            }
        }

        buf_size -= CROP_BUF_STEP_SIZE;
    }

    if (buf_size < MIN_CROP_BUF_SIZE)
    {
        #ifdef RAW_VID_LOG
        send_log_data_str("Couldn't find good crop bufs\n");
        #endif
        // These should both already be null, but it's a big memory leak
        // if we get it wrong!
        if (buf0 != NULL)
        {
            #ifdef RAW_VID_LOG
            send_log_data_str("ERROR: buf0 unexpectedly NULL\n");
            #endif
            fio_free(buf0);
            buf0 = NULL;
        }
        if (buf1 != NULL)
        {
            #ifdef RAW_VID_LOG
            send_log_data_str("ERROR: buf1 unexpectedly NULL\n");
            #endif
            fio_free(buf1);
            buf1 = NULL;
        }
        return;
    }

    // Here we have two large enough bufs.

    crop_bufs[0]._buf = buf0;
    crop_bufs[1]._buf = buf1;

    if ((uint32_t)buf0 % CROP_BUF_ALIGN != 0)
    {
        crop_bufs[0].start = buf0 + (CROP_BUF_ALIGN - (uint32_t)buf0 % CROP_BUF_ALIGN);
    }
    else
    {
        crop_bufs[0].start = buf0;
    }
    if ((uint32_t)buf1 % CROP_BUF_ALIGN != 0)
    {
        crop_bufs[1].start = buf1 + (CROP_BUF_ALIGN - (uint32_t)buf1 % CROP_BUF_ALIGN);
    }
    else
    {
        crop_bufs[1].start = buf1;
    }
    // Do we want to include the unaligned end portion in the above sizes?
    // I'm assuming not, it's a small amount, and would rarely contain data
    // before we write out.

    // We use these for DMA ops, which prefer / require
    // the uncacheable address.
    crop_bufs[0].start = UNCACHEABLE(crop_bufs[0].start);
    crop_bufs[1].start = UNCACHEABLE(crop_bufs[1].start);

    crop_bufs[0].cur = crop_bufs[0].start;
    crop_bufs[0].end = crop_bufs[0].start + buf_size;
    crop_bufs[1].cur = crop_bufs[1].start;
    crop_bufs[1].end = crop_bufs[1].start + buf_size;
}

// Old DryOS used to trigger this, new DryOS
// seems to only trigger the write complete cbr.
static void edmac_cbr_r(void *unused_ctx)
{
}

// Triggers when EDMAC write operation is complete.
static void edmac_cbr_w(void *unused_ctx)
{
    copy_rectangle_non_blocking_cleanup();
    give_semaphore(edmac_sem); // signal edmac copy rect as safe to use
}

static void write_buf_to_disk(struct crop_buf *crop_buf)
{
    #ifdef RAW_VID_LOG
    uint32_t buf_size = 100;
    char log_buf[buf_size];
    #endif

    if (crop_buf == NULL)
    {
        #ifdef RAW_VID_LOG
        send_log_data_str("ERROR: buf struct for disk write was NULL\n");
        #endif
        return;
    }

    if (crop_buf->_buf == NULL
        || crop_buf->start == NULL
        || crop_buf->cur == NULL
        || crop_buf->end == NULL)
    {
        #ifdef RAW_VID_LOG
        send_log_data_str("ERROR: some buf member for disk write was NULL\n");
        #endif
        return;
    }
    
    #ifdef RAW_VID_LOG
    snprintf(log_buf, buf_size, "About to write out, addr, size: 0x%x, %d\n",
             crop_buf->start, crop_buf->cur - crop_buf->start);
    send_log_data_str(log_buf);
    #endif

    // FIXME we probably want a sem to remove the potential
    // for COMMAND_STOP to close this handle before we start
    // the write.
    if (raw_vid_fp == NULL)
    {
        #ifdef RAW_VID_LOG
        send_log_data_str("ERROR: raw_vid_fp was NULL before write\n");
        #endif
        return;
    }

    FIO_WriteFile(raw_vid_fp, crop_buf->start, crop_buf->cur - crop_buf->start);

    // reset buf so it's good to use again
    crop_buf->cur = crop_buf->start;
}

// This is the heart of worker.c.
//
// This runs as a task whenever the module is loaded,
// regardless of whether a recording is in progress.
//
// It is the only consumer of the message queue. 
static void worker(void)
{
    #ifdef RAW_VID_LOG
    uint32_t buf_size = 100;
    char log_buf[buf_size];
    uint32_t start_time = 0;
    uint32_t session_data_size = 0;
    #endif

    char mlv_name[20] = {0};

    struct crop_buf crop_bufs[2] = {NULL};
    struct crop_buf *crop_buf = NULL;

    TASK_LOOP
    {
        struct raw_vid_event *event = NULL;
        msg_queue_receive(event_q, &event, 0);

        if (event == NULL)
        { // shouldn't happen, we only post valid events
            #ifdef RAW_VID_LOG
            send_log_data_str("ERROR: worker, event was NULL\n");
            #endif
            continue;
        }

        switch (event->type)
        {
        case IMAGE_DATA:
            if (recording_state != ACTIVE)
            {
                #ifdef RAW_VID_LOG
                send_log_data_str("ERROR: trying to process frame while recording not ACTIVE\n");
                #endif
                break;
            }

            if (event->src == NULL)
            {
                #ifdef RAW_VID_LOG
                send_log_data_str("ERROR: event->src was NULL\n");
                #endif
                break;
            }

            if (raw_vid_fp == NULL)
            {
                #ifdef RAW_VID_LOG
                send_log_data_str("ERROR: fp was NULL at attempt to write\n");
                #endif
                break;
            }

            #ifdef RAW_VID_LOG
            snprintf(log_buf, buf_size, "Image prod time, cons time, size: %d, %d, %d\n",
                     event->creation_time, get_ms_clock(), event->size);
            send_log_data_str(log_buf);
            #endif

            if (event->size > crop_buf->end - crop_buf->start)
            {
                #ifdef RAW_VID_LOG
                send_log_data_str("ERROR: event size too large for crop buf\n");
                #endif
                break;
            }

            // Here we have src data, and an output file.
            // We want to do a (potentially cropped) copy from
            // src to dst.  We accumulate frames into a larger buffer
            // before triggering flush to disk.

            // Find valid dst
            char *dst = NULL;
            if (crop_buf == NULL)
            {
                // FIXME we should stop here
                #ifdef RAW_VID_LOG
                send_log_data_str("ERROR: crop_buf was NULL at dst check\n");
                #endif
                break;
            }

            struct crop_buf *flush_src = NULL;
            if (event->size > crop_buf->end - crop_buf->cur)
            { // Then the prior IMAGE_DATA was the last that fit in this buffer,
              // we must swap.  And we can trigger flush to disk of prior in use buffer.
                #ifdef RAW_VID_LOG
                send_log_data_str("Swapping bufs\n");
                #endif
                
                flush_src = crop_buf; // this triggers later flush to disk

                if (crop_buf == &crop_bufs[0])
                    crop_buf = &crop_bufs[1];
                else
                    crop_buf = &crop_bufs[0];

                if (crop_buf->cur != crop_buf->start)
                {
                    // raw_write() can reset these, presumably we're writing too fast
                    // so it's not finished.
                    #ifdef RAW_VID_LOG
                    send_log_data_str("After buf swap, buf was in use; not written out\n");
                    #endif
                    crop_buf = NULL; // Hack, prevents next IMAGE_DATA doing anything,
                                     // really we should flush the remaining?
                    break; // FIXME we should stop as well
                }
            }
            dst = crop_buf->cur;
            crop_buf->cur += event->size;

            int err = take_semaphore_now(edmac_sem);
            if (err)
            {
                // If this happens, it means we're not managing to save a frame
                // before the next vsync event in event_pusher.c tries to capture one.

                #ifdef RAW_VID_LOG
                send_log_data_str("ERROR: worker couldn't take edmac_sem\n");
                #endif
                // FIXME this break means we're choosing to drop frames
                // rather than stop recording, I think just stopping is better.
                // Maybe re-imp the option from mlv_lite.
                break;
            }

            // here we know the copy rect from the prior IMAGE_DATA has completed,
            // as we have edmac_sem, so we can safely flush if one of the buffers was full.
            #ifdef RAW_VID_LOG
            session_data_size += event->size;
            #endif
            if (flush_src != NULL)
            {
                task_create("raw_write", WORKER_PRIO - 1, 0x800,
                            write_buf_to_disk, flush_src);
            }

            int line_size = event->w * BPP/8; // width in bytes
            #ifdef RAW_VID_LOG
            snprintf(log_buf, buf_size, "w, h, sz: %d, %d, %d\n",
                     event->w, event->h, line_size * event->h);
            send_log_data_str(log_buf);
            #endif
            copy_rectangle_non_blocking(
                (void*)dst, line_size, 0, 0,
                event->src, raw_info.pitch, (event->x + 7)/8*BPP, event->y/2*2,
                line_size, event->h,
                &edmac_cbr_r, &edmac_cbr_w, NULL
            );
            // edmac_cbr_w() will release edmac sem, when copy complete

            break;

        case COMMAND_START:
            #ifdef RAW_VID_LOG
            send_log_data_str("Worker got start request.\n");
            #endif
            if (recording_state == INACTIVE)
            {
                // Get mem to buffer our cropped frames.  We do this because 
                // card write speeds are improved if we pass only large, size aligned
                // buffers to FIO_WriteFile().  Optimal size and alignment probably
                // varies by cam, card (manufacturer, controller, etc), card format etc.
                //
                // We double buffer so we can record into one as we write out the other.
                // At this point, we don't know the required size as no image data has
                // been sent.
                if (crop_buf != NULL)
                {
                    #ifdef RAW_VID_LOG
                    send_log_data_str("ERROR: crop_buf not null on COMMAND_START\n");
                    #endif
                    break;
                }
                alloc_crop_bufs(crop_bufs);
                if (crop_bufs[0].start == NULL
                    || crop_bufs[1].start == NULL)
                {
                    #ifdef RAW_VID_LOG
                    send_log_data_str("ERROR: could not obtain crop bufs, cannot start\n");
                    #endif
                    break;
                }
                #ifdef RAW_VID_LOG
                snprintf(log_buf, buf_size, "buf[0] and buf[1] size: %d, %d\n",
                         crop_bufs[0].end - crop_bufs[0].cur,
                         crop_bufs[1].end - crop_bufs[1].cur);
                send_log_data_str(log_buf);
                #endif
                crop_buf = &crop_bufs[0];

                // Set LV to raw
                raw_lv_request();

                // Get new save file
                if (raw_vid_fp != NULL)
                {
                    #ifdef RAW_VID_LOG
                    send_log_data_str("ERROR: Worker tried to init save file but already in use\n");
                    #endif
                    break;
                }

                if (get_numbered_file_name("tst%02d.mlv", 99, mlv_name, sizeof(mlv_name)) > -1)
                {
                    raw_vid_fp = FIO_CreateFile(mlv_name);
                    if (raw_vid_fp == NULL)
                    {
                        #ifdef RAW_VID_LOG
                        send_log_data_str("Worker couldn't create mlv file\n");
                        #endif
                    }
                }
                else
                {
                    #ifdef RAW_VID_LOG
                    send_log_data_str("ERROR: Worker couldn't get mlv file name\n");
                    #endif
                    break;
                }

                #ifdef RAW_VID_LOG
                send_log_data_str("Recording is active\n");
                start_time = get_ms_clock();
                #endif
                recording_state = ACTIVE;
            }
            break;

        case COMMAND_STOP:
            #ifdef RAW_VID_LOG
            send_log_data_str("Worker got stop request.\n");
            #endif
            // FIXME how to stop is more complicated than this,
            // need to flush, reset some pointers, block new start
            // until finished.
            if (recording_state == ACTIVE)
            {
                // restore LV away from raw
                // This ML asserts if you haven't paired it with raw_lv_request(),
                // but is safe.
                raw_lv_release();
                // TODO mlv_lite does restore_bit_depth(), we
                // don't mess with this yet.

                if (raw_vid_fp != NULL)
                {
                    FIO_CloseFile(raw_vid_fp);
                    raw_vid_fp = NULL;
                }
                else
                {
                    #ifdef RAW_VID_LOG
                    send_log_data_str("ERROR: unexpected fp state in worker\n");
                    #endif
                }

                #ifdef RAW_VID_LOG
                snprintf(log_buf, buf_size, "session time, data written: %d, %d\n",
                         get_ms_clock() - start_time, session_data_size);
                send_log_data_str(log_buf);
                session_data_size = 0;
                #endif

                // FIXME we want to handle any potential flushing before freeing stuff,
                // we may need buffers for dealing with queued frames.
                crop_buf = NULL;
                if (crop_bufs[0]._buf != NULL)
                {
                    fio_free(crop_bufs[0]._buf);
                    crop_bufs[0]._buf = NULL;
                    crop_bufs[0].start = NULL;
                    crop_bufs[0].cur = NULL;
                    crop_bufs[0].end = NULL;
                }
                if (crop_bufs[1]._buf != NULL)
                {
                    fio_free(crop_bufs[1]._buf);
                    crop_bufs[1]._buf = NULL;
                    crop_bufs[1].start = NULL;
                    crop_bufs[1].cur = NULL;
                    crop_bufs[1].end = NULL;
                }

                recording_state = INACTIVE;
                #ifdef RAW_VID_LOG
                send_log_data_str("Recording is inactive\n");
                #endif
            }
            break;

        case EMPTY:
        case PREPARING:
            // These types should never be sent.  We will need to fix the cause.
            // But we still want to release the event storage.
            #ifdef RAW_VID_LOG
            send_log_data_str("ERROR: unexpected event type\n");
            #endif
            break;
        }

        // Note that we must reach here from every case above,
        // or we will leak events and starve the queue.
        release_event(event);

        // no sleep needed, queue receive with timeout 0 is blocking.
    }
    #ifdef RAW_VID_LOG
    send_log_data_str("Raw video worker stopping.\n");
    #endif
}

void init_worker(struct msg_queue *q)
{
    event_q = q;
    edmac_sem = create_named_semaphore("edmac_raw_worker", SEM_CREATE_UNLOCKED);
    if (edmac_sem == NULL)
    {
        #ifdef RAW_VID_LOG
        send_log_data_str("ERROR: worker could not create edmac sem\n");
        #endif
        return;
    }
    task_create("raw_v_worker", WORKER_PRIO, 0x800, worker, NULL);
    allow_recording();
}
