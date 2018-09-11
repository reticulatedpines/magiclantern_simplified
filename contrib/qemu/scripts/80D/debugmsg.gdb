# ./run_canon_fw.sh 80D -d debugmsg
# ./run_canon_fw.sh 80D -d debugmsg -s -S & arm-none-eabi-gdb -x 80D/debugmsg.gdb

source -v debug-logging.gdb
source -v 80D/patches.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/80D.102/magiclantern
#symbol-file ../magic-lantern/platform/80D.102/autoexec
#symbol-file ../magic-lantern/platform/80D.102/stubs.o

macro define CURRENT_TASK 0x44F4
macro define CURRENT_ISR  (MEM(0x44D0) ? MEM(0x44D4) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x800035E0
# DebugMsg_log

b *0xFF0
task_create_log

b *0xFE547CD4
assert_log

b *0xB60
register_interrupt_log

b *0xFE4841B4
register_func_log

b *0x12AE
CreateStateObject_log

# MPU communication
if 0
  b *0xFE253C0E
  mpu_send_log

  b *0xFE5475CE
  mpu_recv_log
end

if 1
  b *0x1224
  state_transition_log
end

b *0xFE237C9E
commands
  silent
  print_current_location
  printf "Memory region: start=%08X end=%08X flags=%08X\n", $r0, $r1, $r2
  c
end

# semaphores
if 0
  b *0x23E0
  create_semaphore_log

  b *0x242C
  create_semaphore_log

  b *0x24D6
  take_semaphore_log

  b *0x254E
  give_semaphore_log

  b *0x2464
  delete_semaphore_log
end

# message queues
if 0
  b *0x2120
  create_msg_queue_log

  b *0x2334
  post_msg_queue_log

  b *0x21E0
  try_receive_msg_queue_log

  b *0x226C
  receive_msg_queue_log

  b *0x14E6
  try_post_event_log
end

cont
