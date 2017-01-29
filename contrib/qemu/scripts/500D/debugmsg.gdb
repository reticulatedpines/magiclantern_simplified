# ./run_canon_fw.sh 500D -s -S & arm-none-eabi-gdb -x 500D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1A74
macro define CURRENT_ISR  (*(int*)0x664 ? (*(int*)0x668) >> 2 : 0)

b *0xFF066A98
DebugMsg_log

b *0xFF069E2C
task_create_log

b *0xFF18A884
mpu_send_log

b *0xFF05C1F0
mpu_recv_log

b *0xFF064520
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x2BC4)


cont