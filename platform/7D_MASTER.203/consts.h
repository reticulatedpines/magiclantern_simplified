#define CARD_LED_ADDRESS 0xC022D06C // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x138800
#define LEDOFF 0x838C00

#define HIJACK_CACHE_HACK

#define HIJACK_CACHE_HACK_BSS_END_ADDR   0xFF811508
#define HIJACK_CACHE_HACK_BSS_END_INSTR  0xE3A01732
#define HIJACK_CACHE_HACK_INITTASK_ADDR  0xFF811064

#define MVR_516_STRUCT_MASTER (*(void**)0x1B80) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.


//~ max volume supported for beeps
#define ASIF_MAX_VOL 5
