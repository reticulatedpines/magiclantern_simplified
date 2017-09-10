# ./run_canon_fw.sh 80D -s -S & arm-none-eabi-gdb -x 80D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

# infinite loop (memory regions related?)
set *(int*)0xFE237EB0 = 0x4770

source patch-footer.gdb
