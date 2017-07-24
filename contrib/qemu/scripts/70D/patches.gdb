# ./run_canon_fw.sh 70D -s -S & arm-none-eabi-gdb -x 70D/patches.gdb
# tested on 70D 111A
# Only patches required for emulation
# fixme: duplicate code

source -v debug-logging.gdb

macro define CURRENT_TASK 0x7AAC0
macro define CURRENT_ISR  (*(int*)0x648 ? (*(int*)0x64C) >> 2 : 0)

b *0xFF13EA48
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x7B63C)

continue
