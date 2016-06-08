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

cont
