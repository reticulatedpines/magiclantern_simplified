# ./run_canon_fw.sh 5D3 -d debugmsg
# ./run_canon_fw.sh 5D3 -d debugmsg -s -S & arm-none-eabi-gdb -x 5D3/debugmsg.gdb
# unless otherwise specified, these are valid for both 1.1.3 and 1.2.3

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/5D3.113/magiclantern
#symbol-file ../magic-lantern/platform/5D3.113/autoexec
#symbol-file ../magic-lantern/platform/5D3.113/stubs.o

macro define CURRENT_TASK 0x23E14
macro define CURRENT_ISR  (MEM(0x670) ? MEM(0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x5b90
# DebugMsg_log

b *0x8b10
task_create_log

b *0x179A0
CreateStateObject_log

b *0x83B8
register_interrupt_log

# 1.2.3
if MEM(0xFF136C94) == 0xE92D403E
  b *0xFF13B674
  register_func_log
end

# 1.1.3
if MEM(0xFF6AB08C) == 0xE92D4FFE
  b *0xFF13B630
  register_func_log

  b *0xFF1425F8
  register_cmd_log
end

# semaphores
if 0
  b *0x8420
  create_semaphore_log

  b *0x847C
  create_semaphore_log

  b *0x84C8
  delete_semaphore_log

  b *0x8580
  take_semaphore_log

  b *0x866C
  give_semaphore_log
end

# MPU communication - 1.1.3
if 0
  b *0xFF2E42E4
  mpu_send_log

  b *0xFF122B5C
  mpu_recv_log
end

# properties - 1.1.3
if 0
  b *0xFF123600
  prop_request_change_log

  b *0xFF2E6B6C
  mpu_analyze_recv_data_log

  b *0xFF2E4914
  prop_lookup_maybe_log

  b *0xFF2EC5CC
  mpu_prop_lookup_log
end

# properties - 1.2.3 (some are different!)
if 0
  b *0xFF123210
  prop_request_change_log

  b *0xFF2EAED0
  mpu_analyze_recv_data_log

  b *0xFF2E8C78
  prop_lookup_maybe_log
end

# 1.1.3
if 1
  b *0xFF3F4F54
  ptp_register_handler_log
end

cont
