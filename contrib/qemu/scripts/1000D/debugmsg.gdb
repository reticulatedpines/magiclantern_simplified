# ./run_canon_fw.sh 1000D -s -S & arm-none-eabi-gdb -x 1000D/debugmsg.gdb

source -v debug-logging.gdb

//macro define CURRENT_TASK 0x803C
//macro define CURRENT_ISR  (*(int*)0x8160 ? (*(int*)0x8164) : 0)

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
