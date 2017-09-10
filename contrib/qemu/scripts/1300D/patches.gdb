# ./run_canon_fw.sh 1300D -s -S & arm-none-eabi-gdb -x 1300D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

# patch JPCORE (assert)
set *(int*)0xFE4244FC = 0xe12fff1e

source patch-footer.gdb
