.text

#define NSTUB(addr,name) \
	.global name; \
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )

#define RAM_OFFSET 0xFF9DEF48 // some functions are copied to RAM at around ff0c009c; they have to be called from RAM...

NSTUB( 0x1C448, bzero32 )
NSTUB( 0x2EB8, create_init_task )
NSTUB( 0xFF0C1C0C, cstart )
NSTUB(0x8b10, task_create)
NSTUB(    0x8a5c, msleep)

NSTUB( 0xFF309994, FIO_CreateFile )
NSTUB( 0xFF309D38, FIO_CloseFile )
NSTUB( 0xFF309C88, FIO_WriteFile )

NSTUB(0xff0c520c, init_task)
