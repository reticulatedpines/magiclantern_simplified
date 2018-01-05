# ./run_canon_fw.sh 7D2M -s -S & arm-none-eabi-gdb -x 7D2M/patches.gdb -ex quit
# Only patches required for emulation

source patch-header.gdb

# enable TIO (required to print DryOS version)
set *(int*)0xFEC4DCBC = 1

# PROPAD_CreateFROMPropertyHandle
set *(int*)0xFE102B5A = 0x4770

source patch-footer.gdb
