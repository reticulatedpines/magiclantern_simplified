# ./run_canon_fw.sh EOSM2 -s -S & arm-none-eabi-gdb -x EOSM2/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x8FBCC
macro define CURRENT_ISR  (*(int*)0x648 ? (*(int*)0x64C) >> 2 : 0)

b *0x4398
DebugMsg_log

b *0x7360
task_create_log

# patch DL to avoid DL ERROR messages
set *(int*)0xFF156348 = 0xe3a00015

# patch localI2C_Write to always return 1 (success)
set *(int*)0xFF356E24 = 0xe3a00001
set *(int*)0xFF356E28 = 0xe12fff1e

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
