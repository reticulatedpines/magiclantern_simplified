# ./run_canon_fw.sh 600D -s -S & arm-none-eabi-gdb -x 600D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

b *0xFF069E6C
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x2744)

source patch-footer.gdb
