# ./run_canon_fw.sh 80D -s -S & arm-none-eabi-gdb -x 80D/patches.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x44F4
macro define CURRENT_ISR  (*(int*)0x44D0 ? (*(int*)0x44D4) : 0)

# infinite loop (memory regions related?)
set *(int*)0xFE237EB0 = 0x4770

cont
