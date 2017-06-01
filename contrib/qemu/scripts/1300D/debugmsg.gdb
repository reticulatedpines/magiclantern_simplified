# ./run_canon_fw.sh 1300D -s -S & arm-none-eabi-gdb -x 1300D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x31170
macro define CURRENT_ISR  (*(int*)0x31174 ? (*(int*)0x640) >> 2 : 0)

b *0xFE11F394
DebugMsg_log

b *0x38FC
task_create_log

b *0x3CBC
assert_log

# patch JPCORE (assert)
set *(int*)0xFE4244FC = 0xe12fff1e

cont
