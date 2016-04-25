
#include "dryos.h"
#include "edmac.h"

#define WRITE(x) (x)
#define READ(x)  (0x80000000 | (x))

#define IS_USED(ch) ((ch) < NUM_EDMAC_CHANNELS && edmac_chanlist[ch] != 0xFFFFFFFF)
#define IS_WRITE(ch) (((ch) & 8) == 0)
#define IS_READ(ch)  (((ch) & 8) != 0)

/* channel usage for 5D3 */
/* from LockEngineResources:
 * write_edmac_resources:
 *   0, 1, 2, 3, 4, 5, 6, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x20, 0x21
 * read_edmac_resources:
 *   8, 9, 0xA, 0xB, 0xC, 0xD, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x28, 0x29, 0x2A, 0x2B
 */
static uint32_t edmac_chanlist[] = 
{
    WRITE(0),  WRITE(1),  WRITE(2),   WRITE(3),   WRITE(4),   WRITE(5),   WRITE(6),   0xFFFFFFFF,
    READ(0),   READ(1),   READ(2),    READ(3),    READ(4),    READ(5),    0xFFFFFFFF, 0xFFFFFFFF,
    WRITE(7),  WRITE(8),  WRITE(9),   WRITE(10),  WRITE(11),  WRITE(12),  WRITE(13),  0xFFFFFFFF,
    READ(6),   READ(7),   READ(8),    READ(9),    READ(10),   READ(11),   0xFFFFFFFF, 0xFFFFFFFF,
    WRITE(14), WRITE(15), 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    READ(12),  READ(13),  READ(14),   READ(15),   0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
};

#ifdef CONFIG_DIGIC_V
#define NUM_EDMAC_CHANNELS 48
#else
#define NUM_EDMAC_CHANNELS 32
#endif

/* http://www.magiclantern.fm/forum/index.php?topic=6740 */
static uint32_t write_edmacs[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x20, 0x21};
static uint32_t read_edmacs[]  = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x28, 0x29, 0x2A, 0x2B};

uint32_t edmac_channel_to_index(uint32_t channel)
{
    if (!IS_USED(channel))
    {
        return -1;
    }
    
    uint32_t direction = edmac_get_dir(channel);
    
    switch (direction)
    {
        case EDMAC_DIR_READ:
        {
            for (int i = 0; i < COUNT(read_edmacs); i++)
                if (read_edmacs[i] == channel)
                    return i;
        }
        case EDMAC_DIR_WRITE:
        {
            for (int i = 0; i < COUNT(write_edmacs); i++)
                if (write_edmacs[i] == channel)
                    return i;
        }
    }
    
    return -1;
}

uint32_t edmac_index_to_channel(uint32_t index, uint32_t direction)
{
    if (index >= COUNT(read_edmacs))
    {
        return -1;
    }
    
    switch (direction)
    {
        case EDMAC_DIR_READ:
            return read_edmacs[index];
            
        case EDMAC_DIR_WRITE:
            return write_edmacs[index];
    }
    
    return -1;
}

uint32_t edmac_get_dir(uint32_t channel)
{
    if (!IS_USED(channel))
    {
        return EDMAC_DIR_UNUSED;
    }
    
    if (IS_WRITE(channel))
    {
        return EDMAC_DIR_WRITE;
    }
    
    if (IS_READ(channel))
    {
        return EDMAC_DIR_READ;
    }
    
    return EDMAC_DIR_UNUSED;
}

uint32_t edmac_get_base(uint32_t channel)
{
    if (channel >= NUM_EDMAC_CHANNELS)
    {
        return -1;
    }

    uint32_t bases[] = { 0xC0F04000, 0xC0F26000, 0xC0F30000 };
    uint32_t edmac_block = channel >> 4;
    uint32_t edmac_num = channel & 0x0F;
    
    return bases[edmac_block] + (edmac_num << 8);
}

uint32_t edmac_get_state(uint32_t channel)
{
    if (channel >= NUM_EDMAC_CHANNELS)
    {
        return -1;
    }
    
    /* this one is retrieved by EngDrvIn (reading directly from the register, not the cached value) */
    return MEM(edmac_get_base(channel) + 0x00);
}

uint32_t edmac_get_flags(uint32_t channel)
{
    if (channel >= NUM_EDMAC_CHANNELS)
    {
        return -1;
    }

    return shamem_read(edmac_get_base(channel) + 0x04);
}

uint32_t edmac_get_address(uint32_t channel)
{
    if (channel >= NUM_EDMAC_CHANNELS)
    {
        return -1;
    }

    return shamem_read(edmac_get_base(channel) + 0x08);
}

uint32_t edmac_get_length(uint32_t channel)
{
    if (channel >= NUM_EDMAC_CHANNELS)
    {
        return -1;
    }

    return shamem_read(edmac_get_base(channel) + 0x10);
}

uint32_t edmac_get_connection(uint32_t channel, uint32_t direction)
{
    if (channel >= NUM_EDMAC_CHANNELS)
    {
        return -1;
    }

    uint32_t addr = 0;
    
    if(direction == EDMAC_DIR_READ)
    { 
        uint32_t dest_chan = 0;
        uint32_t edmac_num = channel & 0x0F;
        
        if ( channel >= 24 )
        {
            if ( channel < 40 )
            {
                dest_chan = edmac_num - 2;
            }
            else
            {
                dest_chan = edmac_num + 4;
            }
        }
        else
        {
            dest_chan = channel - 8;
        }
        
        for(uint32_t pos = 0; pos < 48; pos++)
        {
            addr = 0xC0F05020 + (4 * pos);
            uint32_t dst = shamem_read(addr);
            
            if(dst == dest_chan)
            {
                return pos;
            }
        }
        
        return 0xFF;        
    }
    else
    {
        if (channel == 16)
        {
            addr = 0xC0F0501C;
        }
        else if (channel < 16)
        {
            addr = 0xC0F05000 + (4 * channel);
        }
        else /* channel > 16 */
        {
            uint32_t pos = 0;
            
            if ( channel < 32 )
            {
                pos = channel - 1;
            }
            else
            {
                pos = channel + 6;
            }
            addr = 0xC0F05200 + (4 * (pos & 0x0F));
        }
    }
    return shamem_read(addr);
}


















































