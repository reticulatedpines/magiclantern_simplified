/* this magic is a BX R3 */
#define FOOTER_MAGIC 0xE12FFF13
#define STR(x) STRx(x)
#define STRx(x) #x

asm(
    /* footer is read by first instructions to check if autoexec.bin was loaded correctly 
       .data is selected after .text by the default linker script, so this will go to the
       end of the file, being a usable footer.
    */
    ".section .data\n"
    
    ".align 5, 0xCE\n"

    /* fill up so there are only two words left */
    ".word   0xCEEEEEEC\n"
    ".word   0xCEEEEEEC\n"
    ".word   0xCEEEEEEC\n"
    ".word   0xCEEEEEEC\n"
    ".word   0xCEEEEEEC\n"
    ".word   0xCEEEEEEC\n"
    
    ".globl autoexec_bin_footer\n"
    "autoexec_bin_footer:\n"
    ".word   "STR(FOOTER_MAGIC)"\n"
    "autoexec_bin_checksum:\n"
    ".word   0xCCCCCCCC\n"
    ".globl autoexec_bin_checksum_end\n"
    "autoexec_bin_checksum_end:\n"
);
