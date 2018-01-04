# ./run_canon_fw.sh 5D4 -d debugmsg
# ./run_canon_fw.sh 5D4 -d debugmsg -s -S & arm-none-eabi-gdb -x 5D4/debugmsg.gdb

source -v debug-logging.gdb
source -v 5D4/patches.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/5D4.104/magiclantern
#symbol-file ../magic-lantern/platform/5D4.104/autoexec
#symbol-file ../magic-lantern/platform/5D4.104/stubs.o

macro define CURRENT_TASK 0x45A4
macro define CURRENT_ISR  (*(int*)0x4580 ? (*(int*)0x4584) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFE426B8C
# DebugMsg_log

b *0x6D8
task_create_log

b *0x38
register_interrupt_log

cont
