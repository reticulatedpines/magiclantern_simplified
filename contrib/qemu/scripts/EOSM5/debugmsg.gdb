# ./run_canon_fw.sh EOSM5 -s -S & arm-none-eabi-gdb -x EOSM5/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x1020
macro define CURRENT_ISR 0
macro define NUM_CORES 2

b *0xDFFC93A2
task_create_log

b *0xDFFCCAFC
register_interrupt_log

b *0xE0355008
register_func_log

# infinite loop, not sure what it does
b *0xE0008266
commands
  silent
  set $r1 = 1
  c
end

b *0xE0008760          
commands
  silent
  print_current_location
  KYLW
  printf "set_int_handler(%x, %x, %x)\n", $r2, $r0, $r1
  KRESET
  c
end

b *0xE0008684
commands
  silent
  print_current_location
  KYLW
  printf "enable_interrupt_1(%x)\n", $r0
  KRESET
  c
end

b *0xE0028410
commands
  silent
  print_current_location
  KYLW
  printf "enable_interrupt_2(%x)\n", $r0
  KRESET
  c
end

b *0xE0008DA6
commands
  silent
  print_current_location
  KRED
  printf "dryos_panic(%x, %x)\n", $r0, $r1
  KRESET
  c
end

cont
