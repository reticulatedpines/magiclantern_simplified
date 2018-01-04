# ./run_canon_fw.sh EOSM2 -s -S & arm-none-eabi-gdb -x EOSM2/patches.gdb -ex quit
# Only patches required for emulation

source patch-header.gdb

# patch DL to avoid DL ERROR messages
set *(int*)0xFF156348 = 0xe3a00015

# patch localI2C_Write to always return 1 (success)
set *(int*)0xFF356E24 = 0xe3a00001
set *(int*)0xFF356E28 = 0xe12fff1e

# patch TouchMgr to avoid loop timeout
# ExecuteSIO8
set *(int*)0xFF344F40 = 0xe1a0000b
# ExecuteSIO32
set *(int*)0xFF345148 = 0xe1a0000b

# break infinite loop at Wait LeoLens Complete
b *0xFF0C5144
commands
  printf "Patching LeoLens (infinite loop)\n"
  set *(int*)($r4 + 0x28) = 1
  c
end

source patch-footer.gdb
