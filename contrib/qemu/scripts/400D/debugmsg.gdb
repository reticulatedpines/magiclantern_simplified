# ./run_canon_fw.sh 400D -d debugmsg
# ./run_canon_fw.sh 400D -d debugmsg -s -S & arm-none-eabi-gdb -x 400D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/400D.111/magiclantern
#symbol-file ../magic-lantern/platform/400D.111/autoexec
#symbol-file ../magic-lantern/platform/400D.111/stubs.o

macro define CURRENT_TASK 0x27C20
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  0

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFFB1EC18
# DebugMsg_log

b *0xFFD1B7D8
assert_log

b *0xFFB106CC
task_create_log

b *0xFFB10484
msleep_log

b *0xFFB0CF20
register_interrupt_log

b *0xFFB06568
register_func_log

b *0xFFB21890
CreateStateObject_log

cont
