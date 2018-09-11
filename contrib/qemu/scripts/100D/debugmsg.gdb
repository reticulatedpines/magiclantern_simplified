# ./run_canon_fw.sh 100D -d debugmsg
# ./run_canon_fw.sh 100D -d debugmsg -s -S & arm-none-eabi-gdb -x 100D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/100D.101/magiclantern
#symbol-file ../magic-lantern/platform/100D.101/autoexec
#symbol-file ../magic-lantern/platform/100D.101/stubs.o

macro define CURRENT_TASK 0x652AC
macro define CURRENT_ISR  (MEM(0x652B0) ? MEM(0x64C) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x4A74
# DebugMsg_log

b *0x79E8
task_create_log

b *0x7290
register_interrupt_log

b *0xFF143310
register_func_log

b *0x3A53C
CreateStateObject_log

cont


