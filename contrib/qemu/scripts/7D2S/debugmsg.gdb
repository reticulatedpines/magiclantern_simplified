# ./run_canon_fw.sh 7D2S -d debugmsg
# ./run_canon_fw.sh 7D2S -d debugmsg -s -S & arm-none-eabi-gdb -x 7D2S/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x44EC
macro define CURRENT_ISR  (*(int*)0x44C8 ? (*(int*)0x44CC) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x236
# DebugMsg_log

b *0x1AB0
task_create_log

b *0x16D8
register_interrupt_log

cont
