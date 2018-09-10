# ./run_canon_fw.sh 5D4 -d debugmsg
# ./run_canon_fw.sh 5D4 -d debugmsg -s -S & arm-none-eabi-gdb -x 5D4/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/5D4.112/magiclantern
#symbol-file ../magic-lantern/platform/5D4.112/autoexec
#symbol-file ../magic-lantern/platform/5D4.112/stubs.o

macro define CURRENT_TASK 0x45A4
macro define CURRENT_ISR  (MEM(0x4580) ? MEM(0x4584) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xC14
# DebugMsg_log

b *0xFE35EFC8
assert_log

b *0x6D8
task_create_log

b *0x38
register_interrupt_log

b *0xFE48CE74
register_func_log

b *0x80001FC0
create_semaphore_n3_log

b *0x2122
CreateStateObject_log

cont
