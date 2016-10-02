# ./run_canon_fw.sh 5D4 -s -S & arm-none-eabi-gdb -x 5D4/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x45A4
macro define CURRENT_ISR  (*(int*)0x4580 ? (*(int*)0x4584) : 0)

b *0xFE426B1C
DebugMsg_log

b *0x6D8
task_create_log

b *0x38
register_interrupt_log

# infinite loop (memory regions related?)
set *(int*)0xFE28AAB0 = 0x4770

# infinite loop (not sure why)
set *(int*)0xFE28AA2C = 0x4770

# infinite loop (SD clock calibration?)
set *(int*)0xFE3D6522 = 0x4770

cont
