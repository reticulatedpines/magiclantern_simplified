# ./run_canon_fw.sh 6D2 -d debugmsg
# ./run_canon_fw.sh 6D2 -d debugmsg -s -S & arm-none-eabi-gdb -x 6D2/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/6D2.103/magiclantern
#symbol-file ../magic-lantern/platform/6D2.103/autoexec
#symbol-file ../magic-lantern/platform/6D2.103/stubs.o

macro define CURRENT_TASK 0x1028
macro define CURRENT_ISR  (*(int*)0x100C ? (*(int*)0x1010) : 0)
macro define NUM_CORES 2

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xDF006E6C
# DebugMsg_log

b *0xDF008CE6
task_create_log

b *0xE06170EC
assert_log

b *0xDF008284
register_interrupt_log

b *0xE04E6FF0
register_func_log

b *0xDF00A1FA
CreateStateObject_log

b *0xE01F67AA
mpu_send_log

b *0xE0644786
mpu_recv_log

cont
