# ./run_canon_fw.sh 100D -s -S & arm-none-eabi-gdb -x 100D/debugmsg.gdb

source -v debug-logging.gdb

# fixme
macro define CURRENT_TASK ...
macro define CURRENT_ISR  ...

b *0x4A74
DebugMsg_log

cont


