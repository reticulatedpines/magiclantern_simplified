# ./run_canon_fw.sh 760D -d debugmsg
# ./run_canon_fw.sh 760D -d debugmsg -s -S & arm-none-eabi-gdb -x 760D/debugmsg.gdb

source -v debug-logging.gdb
source -v 760D/patches.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/760D.100/magiclantern
#symbol-file ../magic-lantern/platform/760D.100/autoexec
#symbol-file ../magic-lantern/platform/760D.100/stubs.o

macro define CURRENT_TASK 0x44F4
macro define CURRENT_ISR  (MEM(0x44D0) ? MEM(0x44D4) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x268
# DebugMsg_log

b *0x1E44
task_create_log

b *0xFE53809C
assert_log

b *0x1774
register_interrupt_log

b *0xFE458C84
register_func_log

b *0x211A
CreateStateObject_log

cont
