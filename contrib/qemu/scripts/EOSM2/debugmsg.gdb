# ./run_canon_fw.sh EOSM2 -s -S & arm-none-eabi-gdb -x EOSM2/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x8FBCC
macro define CURRENT_ISR  (*(int*)0x648 ? (*(int*)0x64C) >> 2 : 0)

b *0x4398
DebugMsg_log

b *0x7360
task_create_log

# break infinite loop at Wait LeoLens Complete
b *0xFF0C5148
commands
  printf "Patching LeoLens (infinite loop)\n"
  set *(int*)($r4 + 0x28) = 1
  c
end
