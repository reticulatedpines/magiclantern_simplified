source -v debug-logging.gdb

macro define CURRENT_TASK 0x1020
macro define CURRENT_ISR  (*(int*)0x1004 ? (*(int*)0x1008) : 0)
macro define NUM_CORES 2

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xDF006E6C
# DebugMsg_log

b *0xDF008CD6
task_create_log

b *0xE05D4B08
assert_log

b *0xDF00A1EA
CreateStateObject_log

b *0xE04BFDD8
register_func_log

b *0xDF008274
register_interrupt_log

b *0xE01C56B6
mpu_send_log

b *0xE05A423A
mpu_recv_log

cont
