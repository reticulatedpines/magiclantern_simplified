# ./run_canon_fw.sh 5D4 -s -S & arm-none-eabi-gdb -x 5D4/patches.gdb -ex quit
# Only patches required for emulation

source patch-header.gdb

# infinite loop (not sure why)
set *(int*)0xFE28AA6C = 0x4770

# infinite loop (SD clock calibration?)
set *(int*)0xFE3D65C2 = 0x4770

source patch-footer.gdb
