# ./run_canon_fw.sh 60D -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb

source -v debug-logging.gdb

set $CURRENT_TASK = 0x1a2c

b *0xFF06B8DC
DebugMsg_log

b *0xFF06EABC
task_create_log

b *0xFF1BF26C
mpu_send_log

b *0xFF05DFDC
mpu_recv_log

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


cont
