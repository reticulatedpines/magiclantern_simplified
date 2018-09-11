# ./run_canon_fw.sh 40D -d debugmsg
# ./run_canon_fw.sh 40D -d debugmsg -s -S & arm-none-eabi-gdb -x 40D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/40D.111/magiclantern
#symbol-file ../magic-lantern/platform/40D.111/autoexec
#symbol-file ../magic-lantern/platform/40D.111/stubs.o

macro define CURRENT_TASK 0x22E00
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  0

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFFD4C1EC
# DebugMsg_log

b *0xFFD5A014
assert_log

b *0xFFD4464C
task_create_log

b *0xFFD44300
msleep_log

b *0xFFD427B0
register_interrupt_log

b *0xFFD3E45C
register_func_log

b *0xFFD4EE48
CreateStateObject_log

# MPU communication
if 0
  b *0xFFC8E2D0
  mpu_send_log

  b *0xFFC8D660
  mpu_recv_log
end

cont
