
#include "dryos.h"
#include "edmac.h"

#if defined(CONFIG_5D3) || defined(CONFIG_6D) /* 6D + 5D3 are Identical */

#define WRITE(x) (x)
#define READ(x)  (0x80000000 | (x))

#define IS_USED(x) ((x) != 0xFFFFFFFF)
#define IS_WRITE(x) (((x) & 0x80000000) == 0)
#define IS_READ(x)  (((x) & 0x80000000) != 0)

/* channel usage for 5D3 */
static uint32_t edmac_chanlist[] = 
{
    WRITE(0), WRITE(1), WRITE(2), WRITE(3), WRITE(4), WRITE(5), WRITE(6), 0xFFFFFFFF,
    READ(0), READ(1), READ(2), READ(3), READ(4), READ(5), 0xFFFFFFFF, 0xFFFFFFFF,
    WRITE(7),  WRITE(8), WRITE(9), WRITE(10), WRITE(11), WRITE(12), WRITE(13), 0xFFFFFFFF,
    READ(6), READ(7), READ(8), READ(9), READ(10), READ(11), 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    WRITE(14), WRITE(15), 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    READ(12), READ(13), READ(14), READ(15), 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
};

uint32_t edmac_get_dir(uint32_t channel)
{
    if(!IS_USED(edmac_chanlist[channel]))
    {
        return EDMAC_DIR_UNUSED;
    }
    
    if(IS_WRITE(edmac_chanlist[channel]))
    {
        return EDMAC_DIR_WRITE;
    }
    
    if(IS_READ(edmac_chanlist[channel]))
    {
        return EDMAC_DIR_READ;
    }
    
    return EDMAC_DIR_UNUSED;
}

#endif

uint32_t edmac_get_base(uint32_t channel)
{
    uint32_t bases[] = { 0xC0F04000, 0xC0F26000, 0xC0F30000 };
    uint32_t edmac_block = channel >> 4;
    uint32_t edmac_num = channel & 0x0F;
    
    return bases[edmac_block] + (edmac_num << 8);
}

uint32_t edmac_get_state(uint32_t channel)
{
    /* this one is retrieved by EngDrvIn (reading directly from the register, not the cached value) */
    return MEM(edmac_get_base(channel) + 0x00);
}

uint32_t edmac_get_flags(uint32_t channel)
{
    return shamem_read(edmac_get_base(channel) + 0x04);
}

uint32_t edmac_get_address(uint32_t channel)
{
    return shamem_read(edmac_get_base(channel) + 0x08);
}

uint32_t edmac_get_length(uint32_t channel)
{
    return shamem_read(edmac_get_base(channel) + 0x10);
}

uint32_t edmac_get_connection(uint32_t channel, uint32_t direction)
{
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
        if ( channel < 16 )
        {
            addr = 0xC0F05000 + (4 * channel);
        }
        if ( channel != 16 )
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
        else
        {
            addr = 0xC0F0501C;
        }
    }
    return shamem_read(addr);
}


















































