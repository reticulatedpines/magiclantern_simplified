# ./run_canon_fw.sh 70D -s -S & arm-none-eabi-gdb -x 70D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

b *0xFF13EA48
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x7B63C)

source patch-footer.gdb
