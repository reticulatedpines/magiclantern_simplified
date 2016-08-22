# ./run_canon_fw.sh 70D -s -S & arm-none-eabi-gdb -x 70D/debugmsg.gdb
# tested on 70D 111A

source -v debug-logging.gdb

macro define CURRENT_TASK 0x233DC
macro define CURRENT_ISR  (*(int*)0x928 ? (*(int*)0x92C) >> 2 : 0)

b *0x395C
DebugMsg_log

b *0x1900
assert_log

b *0x6868
task_create_log

cont
