# ./run_canon_fw.sh 70D -s -S & arm-none-eabi-gdb -x 70D/patches.gdb -ex quit
# Only patches required for emulation

source patch-header.gdb

# patch sio_send_retry (send retrying...)
set *(int*)0xFF33A570 = 0xe3a00000
set *(int*)0xFF33A574 = 0xe12fff1e

source patch-footer.gdb
