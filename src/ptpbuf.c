
#include "dryos.h"
#include "config.h"
#include "version.h"
#include "consts.h"
#include "ptpbuf.h"


ptpbuf_buffer_t ptpbuf_buffers[PTPBUF_BUFS];
uint32_t ptpbuf_fetchable[PTPBUF_BUFS];
ptpbuf_t ptpbuf_data = { PTPBUF_MAGIC, 0, PTPBUF_BUFS, PTPBUF_BUFSIZE, 0, 0, ptpbuf_buffers, ptpbuf_fetchable};

uint32_t get_free_buffer()
{
    ptpbuf_t *ptpbuf = UNCACHEABLE(&ptpbuf_data);
    uint32_t *fetchable = UNCACHEABLE(ptpbuf->fetchable);
    
    /* go through all buffers */
    for(uint32_t buf = 0; buf < ptpbuf->buffer_count; buf++)
    {
        /* check if buffer is free for us */
        if(fetchable[buf] == 0)
        {
            ptpbuf_buffer_t *buffer = UNCACHEABLE(&(ptpbuf->buffers[buf]));
            buffer->bytes_used = 0;
            return buf;
        }
    }
    
    return 0xFFFFFFFF;
}

uint32_t ptpbuf_add(uint32_t type, uint8_t *data, uint32_t length)
{
    ptpbuf_t *ptpbuf = UNCACHEABLE(&ptpbuf_data);
    uint32_t *fetchable = UNCACHEABLE(ptpbuf->fetchable);
    ptpbuf_buffer_t *buffer = UNCACHEABLE(&(ptpbuf->buffers[ptpbuf->current_buffer]));
    ptpbuf_packet_t packet;
    
    /* check for enough space in current buffer */
    if(buffer->bytes_used + sizeof(ptpbuf_packet_t) + length >= ptpbuf->buffer_size)
    {
        /* buffer is full, get next */
        fetchable[ptpbuf->current_buffer] = 1;
        uint32_t next_buffer = get_free_buffer();
        
        if(next_buffer >= ptpbuf->buffer_count)
        {
            /* no free buffers. fail. */
            ptpbuf->overflow = 1;
            return 0;
        }
        
        ptpbuf->overflow = 0;        
        ptpbuf->current_buffer = next_buffer;
        buffer = UNCACHEABLE(&(ptpbuf->buffers[ptpbuf->current_buffer]));
        buffer->bytes_used = 0;
    }
    
    packet.type = type;
    packet.length = length;
    
    /* store packet header */
    memcpy(&(buffer->data[buffer->bytes_used]), &packet, sizeof(ptpbuf_packet_t));
    buffer->bytes_used += sizeof(ptpbuf_packet_t);
    
    /* store data */
    memcpy(&(buffer->data[buffer->bytes_used]), data, length);
    buffer->bytes_used += length;

    
    /* commit this buffer after populating with data? flag gets set by ptpbuf client periodically */
    if(ptpbuf->commit)
    {
        ptpbuf->commit = 0;
        
        /* get next buffer */
        uint32_t next_buffer = get_free_buffer();
        
        if(next_buffer >= ptpbuf->buffer_count)
        {
            /* no free buffers. could not commit. */
            return 1;
        }
        
        /* mark buffer for being retrieved */
        fetchable[ptpbuf->current_buffer] = 1;
        
        ptpbuf->overflow = 0;        
        ptpbuf->current_buffer = next_buffer;
        buffer = UNCACHEABLE(&(ptpbuf->buffers[ptpbuf->current_buffer]));
        buffer->bytes_used = 0;
    }
    
    return 1;
}
