# ./run_canon_fw.sh 5D2 -s -S & arm-none-eabi-gdb -x 5D2/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1A24
macro define CURRENT_ISR  (*(int*)0x664 ? (*(int*)0x668) >> 2 : 0)

b *0xFF86AF64
DebugMsg_log

b *0xFF86E2E4
task_create_log

b *0xFF9B3CB4
register_interrupt_log

cont
