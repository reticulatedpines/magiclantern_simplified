#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "serial_flash.h"


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
	sf->verbose = 1;
	if (sf->data == NULL) {
		free(sf);
	    fprintf(stderr, "Could not allocate %zd (0x%zX) bytes for serial flash\n", size, size);
		return NULL;
	}

	// Initialize data
	FILE * f = (filename != NULL) ? fopen(filename, "rb") : NULL;
	if (f != NULL) {
	    size_t read_size = fread(sf->data, sizeof(uint8_t), size, f);
	    if (read_size != size) {
	        fprintf(stderr, "Could not read %zd (0x%zX) bytes from %s (was %zd)\n", size, size, filename, read_size);
			memset(sf->data,0,size);
		}
	    fclose(f);
	} else {
		memset(sf->data,0,size);
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
		if (sf->verbose) {
		    if (sf->state == 0x03) { // Read array
		        printf("[EEPROM]: Verbose: Sent %d bytes\n", sf->rw_count);
		    } else if (sf->state == 0x07) { // Write array
		        printf("[EEPROM]: Verbose: Received %d bytes\n", sf->rw_count);
		    }
		}
	    sf->data_pointer = 0xFFFFFFFF;
	    sf->state = 0;
	    sf->substate = 0;
	    sf->read_value = 0;
	    sf->write_poll = 0;
	}

	if (sf->verbose)
		printf("[EEPROM]: CS = %d\n", value);
}

uint8_t serial_flash_write_poll(SerialFlashState * sf)
{
	uint8_t ret = (sf->write_poll > 0) ? 1 : 0;
	if (ret) sf->write_poll--;
	return ret;
}

uint8_t serial_flash_spi_read(SerialFlashState * sf)
{
    int ret = sf->read_value;
    switch (sf->state) {
        case 0x6B: // QOFR: Quad Output Fast Read
        case 0x03: // Read array
            sf->data_pointer++;
			if (sf->data_pointer >= sf->size)
            	sf->data_pointer -= sf->size;
            sf->read_value = sf->data[sf->data_pointer];
            sf->rw_count++;
            sf->write_poll = 10; // TODO parameter
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
			if (sf->verbose)
            	printf("[EEPROM]: Verbose: READ in RDID = %02Xh\n", ret);
            break;

        default:
            printf("[EEPROM]: Error: read SO in state=%d\n", sf->state);
            sf->read_value = 0;
            break;
    }
    //printf("[EEPROM]: READ >> %d\n", ret);
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
				if (sf->verbose)
					printf("[EEPROM]: Verbose: [SR] << ...\n");
                sf->state = 0x01;
                sf->status_register = 0;
                break;

            case 0x05: // RDSR: Read Status Register
				if (sf->verbose)
                	printf("[EEPROM]: Verbose: [SR] >> 0x%X\n", sf->status_register);
                sf->read_value = sf->status_register;
                sf->state = 0x05;
                break;

            case 0x08: // LPWP: Low Power Write Poll
				if (sf->verbose)
                	printf("[EEPROM]: Verbose: Write Poll\n");
                // Pretend to use some time...
                sf->read_value = (sf->write_poll > 0) ? 1 : 0;
                if (sf->read_value) sf->write_poll--;
                break;

            case 0x06: // WREN: Set Write Enable Latch
				if (sf->verbose)
                	printf("[EEPROM]: Verbose: Set Write Enable Latch\n");
                sf->status_register |= (1 << 1); // Set WEL bit
                break;

            case 0x04: // WRDI: Reset Write Enable Latch
				if (sf->verbose)
                	printf("[EEPROM]: Verbose: Reset Write Enable Latch\n");
                sf->status_register &= ~(1 << 1); // Unset WEL bit
                break;

            case 0x9f: // RDID: Read identification
				if (sf->verbose)
                	printf("[EEPROM]: Verbose: Got RDID\n");
                sf->read_value = sf->RDID_seq[0];
                sf->state = 0x9f;
                sf->substate = 0;
                break;
                


            case 0x03: // READ: Read from Memory Array
            case 0x02: // WRITE: Write byte to Memory
            case 0x07: // WRITE: Write array to Memory
            case 0x6B: // QOFR: Quad Output Fast Read
				if (sf->verbose)
                	printf("[EEPROM]: Verbose: Got %s (%02Xh)\n", spi_opname(value), value);
                sf->state = value;
                sf->substate = 0;
                sf->data_pointer = 0;
                sf->rw_count = 0;
                sf->write_poll = 0;
                break;

            default:
                printf("[EEPROM]: Error: Illegal opcode 0x%02X\n", value);
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
		if (sf->verbose)
        	printf("[EEPROM]: Verbose: [SR] << 0x%02X\n", value);
		return;
    }

    // Byte write
    if (sf->state == 0x02 && sf->substate == 3) {
        sf->data[sf->data_pointer] = value;
        sf->state = 0;
        sf->substate = 0;
		if (sf->verbose)
        	printf("[EEPROM]: Verbose: Wrote byte 0x%02X @ 0x%06X\n", value, sf->data_pointer);
		return;
    }

    // Array write
    if (sf->state == 0x07 && sf->substate == 3) {
        sf->data[sf->data_pointer] = value;
		if (sf->verbose)
        	printf("[EEPROM]: Verbose: Wrote array byte 0x%02X @ 0x%06X\n", value, sf->data_pointer);
        sf->data_pointer = (sf->data_pointer+1) % sf->size;
        sf->rw_count++;
		return;
    }

    // Address read
    if ((sf->state == 0x03 || sf->state == 0x02 || sf->state == 0x07 || sf->state == 0x6B) && sf->substate < 3) {
        sf->data_pointer |= (value << (8*(2 - sf->substate)));
        sf->substate++;
        if (sf->substate == 3) {
			if (sf->verbose)
            	printf("[EEPROM]: Verbose: address is now: 0x%06X\n", sf->data_pointer);
            if (sf->state == 0x03)
                sf->read_value = sf->data[sf->data_pointer];
        }
        sf->write_poll = 10;
		return;
    }

    // Otherwise invalid
    printf("[EEPROM]: Error: WRITE in illegal state (state = %02Xh:%d, val=%d)\n", sf->state, sf->substate, value);
}

