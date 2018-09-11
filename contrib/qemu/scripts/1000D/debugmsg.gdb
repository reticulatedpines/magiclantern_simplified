# ./run_canon_fw.sh 1000D -d debugmsg
# ./run_canon_fw.sh 1000D -d debugmsg -s -S & arm-none-eabi-gdb -x 1000D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/1000D.107/magiclantern
#symbol-file ../magic-lantern/platform/1000D.107/autoexec
#symbol-file ../magic-lantern/platform/1000D.107/stubs.o

macro define CURRENT_TASK 0x352C0
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  0

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFFD0D5F4
# DebugMsg_log

b *0xFFD1B828
assert_log

b *0xFFD05A54
task_create_log

b *0xFFD05708
msleep_log

b *0xFFD03BB8
register_interrupt_log

b *0xFFCFF864
register_func_log

b *0xFFD10250
CreateStateObject_log

cont
