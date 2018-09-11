# ./run_canon_fw.sh 50D -d debugmsg
# ./run_canon_fw.sh 50D -d debugmsg -s -S & arm-none-eabi-gdb -x 50D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/50D.109/magiclantern
#symbol-file ../magic-lantern/platform/50D.109/autoexec
#symbol-file ../magic-lantern/platform/50D.109/stubs.o

macro define CURRENT_TASK 0x1A70
macro define CURRENT_ISR  (MEM(0x664) ? MEM(0x668) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFF863B10
# DebugMsg_log

b *0xFF866EBC
task_create_log

b *0xFF98AB74
register_interrupt_log

b *0xFF8647B0
register_func_log

b *0xFF9900D8
CreateStateObject_log

cont
