# ./run_canon_fw.sh 60D -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb

source -v debug-logging.gdb

set $CURRENT_TASK = 0x1a2c

b *0xFF06B8DC
DebugMsg_log

b *0xFF06EABC
task_create_log

cont
