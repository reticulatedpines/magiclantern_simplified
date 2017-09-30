# ./run_canon_fw.sh 5D3 -d debugmsg
# ./run_canon_fw.sh 5D3 -d debugmsg -s -S & arm-none-eabi-gdb -x 5D3/debugmsg.gdb
# unless otherwise specified, these are valid for both 1.1.3 and 1.2.3

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment this:
#symbol-file ../magic-lantern/platform/5D3.113/magiclantern

macro define CURRENT_TASK 0x23E14
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x5b90
# DebugMsg_log

b *0x8b10
task_create_log

b *0x83B8
register_interrupt_log

# 1.2.3
if *(int*)0xFF136C94 == 0xE92D403E
  b *0xFF13B674
  register_func_log
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

cont
