# ./run_canon_fw.sh 200D -d debugmsg
# ./run_canon_fw.sh 200D -d debugmsg -s -S & arm-none-eabi-gdb -x 200D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/200D.101/magiclantern
#symbol-file ../magic-lantern/platform/200D.101/autoexec
#symbol-file ../magic-lantern/platform/200D.101/stubs.o

macro define CURRENT_TASK 0x1028
macro define CURRENT_ISR  (*(int*)0x100C ? (*(int*)0x1010) : 0)
macro define NUM_CORES 2

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xDF006E6C
# DebugMsg_log

b *0xDF008CE6
task_create_log

b *0xE05F1C94
assert_log

b *0xDF008284
register_interrupt_log

b *0xE04BDB14
register_func_log

b *0xDF00A1FA
CreateStateObject_log

b *0xE01C7656
mpu_send_log

b *0xE056314E
mpu_recv_log

cont
