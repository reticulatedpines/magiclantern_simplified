# ./run_canon_fw.sh 80D -s -S & arm-none-eabi-gdb -x 80D/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x44F4
macro define CURRENT_ISR  (*(int*)0x44D0 ? (*(int*)0x44D4) : 0)

b *0x800035E0
DebugMsg_log

b *0xFF0
task_create_log

b *0xB60
register_interrupt_log

b *0xFE237C9E
commands
  silent
  print_current_location
  printf "Memory region: start=%08X end=%08X flags=%08X\n", $r0, $r1, $r2
  c
end

# infinite loop (memory regions related?)
set *(int*)0xFE237EB0 = 0x4770

cont
