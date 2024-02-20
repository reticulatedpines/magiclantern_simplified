# ./run_canon_fw.sh EOSM10 -s -S & arm-none-eabi-gdb -x EOSM10/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x803C
macro define CURRENT_ISR  (MEM(0x8160) ? MEM(0x8164) : 0)

b *0xBFE14A40
task_create_log

b *0xBFE1496C
msleep_log

b *0x10E1008
assert_log

b *0xBFE14F04
register_interrupt_log

b *0xFC33E25C
register_func_log

continue
