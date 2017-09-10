# ./run_canon_fw.sh 650D -s -S & arm-none-eabi-gdb -x 650D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

# patch I2C_Write to always return 1 (success)
set *(int*)0xFF341F94 = 0xe3a00001
set *(int*)0xFF341F98 = 0xe12fff1e

source patch-footer.gdb
