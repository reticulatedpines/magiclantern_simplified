# ./run_canon_fw.sh 750D -d debugmsg
# ./run_canon_fw.sh 750D -d debugmsg -s -S & arm-none-eabi-gdb -x 750D/debugmsg.gdb

source -v debug-logging.gdb
source -v 750D/patches.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/750D.100/magiclantern
#symbol-file ../magic-lantern/platform/750D.100/autoexec
#symbol-file ../magic-lantern/platform/750D.100/stubs.o

macro define CURRENT_TASK 0x44F4
macro define CURRENT_ISR  (*(int*)0x44D0 ? (*(int*)0x44D4) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x268
# DebugMsg_log

b *0x1E44
task_create_log

b *0xFE52F980
assert_log

b *0x1774
register_interrupt_log

b *0xFE445CC0
register_func_log

b *0x211A
CreateStateObject_log

cont
