# ./run_canon_fw.sh 7D -s -S & arm-none-eabi-gdb -x 7D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1a1c
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

b *0xFF0776AC
DebugMsg_log

b *0xFF07BEAC
task_create_log

cont
