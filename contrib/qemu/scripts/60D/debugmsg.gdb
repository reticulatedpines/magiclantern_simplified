target remote localhost:1234
set pagination off

define hook-quit
  set confirm off
end

# DryosDebugMsg - QEMU hook
b *0xFF06B8DC
commands
  silent
  set *0xCF999001 = *0xCF999001
  c
end

# task_create
b *0xFF06EABC
commands
  silent
  printf "*** task_create(%s, prio=%x, stack=%x, entry=%x, arg=%x)\n", $r0, $r1, $r2, $r3, *(int*)$sp
  c
end

cont
