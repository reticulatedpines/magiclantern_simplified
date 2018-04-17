# ./run_canon_fw.sh EOSM2 -d debugmsg
# ./run_canon_fw.sh EOSM2 -d debugmsg -s -S & arm-none-eabi-gdb -x EOSM2/debugmsg.gdb

source -v debug-logging.gdb
source -v EOSM2/patches.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/EOSM2.102/magiclantern
#symbol-file ../magic-lantern/platform/EOSM2.103/autoexec
#symbol-file ../magic-lantern/platform/EOSM2.103/stubs.o

macro define CURRENT_TASK 0x8FBCC
macro define CURRENT_ISR  (MEM(0x648) ? MEM(0x64C) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x4398
# DebugMsg_log

b *0x1900
assert_log

b *0x7360
task_create_log

b *0x6C08
register_interrupt_log

b *0xFF146DD8
register_func_log

b *0x36468
CreateStateObject_log

continue
