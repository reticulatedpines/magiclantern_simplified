# ./run_canon_fw.sh 70D -s -S & arm-none-eabi-gdb -x 70D/debugmsg.gdb
# tested on 70D 111A

source -v debug-logging.gdb

macro define CURRENT_TASK 0x7AAC0
macro define CURRENT_ISR  (*(int*)0x648 ? (*(int*)0x64C) >> 2 : 0)

b *0x6904
DebugMsg_log

b *0x98CC
task_create_log

b *0x396bc
mpu_send_log

b *0x5ed0
mpu_recv_log

cont
