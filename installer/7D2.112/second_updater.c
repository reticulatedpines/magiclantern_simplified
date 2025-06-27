// macros to convert the passed in MAIN_FIRMWARE_ADDR
// from number to string, as asm sections work on strings only.
#define STR(x) #x
#define XSTR(s) STR(s)

asm(
    ".text\n"
    ".globl _start\n"
"_start:\n"
    // 7D2 slave loads FIR updater code as ARM, not Thumb
    ".code 32\n"
    "ldr pc, =" XSTR(MAIN_FIRMWARE_ADDR) "\n"
);
