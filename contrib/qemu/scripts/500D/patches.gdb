# ./run_canon_fw.sh 500D -s -S & arm-none-eabi-gdb -x 500D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

b *0xFF064520
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x2BC4)

source patch-footer.gdb
