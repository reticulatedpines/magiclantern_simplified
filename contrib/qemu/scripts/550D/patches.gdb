# ./run_canon_fw.sh 550D -s -S & arm-none-eabi-gdb -x 550D/patches.gdb
# Only patches required for emulation
# fixme: duplicate code

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1a20
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

b *0xFF0638FC
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x26C4)

cont
