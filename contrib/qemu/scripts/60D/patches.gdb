# ./run_canon_fw.sh 60D -s -S & arm-none-eabi-gdb -x 60D/patches.gdb
# Only patches required for emulation
# fixme: duplicate code

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1a2c
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

b *0xFF067EEC
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x27B0)

cont
