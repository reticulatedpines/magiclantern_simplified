# ./run_canon_fw.sh 650D -s -S & arm-none-eabi-gdb -x 650D/patches.gdb
# Only patches required for emulation
# fixme: duplicate code

source -v debug-logging.gdb

macro define CURRENT_TASK 0x233D8
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

# patch I2C_Write to always return 1 (success)
set *(int*)0xFF341F94 = 0xe3a00001
set *(int*)0xFF341F98 = 0xe12fff1e

continue
