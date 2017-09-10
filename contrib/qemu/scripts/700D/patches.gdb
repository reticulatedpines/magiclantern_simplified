# ./run_canon_fw.sh 700D -s -S & arm-none-eabi-gdb -x 700D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

# patch DL to avoid DL ERROR messages
set *(int*)0xFF64254C = 0xe3a00015

# patch I2C_Write to always return 1 (success)
set *(int*)0xFF344AE4 = 0xe3a00001
set *(int*)0xFF344AE8 = 0xe12fff1e

#b *0xFF132DD0
#load_default_date_time_log
#macro define RTC_VALID_FLAG (*(int*)0x3E9F8)

source patch-footer.gdb
