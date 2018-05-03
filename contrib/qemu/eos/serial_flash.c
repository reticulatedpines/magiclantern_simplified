#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "eos.h"
#include "hw/sd/sd.h"
#include "hw/eos/serial_flash.h"
#include "hw/eos/eos_utils.h"
#include "model_list.h"

#include "serial_flash.h"

#define SF_DPRINTF(fmt, ...) DPRINTF("[SFIO] ",   EOS_LOG_SFLASH, fmt, ## __VA_ARGS__)
#define SF_EPRINTF(fmt, ...) EPRINTF("[SFIO] ",   EOS_LOG_SFLASH, fmt, ## __VA_ARGS__)
#define EE_DPRINTF(fmt, ...) DPRINTF("[EEPROM] ", EOS_LOG_SFLASH, fmt, ## __VA_ARGS__)
#define EE_EPRINTF(fmt, ...) EPRINTF("[EEPROM] ", EOS_LOG_SFLASH, fmt, ## __VA_ARGS__)
#define EE_VPRINTF(fmt, ...) VPRINTF("[EEPROM] ", EOS_LOG_SFLASH, fmt, ## __VA_ARGS__)

static const char * spi_opname(int code)
{
    switch (code) {
        case 0x01: return "WRSR";    // Write Status Register
        case 0x05: return "RDSR";    // Read Status Register
        case 0x08: return "LPWP";    // Low Power Write Poll
        case 0x06: return "WREN";    // Set Write Enable Latch
        case 0x04: return "WRDI";    // Reset Write Enable Latch
        case 0x9f: return "RDID";    // Read identification
        case 0x03: return "READ";    // Read from Memory Array
        case 0x02: return "WRITE-B"; // Write byte to Memory
        case 0x07: return "WRITE-A"; // Write array to Memory
        case 0x6B: return "QOFR";    // Quad Output Fast Read
        default:   return "???";
    }
}

SerialFlashState * serial_flash_init(const char * filename, size_t size)
{
    // Allocate
    SerialFlashState * sf = (SerialFlashState*) malloc(sizeof(SerialFlashState));
    if (sf == NULL) {
        return NULL;
    }

    // Initialize struct
    memset(sf,0,sizeof(SerialFlashState));
    sf->data = (uint8_t*) malloc(size);
    sf->size = size;
    memcpy(sf->RDID_seq, (uint8_t[3])RDID_MACRONIX, sizeof(sf->RDID_seq));

    if (sf->data == NULL) {
        free(sf);
        fprintf(stderr, "Could not allocate %zd (0x%zX) bytes for serial flash\n", size, size);
        return NULL;
    }

    // Initialize data
    FILE * f = (filename != NULL) ? fopen(filename, "rb") : NULL;
    if (f != NULL) {
        fprintf(stderr, "[EOS] loading '%s' as serial flash, size=0x%X\n", filename, (int) size);
        size_t read_size = fread(sf->data, sizeof(uint8_t), size, f);
        if (read_size != size) {
            fprintf(stderr, "Could not read %zd (0x%zX) bytes from %s (was %zd)\n", size, size, filename, read_size);
            memset(sf->data,0,size);
        }
        fclose(f);
    } else {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    
    return sf;
}

void serial_flash_free(SerialFlashState * sf)
{
    free(sf->data);
    free(sf);
}

void serial_flash_set_CS(SerialFlashState * sf, int value)
{
    if (value == 1) {
        if (sf->state == 0x03) { // Read array
            EE_DPRINTF("Verbose: Sent %d bytes\n", sf->rw_count);
        } else if (sf->state == 0x07) { // Write array
            EE_DPRINTF("Verbose: Received %d bytes\n", sf->rw_count);
        }
        sf->data_pointer = 0xFFFFFFFF;
        sf->state = 0;
        sf->substate = 0;
        sf->read_value = 0;
        sf->write_poll = 0;
    }

    EE_DPRINTF("CS = %d\n", value);
}

uint8_t serial_flash_write_poll(SerialFlashState * sf)
{
    uint8_t ret = (sf->write_poll > 0) ? 1 : 0;
    if (ret) sf->write_poll--;
    return ret;
}

uint8_t serial_flash_spi_read(SerialFlashState * sf)
{
    uint8_t ret = sf->read_value;
    switch (sf->state) {
        case 0x6B: // QOFR: Quad Output Fast Read
        case 0x03: // Read array
            // fprintf(stderr, "A: %X\n",sf->read_value);
            // fprintf(stderr, "B: %X\n",sf->data[sf->data_pointer]);
            // fprintf(stderr, "i: %p[0x%X]\n",sf->data,sf->data_pointer);
            sf->data_pointer++;
            if (sf->data_pointer >= sf->size)
                sf->data_pointer -= sf->size;
            sf->read_value = sf->data[sf->data_pointer];
            sf->rw_count++;
            sf->write_poll = 10; // TODO parameter
            // fprintf(stderr, "C: %X\n",sf->read_value);
            // fprintf(stderr, "D: %X\n",sf->data[sf->data_pointer]);
            // fprintf(stderr, "j: %p[0x%X]\n",sf->data,sf->data_pointer);
            break;

        case 0x05: // Read status
            sf->state = 0;
            break;

        case 0x9f: // Read id
            sf->read_value = sf->RDID_seq[sf->substate+1];
            sf->rw_count++;
            sf->substate++;
            if (sf->substate == 3)
            {
                sf->state = 0;
                sf->substate = 0;
            }
            EE_DPRINTF("Verbose: READ in RDID = %02Xh\n", ret);
            break;

        default:
            EE_EPRINTF("Error: read SO in state=%d\n", sf->state);
            sf->read_value = 0;
            break;
    }
    EE_VPRINTF("READ >> 0x%X\n", ret);
    return ret;
}

void serial_flash_spi_write(SerialFlashState * sf, uint8_t value)
{
    // If standby
    if (sf->state == 0)
    {
        switch (value)
        {
            case 0x01: // WRSR: Write Status Register
                EE_DPRINTF("Verbose: [SR] << ...\n");
                sf->state = 0x01;
                sf->status_register = 0;
                break;

            case 0x05: // RDSR: Read Status Register
                EE_DPRINTF("Verbose: [SR] >> 0x%X\n", sf->status_register);
                sf->read_value = sf->status_register;
                sf->state = 0x05;
                break;

            case 0x08: // LPWP: Low Power Write Poll
                EE_DPRINTF("Verbose: Write Poll\n");
                // Pretend to use some time...
                sf->read_value = (sf->write_poll > 0) ? 1 : 0;
                if (sf->read_value) sf->write_poll--;
                break;

            case 0x06: // WREN: Set Write Enable Latch
                EE_DPRINTF("Verbose: Set Write Enable Latch\n");
                sf->status_register |= (1 << 1); // Set WEL bit
                break;

            case 0x04: // WRDI: Reset Write Enable Latch
                EE_DPRINTF("Verbose: Reset Write Enable Latch\n");
                sf->status_register &= ~(1 << 1); // Unset WEL bit
                break;

            case 0x9f: // RDID: Read identification
                EE_DPRINTF("Verbose: Got RDID\n");
                sf->read_value = sf->RDID_seq[0];
                sf->state = 0x9f;
                sf->substate = 0;
                break;
                


            case 0x03: // READ: Read from Memory Array
            case 0x02: // WRITE: Write byte to Memory
            case 0x07: // WRITE: Write array to Memory
            case 0x6B: // QOFR: Quad Output Fast Read
                EE_DPRINTF("Verbose: Got %s (%02Xh)\n", spi_opname(value), value);
                sf->state = value;
                sf->substate = 0;
                sf->data_pointer = 0;
                sf->rw_count = 0;
                sf->write_poll = 0;
                break;

            default:
                EE_EPRINTF("Error: Illegal opcode 0x%02X\n", value);
                break;
        }
        return;
    }

    // If non-standby state

    // WRSR: Write Status Register
    if (sf->state == 0x01) {
        sf->status_register = value;
        sf->state = 0;
        sf->substate = 0;
        EE_DPRINTF("Verbose: [SR] << 0x%02X\n", value);
        return;
    }

    // Byte write
    if (sf->state == 0x02 && sf->substate == 3) {
        sf->data[sf->data_pointer] = value;
        sf->state = 0;
        sf->substate = 0;
        EE_DPRINTF("Verbose: Wrote byte 0x%02X @ 0x%06X\n", value, sf->data_pointer);
        return;
    }

    // Array write
    if (sf->state == 0x07 && sf->substate == 3) {
        sf->data[sf->data_pointer] = value;
        EE_DPRINTF("Verbose: Wrote array byte 0x%02X @ 0x%06X\n", value, sf->data_pointer);
        sf->data_pointer = (sf->data_pointer+1) % sf->size;
        sf->rw_count++;
        return;
    }

    // Address read
    if ((sf->state == 0x03 || sf->state == 0x02 || sf->state == 0x07 || sf->state == 0x6B) && sf->substate < 3) {
        sf->data_pointer |= (value << (8*(2 - sf->substate)));
        sf->substate++;
        if (sf->substate == 3) {
            EE_DPRINTF("Verbose: address is now: 0x%06X\n", sf->data_pointer);
            if (sf->state == 0x03) {
                sf->read_value = sf->data[sf->data_pointer];
            }
        }
        sf->write_poll = 10;
        return;
    }

    // Otherwise invalid
    EE_EPRINTF("WRITE in illegal state (state = %02Xh:%d, val=%d)\n", sf->state, sf->substate, value);
}


/* based on pl181_send_command from hw/sd/pl181.c */
#define SDIO_STATUS_OK              0x1
#define SDIO_STATUS_ERROR           0x2
#define SDIO_STATUS_DATA_AVAILABLE  0x200000

#define BLOCK_SIZE   0x7F0
#define BLOCK_OFFSET 0x800

static void sfio_do_transfer( EOSState *s)
{
    SF_DPRINTF("eos_handle_sfio (copying now)\n");

    // FIXME: bad bad
    SDIOState * sd = &s->sf->sd;

    // FIXME sanitize addresses, this can seriously break stuff
    void * source = &s->sf->data[s->sf->data_pointer];
    fprintf(stderr, "[EEPROM-DMA]! [0x%X] -> [0x%X] (0x%X bytes)\n", 
           s->sf->data_pointer, sd->dma_addr, sd->dma_count);

    /* the data appears screwed up a bit - offset by half-byte?! */
    int num_blocks = (sd->dma_count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    // TODO assert that num_blocks is equal to num blocks sent to controller
    // TODO assert that BLOCK_SIZE is equal to block size sent to controller
    for (int i = 0; i < num_blocks; i++) {
        uint8_t * block_src = (uint8_t*)(source + i*BLOCK_OFFSET);
        uint32_t  block_dst = (uint32_t)(sd->dma_addr + i*BLOCK_SIZE);
        uint8_t block[BLOCK_SIZE];
        for (int j = 0; j < BLOCK_SIZE; j++) {
            uint8_t this = *(uint8_t*)(block_src + j);
            uint8_t next = *(uint8_t*)(block_src + j + 1);
            uint8_t byte = (this << 4) | (next >> 4);
            block[j] = byte;

            if (i == 0 && j < 16*4)
            {
                qemu_log_mask(EOS_LOG_SFLASH, "%s%02X%s",
                    (j % 16 == 0) ? "[EEPROM-DATA]: " : "",
                    byte,
                    (j % 16 == 15) ? "\n" : " "
                );
            }
        }
        eos_mem_write(s, block_dst, block, BLOCK_SIZE);
    }
    sd->dma_count = 0;
            //sdio_write_data(sd);
            //sfio_trigger_interrupt(s);

// if (false)                    sfio_trigger_interrupt(s,sd);
}

static unsigned int sfio_trigger_int_DMA ( EOSState *s )
{
    SF_DPRINTF("sfio_trigger_int_DMA\n");
    sfio_do_transfer(s);
    eos_trigger_int(s, s->model->serial_flash_interrupt, 0);
    return 0;
}

static inline void sfio_trigger_interrupt(EOSState *s, SDIOState *sd)
{
    SF_DPRINTF("sfio_trigger_interrupt IN\n");
    /* after a successful operation, trigger int 0xB1 if requested */
    
    if ((sd->cmd_flags == 0x13 || sd->cmd_flags == 0x14)
        && !(sd->status & SDIO_STATUS_DATA_AVAILABLE))
    {
        /* if the current command does a data transfer, don't trigger until complete */
        SF_DPRINTF("Data transfer not yet complete\n");
        return;
    }
    
//    if ((sd->status & 3) == 1 && sd->irq_flags)
//    {
        eos_trigger_int(s, s->model->serial_flash_interrupt, 0);
        //eos_trigger_int(s, 0xB1, 0);
//    }
    SF_DPRINTF("sfio_trigger_interrupt OUT\n");
}

unsigned int eos_handle_sfio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;

    // FIXME: bad bad
    SDIOState * sd = &s->sf->sd;

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
                SF_DPRINTF("sdio_send_command (UNHANDLED)\n");
                // sdio_send_command(&s->sd);
                sd->status |= (SDIO_STATUS_OK|SDIO_STATUS_DATA_AVAILABLE); // Assume it's OK
                
                if (value == 0x14)
                {
                    sd->status &= ~1;
                    sfio_do_transfer(s);
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
                SF_EPRINTF("sfio_write_data (UNHANDLED)\n");
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
                sfio_trigger_int_DMA(s);
            }
            break;
    }

    if (qemu_loglevel_mask(EOS_LOG_SFLASH)) {
        io_log("SFIO", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    }
    return ret;
}

unsigned int eos_handle_sfdma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;

    // FIXME: bad bad
    SDIOState * sd = &s->sf->sd;

    switch(address & 0x1F)
    {
        case 0x00:
            msg = "Transfer memory address";
            MMIO_VAR(sd->dma_addr);
            break;
        case 0x04:
            msg = "Transfer byte count";
            if (type & MODE_WRITE)
            {
                sd->dma_count = value;
            }
            break;
        case 0x10:
            msg = "Command/Status?";
            if (type & MODE_WRITE)
            {
                sd->dma_enabled = value & 1;
            }
            break;
        case 0x14:
            msg = "Status?";
            break;
        case 0x18:
            break;
    }

    io_log("SFDMA", s, address, type, value, ret, msg, 0, 0);
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
                SF_EPRINTF("sdio_send_command (UNHANDLED)\n");
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
                SF_EPRINTF("sfio_write_data (UNHANDLED)\n");
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
    unsigned int ret = 0;
    const char * msg = 0;

    if (s->sf == NULL)
    {
        goto end;
    }

    switch(address & 0xFF)
    {
        case 0x04:
            if (type & MODE_READ) {
                msg = "busy (write poll)";
                ret = serial_flash_write_poll(s->sf);
            }
            break;
        case 0x10:
            msg = "write mode?";
            break;
        case 0x18:
            msg = "write";
            if (type & MODE_WRITE) {
                serial_flash_spi_write(s->sf,value);
            }
            break;
        case 0x1C:
            msg = "read";
            if (type & MODE_READ) {
                ret = serial_flash_spi_read(s->sf);
            }
            // last_was_tx = 1;
            break;
        case 0x38:
            msg = "mode";
            MMIO_VAR(s->sf->mode);
            break;
    }

end:
    if (qemu_loglevel_mask(EOS_LOG_SFLASH) &&
        qemu_loglevel_mask(EOS_LOG_VERBOSE))
    {
        char sio_name[16];
        snprintf(sio_name, sizeof(sio_name), "SIO%d-SF", parm);
        io_log(sio_name, s, address, type, value, ret, msg, 0, 0);
    }
    return ret;
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
                        fprintf(stderr, "[EEPR-DMA]  [0x%X] -> [0x%X] (0x%X bytes)\n", s->sf->data_pointer, dma_addr, dma_count);
                        eos_mem_write(s, dma_addr, source, dma_count);
                        dma_count = 0;
                    }
                }
            }
            break;
        case 0x78:
        case 0x38:
            msg = "Transfer start?";

            //eos_mem_write(s, dma_addr, &sf_data[sf_address], dma_count);
            if (dma_count > 0)
            {
                void * source = &s->sf->data[s->sf->data_pointer];
                fprintf(stderr, "[EEPR-DMA]! [0x%X] -> [0x%X] (0x%X bytes)\n", s->sf->data_pointer, dma_addr, dma_count);
                eos_mem_write(s, dma_addr, source, dma_count);
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
