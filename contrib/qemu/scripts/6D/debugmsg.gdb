# ./run_canon_fw.sh 6D -s -S & arm-none-eabi-gdb -x 6D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x74C28
macro define CURRENT_ISR  (*(int*)0x648 ? (*(int*)0x64C) >> 2 : 0)

b *0x67c8
DebugMsg_log

b *0x973c
task_create_log

cont
