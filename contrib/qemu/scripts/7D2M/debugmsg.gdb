# ./run_canon_fw.sh 7D2M -d debugmsg
# ./run_canon_fw.sh 7D2M -d debugmsg -s -S & arm-none-eabi-gdb -x 7D2M/debugmsg.gdb

source -v debug-logging.gdb
source -v 7D2M/patches.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/7D2.104/magiclantern
#symbol-file ../magic-lantern/platform/7D2.104/autoexec
#symbol-file ../magic-lantern/platform/7D2.104/stubs.o

macro define CURRENT_TASK 0x28568
macro define CURRENT_ISR  (MEM(0x28544) ? MEM(0x28548) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x236
# DebugMsg_log

b *0x1CCC
task_create_log

b *0x1C1C
msleep_log

b *0x16D8
register_interrupt_log

b *0xFE109834
register_func_log

b *0x206A
CreateStateObject_log

# semaphores
if 0
  b *0x1830
  create_semaphore_log

  # what's the difference between these two?
  b *0x187C
  create_semaphore_log

  b *0x18B4
  delete_semaphore_log

  b *0x1926
  take_semaphore_log

  b *0x199e
  give_semaphore_log
end

cont
