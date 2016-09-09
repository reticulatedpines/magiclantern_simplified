# ./run_canon_fw.sh 750D -s -S & arm-none-eabi-gdb -x 750D/debugmsg.gdb

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

# infinite loop (not sure why)
set *(int*)0xFE175734 = 0x4770

cont
