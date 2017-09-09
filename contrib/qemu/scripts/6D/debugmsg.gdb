# ./run_canon_fw.sh 6D -d debugmsg
# ./run_canon_fw.sh 6D -d debugmsg -s -S & arm-none-eabi-gdb -x 6D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment this:
#symbol-file ../magic-lantern/platform/6D.116/magiclantern

macro define CURRENT_TASK 0x74C28
macro define CURRENT_ISR  (*(int*)0x648 ? (*(int*)0x64C) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x67c8
# DebugMsg_log

b *0x973c
task_create_log

cont
