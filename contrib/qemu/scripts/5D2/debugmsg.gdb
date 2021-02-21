# ./run_canon_fw.sh 5D2 -d debugmsg
# ./run_canon_fw.sh 5D2 -d debugmsg -s -S & arm-none-eabi-gdb -x 5D2/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/5D2.212/magiclantern
#symbol-file ../magic-lantern/platform/5D2.212/autoexec
#symbol-file ../magic-lantern/platform/5D2.212/stubs.o

macro define CURRENT_TASK 0x1A24
macro define CURRENT_ISR  (MEM(0x664) ? MEM(0x668) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFF86AF64
# DebugMsg_log

b *0xFF86E2E4
task_create_log

b *0xFF9B3CB4
register_interrupt_log

b *0xFF86BC04
register_func_log

b *0xFF9B9250
CreateStateObject_log

cont
