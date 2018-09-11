# ./run_canon_fw.sh A1100 -s -S & arm-none-eabi-gdb -x A1100/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x195C
macro define CURRENT_ISR  (MEM(0x670) ? MEM(0x674) : 0)

b *0xFFC0B284
assert_log

b *0xFFC0AFAC
task_create_log

b *0xFFC0AF44
register_interrupt_log

b *0xFFC55494
register_func_log

cont
