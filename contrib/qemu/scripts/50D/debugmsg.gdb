# ./run_canon_fw.sh 50D -s -S & arm-none-eabi-gdb -x 50D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1A70
macro define CURRENT_ISR  (*(int*)0x664 ? (*(int*)0x668) >> 2 : 0)

b *0xFF863B10
DebugMsg_log

b *0xFF866EBC
task_create_log

cont
