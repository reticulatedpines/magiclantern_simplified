#ifndef EOS_ML_HELPERS_H
#define EOS_ML_HELPERS_H

#include <stddef.h>

#include "eos.h"

unsigned int eos_handle_ml_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_ml_fio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

/** Some small engio API **/
#define REG_PRINT_CHAR 0xCF123000
#define REG_SHUTDOWN   0xCF123004
#define REG_DUMP_VRAM  0xCF123008
#define REG_PRINT_NUM  0xCF12300C
#define REG_GET_KEY    0xCF123010
#define REG_BMP_VRAM   0xCF123014
#define REG_IMG_VRAM   0xCF123018
#define REG_RAW_BUFF   0xCF12301C
#define REG_DISP_TYPE  0xCF123020

/*
 * FIO access to a local directory
 * A:/ mapped to cfcard/ and B:/ mapped to sdcard/
 * Single-user, single-task for now (only one file open at a time)
 */
#define REG_FIO_NUMERIC_ARG0    0xCF123F00  // R/W
#define REG_FIO_NUMERIC_ARG1    0xCF123F04  // R/W
#define REG_FIO_NUMERIC_ARG2    0xCF123F08  // R/W
#define REG_FIO_NUMERIC_ARG3    0xCF123F0C  // R/W
#define REG_FIO_BUFFER          0xCF123F10  // R/W; buffer position auto-increments; used to pass filenames or to get data
#define REG_FIO_BUFFER_SEEK     0xCF123F14  // MEM(REG_FIO_BUFFER_SEEK) = position;
#define REG_FIO_GET_FILE_SIZE   0xCF123F20  // filename in buffer; size = MEM(REG_FIO_GET_FILE_SIZE);
#define REG_FIO_OPENDIR         0xCF123F24  // path name in buffer; ok = MEM(REG_FIO_OPENDIR);
#define REG_FIO_CLOSEDIR        0xCF123F28  // ok = MEM(REG_FIO_CLOSEDIR);
#define REG_FIO_READDIR         0xCF123F2C  // ok = MEM(REG_FIO_READDIR); dir name in buffer; size, mode, time in arg0-arg2
#define REG_FIO_OPENFILE        0xCF123F34  // file name in buffer; ok = MEM(REG_FIO_OPENFILE);
#define REG_FIO_CLOSEFILE       0xCF123F38  // ok = MEM(REG_FIO_CLOSEFILE);
#define REG_FIO_READFILE        0xCF123F3C  // size in arg0; pos in arg1; bytes_read = MEM(REG_FIO_READFILE); contents in buffer



#endif

