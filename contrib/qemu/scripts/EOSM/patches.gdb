# ./run_canon_fw.sh EOSM -s -S & arm-none-eabi-gdb -x EOSM/patches.gdb -ex quit
# Only patches required for emulation

source patch-header.gdb

# patch DL to avoid DL ERROR messages
set *(int*)0xFF1BE4AC = 0xe3a00015

# patch localI2C_Write to always return 1 (success)
set *(int*)0xFF3460C4 = 0xe3a00001
set *(int*)0xFF3460C8 = 0xe12fff1e

source patch-footer.gdb
