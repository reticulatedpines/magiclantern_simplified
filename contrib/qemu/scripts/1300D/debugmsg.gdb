# ./run_canon_fw.sh 1300D -d debugmsg
# ./run_canon_fw.sh 1300D -d debugmsg -s -S & arm-none-eabi-gdb -x 1300D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/1300D.110/magiclantern
#symbol-file ../magic-lantern/platform/1300D.110/autoexec
#symbol-file ../magic-lantern/platform/1300D.110/stubs.o

macro define CURRENT_TASK 0x31170
macro define CURRENT_ISR  (MEM(0x31174) ? MEM(0x640) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFE11F394
# DebugMsg_log

b *0x38FC
task_create_log

b *0x3CBC
assert_log

b *0x2E50
register_interrupt_log

b *0xFE1201AC
register_func_log

b *0xFE2BEA5C
CreateStateObject_log

cont
