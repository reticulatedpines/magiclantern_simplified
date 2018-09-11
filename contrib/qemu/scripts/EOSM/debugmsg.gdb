# ./run_canon_fw.sh EOSM -d debugmsg
# ./run_canon_fw.sh EOSM -d debugmsg -s -S & arm-none-eabi-gdb -x EOSM/debugmsg.gdb

source -v debug-logging.gdb
source -v EOSM/patches.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/EOSM.202/magiclantern
#symbol-file ../magic-lantern/platform/EOSM.202/autoexec
#symbol-file ../magic-lantern/platform/EOSM.202/stubs.o

macro define CURRENT_TASK 0x3DE78
macro define CURRENT_ISR  (MEM(0x670) ? MEM(0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x40D4
# DebugMsg_log

b *0x1900
assert_log

b *0x7048
task_create_log

b *0x68F0
register_interrupt_log

b *0xFF137B40
register_func_log

b *0x21F78
CreateStateObject_log

if 0
  b *0x6958
  create_semaphore_log

  b *0x69b4
  create_semaphore_log

  b *0x6ab8
  take_semaphore_log

  b *0x6ba4
  give_semaphore_log
end

# properties
if 0
  b *0xFF11FB04
  prop_request_change_log

  b *0xFF31F6F0
  mpu_analyze_recv_data_log

  b *0xFF31CE8C
  prop_lookup_maybe_log

  b *0xFF325994
  mpu_prop_lookup_log
end

continue
