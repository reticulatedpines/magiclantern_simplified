# ./run_canon_fw.sh 400D -s -S & arm-none-eabi-gdb -x 400D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x27C20
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  0

b *0xFFB1EC18
DebugMsg_log

b *0xFFD1B7D8
assert_log

b *0xFFB106CC
task_create_log

b *0xFFB10484
msleep_log

b *0xFFB0CF20
register_interrupt_log

cont
