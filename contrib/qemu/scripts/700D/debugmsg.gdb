# ./run_canon_fw.sh 700D -d debugmsg
# ./run_canon_fw.sh 700D -d debugmsg -s -S & arm-none-eabi-gdb -x 700D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x233DC
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x395C
# DebugMsg_log

b *0x1900
assert_log

b *0x6868
task_create_log

cont
