# ./run_canon_fw.sh 450D -s -S & arm-none-eabi-gdb -x 450D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x355C0
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  0

b *0xFFD07654
DebugMsg_log

b *0xFFD15914
assert_log

b *0xFFCFFAB4
task_create_log

b *0xFFCFF768
msleep_log

b *0xFFCFDC18
register_interrupt_log

cont
