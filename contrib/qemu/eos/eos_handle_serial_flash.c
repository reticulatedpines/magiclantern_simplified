#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//#include "hw/boards.h"
//#include "exec/address-spaces.h"
//#include "exec/memory-internal.h"
//#include "exec/ram_addr.h"
//#include "hw/sysbus.h"
//#include "qemu/thread.h"
//#include "ui/console.h"
//#include "ui/pixel_ops.h"
//#include "hw/display/framebuffer.h"

#include "eos.h"
#include "hw/sd/sd.h"
#include "hw/eos/serial_flash.h"
#include "hw/eos/eos_utils.h"

#include "eos_handle_serial_flash.h"


/* based on pl181_send_command from hw/sd/pl181.c */
#define DPRINTF(fmt, ...) do { printf("[SFIO] " fmt , ## __VA_ARGS__); } while (0)
#define SDIO_STATUS_OK              0x1
#define SDIO_STATUS_ERROR           0x2
#define SDIO_STATUS_DATA_AVAILABLE  0x200000

static void sfio_do_transfer( EOSState *s)
{
	printf("[SFIO] eos_handle_sfio (copying now)\n");
    // FIXME sanitize addresses, this can seriously break stuff
    void * source = &s->sf->data[s->sf->data_pointer];
    printf("[EEPROM-DMA]! [0x%X] -> [0x%X] (0x%X bytes)\n", 
           s->sf->data_pointer, s->sd.dma_addr, s->sd.dma_count);
    for (int i = 0; i < 4; i++) {
        printf("[EEPROM-DATA]: ");
        for (int j = 0; j < 8; j++)
            printf("%02X%02X ", ((uint8_t*)source)[i*16+j*2], ((uint8_t*)source)[i*16+j*2+1]);
        printf("\n");
    }
    // reverse_bytes_order(source, s->sd.dma_count);
    cpu_physical_memory_write(s->sd.dma_addr, source, s->sd.dma_count);
    // reverse_bytes_order(source, s->sd.dma_count);
    s->sd.dma_count = 0;
            //sdio_write_data(&s->sd);
            //sfio_trigger_interrupt(s);

// if (false)                    sfio_trigger_interrupt(s,s->sd);
}

unsigned int sfio_trigger_int_DMA ( EOSState *s )
{
    DPRINTF("sfio_trigger_int_DMA\n");
    sfio_do_transfer(s);
    eos_trigger_int(s, 0x17B, 0);
	return 0;
}

static inline void sfio_trigger_interrupt(EOSState *s, SDIOState *sd)
{
    DPRINTF("sfio_trigger_interrupt IN\n");
    /* after a successful operation, trigger int 0xB1 if requested */
    
    if ((sd->cmd_flags == 0x13 || sd->cmd_flags == 0x14)
        && !(sd->status & SDIO_STATUS_DATA_AVAILABLE))
    {
        /* if the current command does a data transfer, don't trigger until complete */
        DPRINTF("Data transfer not yet complete\n");
        return;
    }
    
//    if ((sd->status & 3) == 1 && sd->irq_flags)
//    {
        eos_trigger_int(s, 0x17B, 0);
        //eos_trigger_int(s, 0xB1, 0);
//    }
    DPRINTF("sfio_trigger_interrupt OUT\n");
}

unsigned int eos_handle_sfio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
	static SDIOState _sd; // FIXME: bad bad bad
    SDIOState * sd = &_sd;

    switch(address & 0xFFF)
    {
        case 0x08:
            msg = "DMA";
            if(type & MODE_WRITE) {
                sd->dma_enabled = value;
            }
            break;
        case 0x0C:
            msg = "Command flags?";
            sd->cmd_flags = value;
            if(type & MODE_WRITE)
            {
                /* reset status before doing any command */
                sd->status = 0;
                
                /* interpret this command */
				printf("[SFIO] sdio_send_command (UNHANDLED)\n");
                // sdio_send_command(&s->sd);
                sd->status |= (SDIO_STATUS_OK|SDIO_STATUS_DATA_AVAILABLE); // Assume it's OK
                
                if (value == 0x14)
                {
                    // sfio_do_transfer(s);
                }
            }
            break;
        case 0x10:
            msg = "Status";
            /**
             * 0x00000001 => command complete
             * 0x00000002 => error
             * 0x00200000 => data available?
             **/
            if(type & MODE_WRITE)
            {
                /* not sure */
                sd->status = value;
            }
            else
            {
                ret = sd->status;
                ret = 0x200001;
            }
            break;
        case 0x14:
            msg = "irq enable?";
            sd->irq_flags = value;

            /* sometimes, a write command ends with this register
             * other times, it ends with SDDMA register 0x78/0x38
             */
            
            if (sd->cmd_flags == 0x13 && sd->dma_enabled && value)
            {
				printf("[SFIO] sfio_write_data (UNHANDLED)\n");
                //sdio_write_data(&s->sd);
            }

            /* sometimes this register is configured after the transfer is started */
            /* since in our implementation, transfers are instant, this would miss the interrupt,
             * so we trigger it from here too. */
            //sfio_trigger_interrupt(s,sd);
            break;
        case 0x18:
            msg = "init?";
            break;
        case 0x20:
            msg = "cmd_lo";
            sd->cmd_lo = value;
            break;
        case 0x24:
            msg = "cmd_hi";
            sd->cmd_hi = value;
            break;
        case 0x28: msg = "Response size (bits)";
            break;
        case 0x2c: msg = "response setup?";
            break;
        case 0x34: msg = "Response[0]";
            ret = sd->response[0];
            break;
        case 0x38: msg = "Response[1]";
            ret = sd->response[1];
            break;
        case 0x3C: msg = "Response[2]";
            ret = sd->response[2];
            break;
        case 0x40: msg = "Response[3]";
            ret = sd->response[3];
            break;
        case 0x44: msg = "Response[4]";
            ret = sd->response[4];
            break;
        case 0x58: msg = "bus width";
            break;
        case 0x5c:
            msg = "write block size";
            sd->write_block_size = value;
            break;
        case 0x64:
            msg = "bus width";
            break;
        case 0x68:
            msg = "read block size";
            sd->read_block_size = value;
            break;
        case 0x6C: msg = "FIFO data?";
            break;
        case 0x70: msg = "transfer status?";
            break;
        case 0x7c:
            msg = "transfer block count";
            sd->transfer_count = value;
            break;
        case 0x80:
            msg = "transferred blocks";
            /* Goro is very strong. Goro never fails. */
            ret = sd->transfer_count;
            break;
        case 0x84: msg = "SDREP: Status register/error codes";
            break;
        case 0x88:
            msg = "SDBUFCTR: Set to 0x03 before reading";
            if (type & MODE_WRITE && value == 0x20) {
                msg = "Set to 0x20 after address set (SFBUFCTR?)";
                sd->dma_addr  = s->sd.dma_addr;
                sd->dma_count = s->sd.dma_count;
                sfio_trigger_int_DMA(s);
            }
            break;
    }

    io_log("SFIO", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

#if 0
static inline unsigned int eos_handle_sfio_old ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
	static SDIOState _sd; // FIXME: bad bad bad
    SDIOState * sd = &_sd;

    switch(address & 0xFFF)
    {
        case 0x08:
            msg = "DMA";
            if(type & MODE_WRITE)
            {
                sd->dma_enabled = value;
            }
            break;
        case 0x0C:
            msg = "Command flags?";
            sd->cmd_flags = value;
            if(type & MODE_WRITE)
            {
                /* reset status before doing any command */
                sd->status = 0;
                
                /* interpret this command */
				printf("[SFIO] sdio_send_command (UNHANDLED)\n");
                // sdio_send_command(&s->sd);
                sd->status |= (SDIO_STATUS_OK|SDIO_STATUS_DATA_AVAILABLE); // Assume it's OK
                
                if (value == 0x14)
                {
                    // sfio_do_transfer(s);
                }
                
                //sfio_trigger_interrupt(s);
        		//eos_trigger_int(s, 0x8C, 0); // 17Bh or 162h ?
                //sfio_trigger_interrupt(s,sd);
            }
            break;
        case 0x10:
            msg = "Status";
            /**
             * 0x00000001 => command complete
             * 0x00000002 => error
             * 0x00200000 => data available?
             **/
            if(type & MODE_WRITE)
            {
                /* not sure */
                sd->status = value;
            }
            else
            {
                ret = sd->status;
                ret = 0x200001;
            }
            break;
        case 0x14:
            msg = "irq enable?";
            sd->irq_flags = value;

            /* sometimes, a write command ends with this register
             * other times, it ends with SDDMA register 0x78/0x38
             */
            
            if (sd->cmd_flags == 0x13 && sd->dma_enabled && value)
            {
				printf("[SFIO] sfio_write_data (UNHANDLED)\n");
                //sdio_write_data(&s->sd);
            }

            /* sometimes this register is configured after the transfer is started */
            /* since in our implementation, transfers are instant, this would miss the interrupt,
             * so we trigger it from here too. */
            //sfio_trigger_interrupt(s,sd);
            break;
        case 0x18:
            msg = "init?";
            break;
        case 0x20:
            msg = "cmd_lo";
            sd->cmd_lo = value;
            break;
        case 0x24:
            msg = "cmd_hi";
            sd->cmd_hi = value;
            break;
        case 0x28:
            msg = "Response size (bits)";
            break;
        case 0x2c:
            msg = "response setup?";
            break;
        case 0x34:
            msg = "Response[0]";
            ret = sd->response[0];
            break;
        case 0x38:
            msg = "Response[1]";
            ret = sd->response[1];
            break;
        case 0x3C:
            msg = "Response[2]";
            ret = sd->response[2];
            break;
        case 0x40:
            msg = "Response[3]";
            ret = sd->response[3];
            break;
        case 0x44:
            msg = "Response[4]";
            ret = sd->response[4];
            break;
        case 0x58:
            msg = "bus width";
            break;
        case 0x5c:
            msg = "write block size";
            sd->write_block_size = value;
            break;
        case 0x64:
            msg = "bus width";
            break;
        case 0x68:
            msg = "read block size";
            sd->read_block_size = value;
            break;
        case 0x6C:
            msg = "FIFO data?";
            break;
        case 0x70:
            msg = "transfer status?";
            break;
        case 0x7c:
            msg = "transfer block count";
            sd->transfer_count = value;
            break;
        case 0x80:
            msg = "transferred blocks";
            /* Goro is very strong. Goro never fails. */
            ret = sd->transfer_count;
            break;
        case 0x84:
            msg = "SDREP: Status register/error codes";
            break;
        case 0x88:
            msg = "SDBUFCTR: Set to 0x03 before reading";
            if (type & MODE_WRITE && value == 0x20) {
                msg = "SFBUFCTR: Set to 0x20 after address set";
                // s->sd.dma_addr = value;
                // s->sd.dma_count = value;
            }
            break;
    }

    io_log("SFIO", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}
#endif

unsigned int eos_handle_sio_serialflash ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    //unsigned int pc = s->cpu->env.regs[15];
    if (s->sf == NULL) return 0;

    if ((type & MODE_READ))
    {
        switch(address & 0xFF)
        {
            case 0x04:
		        value = serial_flash_write_poll(s->sf);
                //printf("[SPI:%i:%02X] ", parm, address & 0xff);
                // printf("[BUSY] >> %d (pc: 0x%08X)\r\n", value, pc);
                return value;
            case 0x10:
                //printf("[SPI:%i:%02X] ", parm, address & 0xff);
                //printf("[WMODE?] >> 0 (write mode?) (pc: 0x%08X)\r\n", pc);
                return 0; // Unk, set to zero before write
            case 0x1C:
		        value = serial_flash_spi_read(s->sf);
                // printf("[SPI:%i:%02X] ", parm, address & 0xff);
                // printf("[TX] >> 0x%02X (pc: 0x%08X)...\r\n", value, pc);
                // last_was_tx = 1;
                return value;
                //return 0;
            default:
                //printf("[SPI:%i:%02X] ", parm, address & 0xff);
                //printf("[???] >> 0 (pc: 0x%08X)\r\n", pc);
                return 0;
        }
    }
    if ((type & MODE_WRITE))
    {
        switch(address & 0xFF)
        {
            case 0x04:
//                printf("[BUSY] << %d (set wait flag) (pc: 0x%08X)\r\n", value, pc);
             //   printf("[SPI:%i:%02X] ", parm, address & 0xff);
                return 0;
            case 0x10:
                //printf("[SPI:%i:%02X] ", parm, address & 0xff);
                //printf("[WMODE?] << 0 (write mode?) (pc: 0x%08X)\r\n", pc);
                return 0; // Unk, set to zero before write
            case 0x18:
                //printf("[SPI:%i:%02X] ", parm, address & 0xff);
                //printf("[RX] << 0x%X (pc: 0x%08X)\r\n", value, pc);
		        serial_flash_spi_write(s->sf,value);
                return 0;
            case 0x38:
                //printf("[SPI:%i:%02X] ", parm, address & 0xff);
                // Set to (([SF_data,#20] != 1) ? 0x80800408 : 0x80A00408) before write (mode)
                //printf("[MODE] << 0x%X (pc: 0x%08X)\r\n", value, pc);
                return 0;
            default:
                //printf("[SPI:%i:%02X] ", parm, address & 0xff);
                //printf("[???] << 0x%X (pc: 0x%08X)\r\n", value, pc);
                return 0;
        }
    }
    return 0;
}

#if 0
// Unused, for testing

unsigned int eos_handle_spidma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;
//    int i;

    static uint32_t dma_addr = 0;
    static uint32_t dma_count = 0;
    static uint32_t dma_delay = 0;


    if (s->sf == NULL) return 0;

    switch(address & 0xFFF)
    {
        case 0x60:
        case 0x20:
            msg = "Transfer memory address";
            dma_addr = value;
            dma_delay = 10;
            break;
        case 0x64:
        case 0x24:
            msg = "Transfer byte count";
            dma_count = value;
            break;
        case 0x70:
        case 0x30:
            msg = "Flags/Status";
            if ((type & MODE_READ))
            {
                if (dma_delay) {
                    dma_delay--;
                    ret = 1;
                } else {
                    ret = 0;
                    if (dma_count > 0)
                    {
                        void * source = &s->sf->data[s->sf->data_pointer];
                        printf("[EEPR-DMA]  [0x%X] -> [0x%X] (0x%X bytes)\n", s->sf->data_pointer, dma_addr, dma_count);
                        cpu_physical_memory_write(dma_addr, source, dma_count);
                        dma_count = 0;
                    }
                }
            }
            break;
        case 0x78:
        case 0x38:
            msg = "Transfer start?";

            //cpu_physical_memory_write(dma_addr, &sf_data[sf_address], dma_count);
            if (dma_count > 0)
            {
                void * source = &s->sf->data[s->sf->data_pointer];
                printf("[EEPR-DMA]! [0x%X] -> [0x%X] (0x%X bytes)\n", s->sf->data_pointer, dma_addr, dma_count);
                cpu_physical_memory_write(dma_addr, source, dma_count);
                dma_count = 0;
            }
            //sdio_write_data(&s->sd);
            //sfio_trigger_interrupt(s);

            break;

        default:
            msg = "???";
            ret = 1;
            break;
    }

    io_log("EEPR-DMA", s, address, type, value, ret, msg, 0, 0);
    return ret;
}
#endif // #if 0


