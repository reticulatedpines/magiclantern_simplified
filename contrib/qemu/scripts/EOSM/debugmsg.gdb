# ./run_canon_fw.sh EOSM -s -S & arm-none-eabi-gdb -x EOSM/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x3DE78
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

b *0x40D4
DebugMsg_log

b *0x1900
assert_log

b *0x7048
task_create_log

cont
