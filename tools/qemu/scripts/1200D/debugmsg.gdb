# ./run_canon_fw.sh 1200D -d debugmsg
# ./run_canon_fw.sh 1200D -d debugmsg -s -S & arm-none-eabi-gdb -x 1200D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/1200D.101/magiclantern
#symbol-file ../magic-lantern/platform/1200D.101/autoexec
#symbol-file ../magic-lantern/platform/1200D.101/stubs.o

# identical to 60D (!)
macro define CURRENT_TASK 0x1a2c
macro define CURRENT_ISR  (MEM(0x670) ? MEM(0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFF11F5DC
# DebugMsg_log

b *0xFF122824
task_create_log

b *0xFF1220D4
register_interrupt_log

b *0xFF1203C8
register_func_log

b *0xFF2B968C
CreateStateObject_log

# MPU communication
if 0
  b *0xFF297780
  mpu_send_log

  b *0xFF10F768
  mpu_recv_log
end

# rename one of the two Startup tasks
b *0xFF0D8420
commands
    silent
    set $r0 = 0xFF0C203C
    c
end

cont
