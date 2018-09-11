# ./run_canon_fw.sh 60D -d debugmsg
# ./run_canon_fw.sh 60D -d debugmsg -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/60D.111/magiclantern
#symbol-file ../magic-lantern/platform/60D.111/autoexec
#symbol-file ../magic-lantern/platform/60D.111/stubs.o

macro define CURRENT_TASK 0x1a2c
macro define CURRENT_ISR  (MEM(0x670) ? MEM(0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFF06B8DC
# DebugMsg_log

b *0xFF06EABC
task_create_log

b *0xFF1D68C0
register_interrupt_log

b *0xFF06C6C8
register_func_log

b *0xFF1DC6CC
CreateStateObject_log

# MPU communication
if 0
  b *0xFF1BF26C
  mpu_send_log

  b *0xFF05DFDC
  mpu_recv_log
end

# properties
if 0
  b *0xFF05A9F0
  prop_request_change_log

  b *0xFF05AFD0
  prop_deliver_log

  b *0xFF1BAEB0
  mpu_analyze_recv_data_log

  b *0xFF1BA444
  prop_lookup_maybe_log

  b *0xFF1BED04
  mpu_prop_lookup_log
end

# image processing engine
if 0
  b *0xFF1C8658
  CreateResLockEntry_log

  b *0xFF1C8B98
  LockEngineResources_log

  b *0xFF1C8CD4
  UnLockEngineResources_log

  b *0xFF1C45A8
  StartEDmac_log

  b *0xFF1C42A8
  SetEDmac_log
end

if 0
  b *0xFF1DC614
  state_transition_log

  b *0xFF05B454
  try_post_event_log
end

cont
