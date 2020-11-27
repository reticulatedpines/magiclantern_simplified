# ./run_canon_fw.sh EOSRP -d debugmsg
# ./run_canon_fw.sh EOSRP -d debugmsg -s -S & arm-none-eabi-gdb -x EOSRP/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magiclantern_simplified/platform/EOSRP.160/magiclantern
#symbol-file ../magiclantern_simplified/platform/EOSRP.160/autoexec
#symbol-file ../magiclantern_simplified/platform/EOSRP.160/stubs.o
# !!!

macro define CURRENT_TASK 0x1028
#macro define CURRENT_ISR  (MEM(0x100C) ? MEM(0x1010) : 0) # TODO: What is the MEM macro used for!?
macro define CURRENT_ISR 0x1010
macro define NUM_CORES 2

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
b *0xe05a5f38
DebugMsg_log

b *0xe05da6c2
task_create_log

b *0xe05d53e8
assert_log

b *0xe05a558c
register_interrupt_log

b *0xe0595bb4
register_func_log

b *0xe0595c2e
call_by_name_log

b *0xe058f992
CreateStateObject_log

b *0xe009b34c
mpu_send_log

# Ghidra disassembly looks broken. Meight be wrong:
b *0xe05da06c 
mpu_recv_log

cont
