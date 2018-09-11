# ./run_canon_fw.sh 77D -d debugmsg
# ./run_canon_fw.sh 77D -d debugmsg -s -S & arm-none-eabi-gdb -x 77D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/77D.102/magiclantern
#symbol-file ../magic-lantern/platform/77D.102/autoexec
#symbol-file ../magic-lantern/platform/77D.102/stubs.o

macro define CURRENT_TASK 0x1020
macro define CURRENT_ISR  (*(int*)0x1004 ? (*(int*)0x1008) : 0)
macro define NUM_CORES 2

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xDF006E6C
# DebugMsg_log

b *0xDF008CD2
task_create_log

b *0xE05E7858
assert_log

b *0xDF008274
register_interrupt_log

b *0xE04D8B94
register_func_log

b *0xDF00A1E6
CreateStateObject_log

b *0xE01E781E
mpu_send_log

b *0xE058866A
mpu_recv_log

# EstimatedSize
b *0xE01E95EA
commands
  silent
  print_current_location
  printf "EstimatedSize %d\n", $r0
  set $r0 = 0x7D0
  c
end

cont
