# ./run_canon_fw.sh EOSM2 -d debugmsg
# ./run_canon_fw.sh EOSM2 -d debugmsg -s -S & arm-none-eabi-gdb -x EOSM2/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment this:
#symbol-file ../magic-lantern/platform/EOSM2.102/magiclantern

macro define CURRENT_TASK 0x8FBCC
macro define CURRENT_ISR  (*(int*)0x648 ? (*(int*)0x64C) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x4398
# DebugMsg_log

b *0x1900
assert_log

b *0x7360
task_create_log

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

# skip SerialFlash version check
set *(int*)0xFF0C4278 = 0xe3a00000

# break infinite loop at Wait LeoLens Complete
b *0xFF0C5144
commands
  printf "Patching LeoLens (infinite loop)\n"
  set *(int*)($r4 + 0x28) = 1
  c
end

continue
