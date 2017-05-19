# ./run_canon_fw.sh 1300D -s -S & arm-none-eabi-gdb -x 1300D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x31170
macro define CURRENT_ISR  (*(int*)0x31174 ? (*(int*)0x640) >> 2 : 0)

b *0x38FC
task_create_log

b *0xFE11D6B4
DebugMsg_log

cont
