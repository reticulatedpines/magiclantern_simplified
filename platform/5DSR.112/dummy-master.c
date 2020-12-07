asm(
    ".text\n"
    ".globl _start\n"
    "_start:\n"

    /* jump to Canon firmware */
    /* note: with B 0xFE0A0000, gcc inserts a veneer */
    "LDR R0, =0xD20C0084\n"
    "MOV R1, #0\n"
    "STR R1, [R0]\n"
    "LDR PC, =0xFE0A0000\n"
);
