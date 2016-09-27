# ./run_canon_fw.sh 1000D -s -S & arm-none-eabi-gdb -x 1000D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK (*(int*)0x352C0 ? *(int*)0x352C0 : 0x352C0)
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  0

b *0xFFD0D5A4
DebugMsg_log

b *0xFFD1B7D8
assert_log

b *0xFFD05A04
task_create_log

b *0xFFD056B8
msleep_log

b *0xFFD03B68
register_interrupt_log

cont
