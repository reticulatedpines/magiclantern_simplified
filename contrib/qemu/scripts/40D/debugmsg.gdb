# ./run_canon_fw.sh 40D -s -S & arm-none-eabi-gdb -x 40D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x22E00
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  0

b *0xFFD4C1EC
DebugMsg_log

b *0xFFD5A014
assert_log

b *0xFFD4464C
task_create_log

b *0xFFD44300
msleep_log

b *0xFFD427B0
register_interrupt_log

cont
