#ifndef SERIAL_FLASH_H
#define SERIAL_FLASH_H

#include <stdint.h>
#include <eos.h>

#define RDID_MACRONIX {0xC2,0x10,0x0C}

// TODO Sometimes WEN should be unset after operations
// TODO Enforce WEN
// TODO Is simulatneous read/write allowed?
// TODO Enforce toggled CS?

typedef struct SerialFlashState {
    // Data
    uint8_t * data;
    size_t size;
    uint8_t RDID_seq[3];
    
    // State
    uint32_t status_register;
    uint32_t data_pointer;
    unsigned int state;
    unsigned int substate;
    unsigned int read_value;
    unsigned int write_poll;
    uint32_t mode;

    // fixme
    SDIOState sd;

    // Debug state
    unsigned int rw_count;
} SerialFlashState;


// Functions
// default: serial_flash_init("SFDATA.bin",0x1000000);
SerialFlashState * serial_flash_init(const char * filename, size_t size);
void serial_flash_free(SerialFlashState * sf);
void serial_flash_set_CS(SerialFlashState * sf, int value);
uint8_t serial_flash_write_poll(SerialFlashState * sf);
uint8_t serial_flash_spi_read(SerialFlashState * sf);
void serial_flash_spi_write(SerialFlashState * sf, uint8_t value);

unsigned int eos_handle_sfio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sio_serialflash ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sfdma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

#endif

