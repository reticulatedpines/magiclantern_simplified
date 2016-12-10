# ./run_canon_fw.sh 1000D -s -S & arm-none-eabi-gdb -x 1000D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x352C0
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  0

b *0xFFD0D5F4
DebugMsg_log

b *0xFFD1B828
assert_log

b *0xFFD05A54
task_create_log

b *0xFFD05708
msleep_log

b *0xFFD03BB8
register_interrupt_log

b *0xFFCFF864
register_func_log

cont
