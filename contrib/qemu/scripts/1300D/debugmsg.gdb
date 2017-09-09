# ./run_canon_fw.sh 1300D -d debugmsg
# ./run_canon_fw.sh 1300D -d debugmsg -s -S & arm-none-eabi-gdb -x 1300D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment this:
#symbol-file ../magic-lantern/platform/1300D.110/magiclantern

macro define CURRENT_TASK 0x31170
macro define CURRENT_ISR  (*(int*)0x31174 ? (*(int*)0x640) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFE11F394
# DebugMsg_log

b *0x38FC
task_create_log

b *0x3CBC
assert_log

# patch JPCORE (assert)
set *(int*)0xFE4244FC = 0xe12fff1e

cont
