# ./run_canon_fw.sh 5D3 -s -S & arm-none-eabi-gdb -x 5D3/patches.gdb
# unless otherwise specified, these are valid for both 1.1.3 and 1.2.3

source -v debug-logging.gdb

macro define CURRENT_TASK 0x23E14
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

# 1.1.3
if *(int*)0xFF136BD0 == 0xE92D403E
  b *0xFF136BD0
  load_default_date_time_log
end

# 1.2.3
if *(int*)0xFF136C94 == 0xE92D403E
  b *0xFF136C94
  load_default_date_time_log
end

# both 1.1.3 and 1.2.3
macro define RTC_VALID_FLAG (*(int*)0x249D8)

cont
