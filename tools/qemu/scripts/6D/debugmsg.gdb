# ./run_canon_fw.sh 6D -d debugmsg
# ./run_canon_fw.sh 6D -d debugmsg -s -S & arm-none-eabi-gdb -x 6D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/6D.116/magiclantern
#symbol-file ../magic-lantern/platform/6D.116/autoexec
#symbol-file ../magic-lantern/platform/6D.116/stubs.o

macro define CURRENT_TASK 0x74C28
macro define CURRENT_ISR  (MEM(0x648) ? MEM(0x64C) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x67c8
# DebugMsg_log

b *0x973c
task_create_log

b *0x8FE4
register_interrupt_log

b *0xFF146760
register_func_log

b *0x39BA0
CreateStateObject_log

# properties
if 0
  b *0xFF12FD6C
  prop_request_change_log

  b *0xFF30FB5C
  mpu_analyze_recv_data_log

  b *0xFF30D268
  prop_lookup_maybe_log

  b *0xFF315E00
  mpu_prop_lookup_log
end

cont
