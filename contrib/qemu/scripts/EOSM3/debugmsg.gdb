target remote localhost:1234
set pagination off

define hook-quit
  set confirm off
end

define KRED
    printf "%c[1;31m", 0x1B
end

define KCYN
    printf "%c[1;36m", 0x1B
end

define KBLU
    printf "%c[1;34m", 0x1B
end

define KRESET
    printf "%c[0m", 0x1B
end

define print_current_location
  KRESET
  printf "["
  KCYN
  printf "%10s:%08x", ((char***)0x803C)[0][9], $r14
  KRESET
  printf " ] "
end

# DryosDebugMsg - QEMU hook
b *0xFC37AF70
commands
  silent
  print_current_location
  set *0xCF999001 = *0xCF999001
  c
end

# DebugMsg0
b *0xFC361A32
commands
  silent
  print_current_location
  printf "[DebugMsg] (%d) %s\n", $r0, $r1
  c
end

# assert
b *0x10E1000
commands
  silent
  print_current_location
  printf "["
  KRED
  printf "ASSERT"
  KRESET
  printf "] "
  printf "%s at %s:%d, %x\n", $r0, $r1, $r2, $r14
  c
end

# task_create
b *0xBFE14A30
commands
  silent
  print_current_location
  KBLU
  printf "task_create(%s, prio=%x, stack=%x, entry=%x, arg=%x)\n", $r0, $r1, $r2, $r3, *(int*)$sp
  KRESET
  c
end

# msleep
b *0xBFE14998
commands
  silent
  print_current_location
  printf "msleep(%d)\n", $r0
  c
end

# take_semaphore
b *0xBFE15400
commands
  silent
  print_current_location
  printf "take_semaphore(0x%x, %d)\n", $r0, $r1
  c
end

b *0xBFE1546E
commands
  silent
  print_current_location
  printf "take_semaphore => %d\n", $r0
  c
end

b *0xBFE15472
commands
  silent
  print_current_location
  printf "take_semaphore => %d\n", $r0
  c
end

b *0xBFE15476
commands
  silent
  print_current_location
  printf "take_semaphore => %d\n", $r0
  c
end

# give_semaphore
b *0xBFE15478
commands
  silent
  print_current_location
  printf "give_semaphore(0x%x)\n", $r0
  c
end

cont
