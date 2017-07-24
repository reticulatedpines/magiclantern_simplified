# ./run_canon_fw.sh 600D -s -S & arm-none-eabi-gdb -x 600D/patches.gdb
# Only patches required for emulation
# fixme: duplicate code

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1a2c
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

b *0xFF069E6C
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x2744)

cont
