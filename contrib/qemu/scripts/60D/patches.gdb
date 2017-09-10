# ./run_canon_fw.sh 60D -s -S & arm-none-eabi-gdb -x 60D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

b *0xFF067EEC
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x27B0)

source patch-footer.gdb
