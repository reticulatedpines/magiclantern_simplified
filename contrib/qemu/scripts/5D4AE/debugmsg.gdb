# ./run_canon_fw.sh 5D4AE -d debugmsg
# ./run_canon_fw.sh 5D4AE -d debugmsg -s -S & arm-none-eabi-gdb -x 5D4AE/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x44F4
macro define CURRENT_ISR  (MEM(0x44D0) ? MEM(0x44D4) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x268
# DebugMsg_log

b *0x1B34
task_create_log

b *0x1774
register_interrupt_log

b *0xFE0AD6B0
register_func_log

b *0x1DFE
CreateStateObject_log

cont
