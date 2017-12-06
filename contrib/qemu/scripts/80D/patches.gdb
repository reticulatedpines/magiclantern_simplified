# ./run_canon_fw.sh 80D -s -S & arm-none-eabi-gdb -x 80D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

source patch-footer.gdb
