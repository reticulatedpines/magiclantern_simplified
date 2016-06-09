# ./run_canon_fw.sh 70D -s -S & arm-none-eabi-gdb -x 70D/debugmsg.gdb
# tested on 70D 111A

source -v debug-logging.gdb

macro define CURRENT_TASK 0x7AAC0
macro define CURRENT_ISR  (*(int*)0x648 ? (*(int*)0x64C) >> 2 : 0)

b *0x6904
DebugMsg_log

b *0x1900
assert_log

b *0x98CC
task_create_log

b *0x9174
register_interrupt_log

b *0x396bc
mpu_send_log

b *0x5ed0
mpu_recv_log

b *0x3dd24
try_post_event_log

b *0xE8AC
SetTimerAfter_log

b *0x7F94
SetHPTimerAfterNow_log

b *0x8094
SetHPTimerNextTick_log

b *0xEAAC
CancelTimer_log

b *0xFF12AF04
commands
  silent
  print_current_location
  printf "prop_register_master(%x,%x,%x,%x,%x,%x,%x,%x)\n", $r0, $r1, $r2, $r3, *(int*)$sp, *(int*)($sp+4), *(int*)($sp+8), *(int*)($sp+12)
  c
end

b *0xFF31B240
commands
  silent
  if *(int*)($r0 + 28) != 0xFFFFFFFF
    print_current_location
    printf "wtf(%x,%x,%x) dwWaitAckID=%x\n", $r0, $r1, $r2, *(int*)($r0 + 28)
  end
  c
end

# rename one of the two Startup tasks
b *0xff0c3314
commands
    silent
    set $r0 = 0xFF0C2C80
    c
end

b *0x5f2c
commands
  silent
  printf "mpu_recv handler: %08x(%x,%x,%x)\n", $r3, $r0, $r1, $r2
  c
end

b *0x5f58
commands
  silent
  printf "mpu_recv handler: %08x(%x,%x)\n", $r2, $r0, $r1
  c
end

b *0x5f70
commands
  silent
  printf "mpu_recv handler: %08x(%x,%x)\n", $r2, $r0, $r1
  c
end

b *0x6008
commands
  silent
  printf "mpu_recv handler: %08x(%x,%x,%x,%x)\n", $r12, $r0, $r1, $r2, $r3
  c
end

b *0x6034
commands
  silent
  printf "mpu_recv handler: %08x(%x,%x,%x,%x)\n", $r12, $r0, $r1, $r2, $r3
  c
end

cont
