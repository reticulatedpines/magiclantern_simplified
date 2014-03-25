asm(
    ".text\n"
    ".globl _start\n"
    "_start:\n"

    /* if used in a .fir file, there is a 0x120 byte address offset.
       so cut the first 0x120 bytes off autoexec.bin before embedding into .fir
     */
    "B       skip_fir_header\n"
    ".incbin \"../../platform/7D.203/version.bin\"\n" // this must have exactly 0x11C (284) bytes
    "skip_fir_header:\n"
    "B  0xFF810000\n"   /* jump to Canon firmware */
);
