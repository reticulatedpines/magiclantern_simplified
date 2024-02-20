# ./run_canon_fw.sh 70D -d debugmsg
# ./run_canon_fw.sh 70D -d debugmsg -s -S & arm-none-eabi-gdb -x 70D/debugmsg.gdb
# tested on 70D 112

source -v debug-logging.gdb
source -v 70D/patches.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/70D.112/magiclantern
#symbol-file ../magic-lantern/platform/70D.112/autoexec
#symbol-file ../magic-lantern/platform/70D.112/stubs.o

macro define CURRENT_TASK 0x7AAC0
macro define CURRENT_ISR  (MEM(0x648) ? MEM(0x64C) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x6904
# DebugMsg_log

b *0x1900
assert_log

b *0x98CC
task_create_log

b *0x9174
register_interrupt_log

b *0xFF1442C0
register_func_log

b *0x3D970
CreateStateObject_log

# MPU communication
if 0
  b *0x396bc
  mpu_send_log

  b *0x5ed0
  mpu_recv_log
end

# properties
if 0
  b *0xFF12AB14
  prop_request_change_log

  b *0xFF31E250
  mpu_analyze_recv_data_log

  b *0xFF31B408
  prop_lookup_maybe_log

  b *0xFF3247B8
  mpu_prop_lookup_log
end

# message queues (incomplete)
if 0
  b *0x3dd24
  try_post_event_log
end

# timers
if 0
  b *0xE8AC
  SetTimerAfter_log

  b *0x7F94
  SetHPTimerAfterNow_log

  b *0x8094
  SetHPTimerNextTick_log

  b *0xEAAC
  CancelTimer_log
end

# rename one of the two Startup tasks
b *0xff0c3314
commands
    silent
    set $r0 = 0xFF0C2C80
    c
end

cont
