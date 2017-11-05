# ./run_canon_fw.sh 5D4 -d debugmsg
# ./run_canon_fw.sh 5D4 -d debugmsg -s -S & arm-none-eabi-gdb -x 5D4/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x45A4
macro define CURRENT_ISR  (*(int*)0x4580 ? (*(int*)0x4584) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFE426B8C
# DebugMsg_log

b *0x6D8
task_create_log

b *0x38
register_interrupt_log

# infinite loop (memory regions related?)
set *(int*)0xFE28AAF0 = 0x4770

# infinite loop (not sure why)
set *(int*)0xFE28AA6C = 0x4770

# infinite loop (SD clock calibration?)
set *(int*)0xFE3D65C2 = 0x4770

cont
