# ./run_canon_fw.sh 7D2M -s -S & arm-none-eabi-gdb -x 7D2/debugmsg-m.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x28568
macro define CURRENT_ISR  (*(int*)0x28544 ? (*(int*)0x28548) : 0)

b *0x236
DebugMsg_log

b *0x1CCC
task_create_log

b *0x1C1C
msleep_log

# b *0x1926
# take_semaphore_log

# b *0x199e
# give_semaphore_log

b *0x16D8
register_interrupt_log

b *0xFE0A0D66
commands
  silent
  print_current_location
  printf "interrupt handler %08x(0x%x)\n", $r1, $r0
  c
end

cont
