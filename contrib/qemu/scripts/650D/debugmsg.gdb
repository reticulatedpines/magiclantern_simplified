# ./run_canon_fw.sh 650D -d debugmsg
# ./run_canon_fw.sh 650D -d debugmsg -s -S & arm-none-eabi-gdb -x 650D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/650D.104/magiclantern
#symbol-file ../magic-lantern/platform/650D.104/autoexec
#symbol-file ../magic-lantern/platform/650D.104/stubs.o

macro define CURRENT_TASK 0x233D8
macro define CURRENT_ISR  (MEM(0x670) ? MEM(0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x395C
# DebugMsg_log

b *0x1900
assert_log

b *0x6868
task_create_log

b *0x13344
register_interrupt_log

b *0xFF137F54
register_func_log

b *0x16890
CreateStateObject_log

cont
