# ./run_canon_fw.sh 1100D -d debugmsg
# ./run_canon_fw.sh 1100D -d debugmsg -s -S & arm-none-eabi-gdb -x 1100D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/1100D.105/magiclantern
#symbol-file ../magic-lantern/platform/1100D.105/autoexec
#symbol-file ../magic-lantern/platform/1100D.105/stubs.o

macro define CURRENT_TASK 0x1a2c
macro define CURRENT_ISR  (MEM(0x670) ? MEM(0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFF06C914
# DebugMsg_log

b *0xFF06FAF4
task_create_log

b *0xFF1E8630
register_interrupt_log

b *0xFF06D700
register_func_log

b *0xFF1EE180
CreateStateObject_log

b *0xFF1D34CC
SetEDmac_log

continue
