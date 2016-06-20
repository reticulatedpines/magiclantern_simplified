# ./run_canon_fw.sh 1200D -s -S & arm-none-eabi-gdb -x 1200D/debugmsg.gdb

source -v debug-logging.gdb

# identical to 60D (!)
macro define CURRENT_TASK 0x1a2c
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

b *0xFF11F5DC
DebugMsg_log

b *0xFF122824
task_create_log

b *0xFF1220D4
register_interrupt_log

b *0xFF297780
mpu_send_log

b *0xFF10F768
mpu_recv_log

b *FF10D50C
commands
  silent
  printf "PROPAD_GetPropertyData(0x%08X)\n", $r0
  c
end

# rename one of the two Startup tasks
b *0xFF0D8420
commands
    silent
    set $r0 = 0xFF0C203C
    c
end

cont
