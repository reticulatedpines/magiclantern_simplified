# ./run_canon_fw.sh 5D3eeko -s -S & arm-none-eabi-gdb -x 5D3eeko/debugmsg.gdb
# unless otherwise specified, these are valid for both 1.1.3 and 1.2.3

source -v debug-logging.gdb

macro define CURRENT_TASK 0x40000148
macro define CURRENT_ISR  (*(int*)0x4000014C ? *(int*)0x40000010 : 0)

b *0x1E002D0
assert0_log

b *0x1E46C04
task_create_log

b *0x1E46B74
register_interrupt_log

b *0x1E43E44
printf_log

# enable early UART output
set *(int*)0x4000000C = 1

b *0x1378
commands
  silent
  print_current_location
  KRED
  printf "init_task\n"
  KRESET
  c
end

b *0x1E46C50
commands
  silent
  print_current_location
  KBLU
  printf "task_create(entry=%x, %x, %x, stack=%x, %x, %x, %s)\n", $r0, $r1, $r2, $r3, *(int*)$sp, *(int*)($sp+4), *(int*)($sp+8)
  KRESET
  c
end

b *0x52C
commands
  silent
  print_current_location
  KYLW
  printf "Interrupt: %x(%x)\n", $r1, $r0
  KRESET
  c
end

b *0x568             
commands
  silent
  print_current_location
  KYLW
  printf "set_int_handler(%x, %x, %x)\n", $r2, $r0, $r1
  KRESET
  c
end

b *0x598
commands
  silent
  print_current_location
  KYLW
  printf "clr_int_handler(%x)\n", $r0
  KRESET
  c
end

b *0xD22
commands
  silent
  print_current_location
  KYLW
  printf "disable_interrupt(%x)\n", $r0
  KRESET
  c
end

cont
