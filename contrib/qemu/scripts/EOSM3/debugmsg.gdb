# ./run_canon_fw.sh EOSM3 -d debugmsg
# ./run_canon_fw.sh EOSM3 -d debugmsg -s -S & arm-none-eabi-gdb -x EOSM3/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x803C
macro define CURRENT_ISR  (MEM(0x8160) ? MEM(0x8164) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFC37AF70
# DebugMsg_log

b *0xFC361A32
DebugMsg1_log

# used for SDIO
b *0x10F4024
DebugMsg1_log

b *0x10E1000
assert_log

b *0xBFE14A30
task_create_log

b *0xBFE14998
msleep_log

b *0xBFE14EF4
register_interrupt_log

b *0xFC327C18
register_func_log

# semaphores
if 0
  # create binary semaphore
  b *0xBFE1530C
  create_semaphore_log

  # create counting semaphore
  b *0xBFE15358
  create_semaphore_log

  b *0xBFE15390
  delete_semaphore_log

  b *0xBFE15400
  take_semaphore_log

  b *0xBFE15478
  give_semaphore_log
end

# message queues
if 0
  b *0xBFE15054
  create_msg_queue_log

  b *0xBFE151A6
  receive_msg_queue_log

  b *0xBFE1511A
  try_receive_msg_queue_log

  b *0xBFE151F0
  post_msg_queue_log

  b *0xBFE1526E
  post_msg_queue_log
end

b *0xFC130FE4
commands
  silent
  print_current_location
  KRED
  printf "shutdown!!!\n"
  KRESET
  c
end

# some weird puts that seems to be supposed to process 13 chars at a time
b *0x010F3D14
commands
  silent
  print_current_location
  KRED
  set $tmp = *(char*)($r2+13)
  set *(char*)($r2+13) = 0
  printf "%s\n",$r2
  set *(char*)($r2+13) = $tmp
  KRESET
  c
end

cont
