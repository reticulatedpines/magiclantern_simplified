# ./run_canon_fw.sh 550D -d debugmsg
# ./run_canon_fw.sh 550D -d debugmsg -s -S & arm-none-eabi-gdb -x 550D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/550D.109/magiclantern
#symbol-file ../magic-lantern/platform/550D.109/autoexec
#symbol-file ../magic-lantern/platform/550D.109/stubs.o

macro define CURRENT_TASK 0x1a20
macro define CURRENT_ISR  (MEM(0x670) ? MEM(0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFF0673EC
# DebugMsg_log

b *0xFF06A3F8
task_create_log

b *0xFF1D2944
register_interrupt_log

b *0xFF06809C
register_func_log

b *0xFF1D85E4
CreateStateObject_log

# RTC communication
if 0
  b *0xFF0639B4
  rtc_read_log

  b *0xFF06398C
  rtc_write_log
end

# MPU communication
if 0
  b *0xFF1BB02C
  mpu_send_log

  b *0xFF05A3AC
  mpu_recv_log
end

# properties
if 0
  b *0xFF056E38
  prop_request_change_log

  b *0xFF1B712C
  mpu_analyze_recv_data_log

  b *0xFF1B66D4
  prop_lookup_maybe_log

  b *0xFF1BAAC0
  mpu_prop_lookup_log
end

# semaphores
if 0
  b *0xFF069D10
  create_semaphore_log

  b *0xFF069D6C
  create_semaphore_log

  b *0xFF069E70
  take_semaphore_log

  b *0xFF069F58
  give_semaphore_log
end

# state objects
if 1
  b *0xff1d84f4
  state_transition_log
end

# image processing engine
if 0
  b *0xFF1C4074
  CreateResLockEntry_log

  b *0xFF1C4A34
  LockEngineResources_log

  b *0xFF1C45BC
  LockEngineResources_log

  b *0xFF1C46F8
  UnLockEngineResources_log

  b *0xFF1C48F8
  AsyncLockEngineResources_log
end

cont
