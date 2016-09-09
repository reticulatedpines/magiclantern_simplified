# ./run_canon_fw.sh 760D -s -S & arm-none-eabi-gdb -x 760D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x44F4
macro define CURRENT_ISR  (*(int*)0x44D0 ? (*(int*)0x44D4) : 0)

b *0x268
DebugMsg_log

b *0x1E44
task_create_log

b *0x1774
register_interrupt_log

# infinite loop (memory regions related?)
set *(int*)0xFE195D1C = 0x4770

cont
