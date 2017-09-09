# ./run_canon_fw.sh 60D -d debugmsg
# ./run_canon_fw.sh 60D -d debugmsg -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment this:
#symbol-file ../magic-lantern/platform/60D.111/magiclantern

macro define CURRENT_TASK 0x1a2c
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFF06B8DC
# DebugMsg_log

b *0xFF06EABC
task_create_log

b *0xFF1BF26C
mpu_send_log

b *0xFF05DFDC
mpu_recv_log

b *0xFF1C8658
CreateResLockEntry_log

b *0xFF1C8B98
LockEngineResources_log

b *0xFF1C8CD4
UnLockEngineResources_log

b *0xFF1C45A8
StartEDmac_log

b *0xFF1C42A8
SetEDmac_log

if 0
b *0xFF1BF0FC
commands
  silent
  printf "*** SIO3_ISR enter\n"
  c
end

b *0xFF1BF190
commands
  silent
  printf "*** SIO3_ISR exit\n"
  c
end

b *0xFF1BF06C
commands
  silent
  printf "*** MREQ_ISR enter\n"
  c
end

b *0xFF1BF0F8
commands
  silent
  printf "*** MREQ_ISR exit\n"
  c
end
end

cont
