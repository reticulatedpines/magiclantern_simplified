# ./run_canon_fw.sh 550D -s -S & arm-none-eabi-gdb -x 550D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1a20
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

b *0xFF0673EC
DebugMsg_log

b *0xFF06A3F8
task_create_log

b *0xFF1BB02C
mpu_send_log

b *0xFF05A3AC
mpu_recv_log

b *0xFF069D10
create_semaphore_log

b *0xFF069D6C
create_semaphore_log

b *0xFF069F58
give_semaphore_log

b *0xFF069E70
take_semaphore_log

b *0xFF06809C
register_func_log

cont
