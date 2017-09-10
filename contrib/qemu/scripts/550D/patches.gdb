# ./run_canon_fw.sh 550D -s -S & arm-none-eabi-gdb -x 550D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

b *0xFF0638FC
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x26C4)

source patch-footer.gdb
