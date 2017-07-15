# ./run_canon_fw.sh 100D -d debugmsg
# ./run_canon_fw.sh 100D -d debugmsg -s -S & arm-none-eabi-gdb -x 100D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x652AC
macro define CURRENT_ISR  (*(int*)0x652B0 ? (*(int*)0x64C) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x4A74
# DebugMsg_log

b *0x79E8
task_create_log

cont


