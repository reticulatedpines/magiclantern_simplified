asm(
    ".text\n"
    ".globl _start\n"
    "_start:\n"

    "B  0xFF810000\n"   /* jump to Canon firmware */
);
