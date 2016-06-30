# ./run_canon_fw.sh 600D -s -S & arm-none-eabi-gdb -x 600D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1a2c
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

b *0xFF06E398
DebugMsg_log

b *0xFF071578
task_create_log

b *0xFF1DB524
mpu_send_log

b *0xFF05ED84
mpu_recv_log

b *0xFF1F5944
create_msg_queue_log

b *0xFF1F5C40
post_msg_queue_log

b *0xFF1F5B9C
post_msg_queue_log

b *0xFF1F5A54
try_receive_msg_queue_log

b *0xFF1F5B0C
receive_msg_queue_log

b *0xFF069E6C
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x2744)

cont
