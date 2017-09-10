# ./run_canon_fw.sh 500D -d debugmsg
# ./run_canon_fw.sh 500D -d debugmsg -s -S & arm-none-eabi-gdb -x 500D/debugmsg.gdb

source -v debug-logging.gdb
source -v 500D/patches.gdb

# To get debugging symbols from Magic Lantern, uncomment this:
#symbol-file ../magic-lantern/platform/500D.111/magiclantern

macro define CURRENT_TASK 0x1A74
macro define CURRENT_ISR  (*(int*)0x664 ? (*(int*)0x668) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFF066A98
# DebugMsg_log

b *0xFF069E2C
task_create_log

# MPU communication
if 0
  b *0xFF18A884
  mpu_send_log

  b *0xFF05C1F0
  mpu_recv_log
end

cont
