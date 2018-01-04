# ./run_canon_fw.sh 5D -d debugmsg
# ./run_canon_fw.sh 5D -d debugmsg -s -S & arm-none-eabi-gdb -x 5D/debugmsg.gdb
# fixme: this crashes GDB (Cannot access memory at address 0xe7fddef6)
# but... why it's trying to do that?
# The crash happens after setting up the MMU (or MPU?) at FF810B60,
# where the background region is disabled.

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/5D.111/magiclantern
#symbol-file ../magic-lantern/platform/5D.111/autoexec
#symbol-file ../magic-lantern/platform/5D.111/stubs.o

macro define CURRENT_TASK (*(int*)0x2D2C4 ? *(int*)0x2D2C4 : 0x2D2C4)
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR 0

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFFB20698
# DebugMsg_log

b *0xFFB2DA9C
assert_log

b *0xFFB18B40
task_create_log

b *0xFFB187F4
msleep_log

b *0xFFB16CDC
register_interrupt_log

cont
