# ./run_canon_fw.sh 7D2M -s -S & arm-none-eabi-gdb -x 7D2/debugmsg-m.gdb

source -v debug-logging.gdb

set $CURRENT_TASK = 0x28568

b *0x236
DebugMsg_log

b *0x1CCC
task_create_log

b *0x1C1C
msleep_log

cont
