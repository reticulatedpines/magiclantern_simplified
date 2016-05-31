# GDB scripts for tracing stuff around the firmware
# This file contains generic routines that can be used on all cameras.

# To use gdb, start emulation with:
#    ./run_canon_fw 60D -s -S
target remote localhost:1234

# misc preferences
set pagination off
set radix 16

define hook-quit
  set confirm off
end

# color output to terminal
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

# print current task name and return address
# (you need to define $CURRENT_TASK address in the firmware - look in task_create)
define print_current_location
  KRESET
  printf "["
  KCYN
  printf "%10s:%08x ", ((char***)$CURRENT_TASK)[0][9], $r14-4
  KRESET
  printf "] "
end

# trace all DebugMsg calls
define DebugMsg_log
  commands
    silent
    print_current_location

    # count how many % characters we have
    # (gdb is very picky about the number of arguments...)
    # note: 60D uses %S incorrectly at some point, so we don't format that string
    set $num = 0
    set $i = 0
    set $badfmt = 0
    while *(char*)($r2+$i)
      set $num = $num + ( *(char*)($r2+$i) == '%' && *(char*)($r2+$i+1) != '%' )
      set $badfmt = $badfmt + ( *(char*)($r2+$i) == '%' && *(char*)($r2+$i+1) == 'S' )
      set $i = $i + 1
    end
    
    # fixme: is gdb's nested if/else syntax that ugly?
    # or, even better: a nicer way to format these strings?
    if $num == 0 || $badfmt
      printf "(%02x:%02x) %s\n", $r0, $r1, $r2
    else
    if $num == 1
      eval "printf \"(%02x:%02x) %s\n\", $r3", $r0, $r1, $r2
    else
    if $num == 2
      eval "printf \"(%02x:%02x) %s\n\", $r3, *(int*)$sp", $r0, $r1, $r2
    else
    if $num == 3
      eval "printf \"(%02x:%02x) %s\n\", $r3, *(int*)$sp, *(int*)($sp+4)", $r0, $r1, $r2
    else
    if $num == 4
      eval "printf \"(%02x:%02x), %s\n\", $r3, *(int*)$sp, *(int*)($sp+4), *(int*)($sp+8)", $r0, $r1, $r2
    else
    if $num == 5
      eval "printf \"(%02x:%02x), %s\n\", $r3, *(int*)$sp, *(int*)($sp+4), *(int*)($sp+8), *(int*)($sp+12)", $r0, $r1, $r2
    else
    if $num == 6
      eval "printf \"(%02x:%02x), %s\n\", $r3, *(int*)$sp, *(int*)($sp+4), *(int*)($sp+8), *(int*)($sp+12), *(int*)($sp+16)", $r0, $r1, $r2
    else
    if $num == 7
      eval "printf \"(%02x:%02x), %s\n\", $r3, *(int*)$sp, *(int*)($sp+4), *(int*)($sp+8), *(int*)($sp+12), *(int*)($sp+16), *(int*)($sp+20)", $r0, $r1, $r2
    else
    if $num == 8
      eval "printf \"(%02x:%02x), %s\n\", $r3, *(int*)$sp, *(int*)($sp+4), *(int*)($sp+8), *(int*)($sp+12), *(int*)($sp+16), *(int*)($sp+20), *(int*)($sp+24)", $r0, $r1, $r2
    else
      KRED
      printf "%s [FIXME: %d args]\n", $r2, $num
      KRESET
    end
    end
    end
    end
    end
    end
    end
    end
    end
    c
  end
end

# log task_create calls
define task_create_log
  commands
    silent
    print_current_location
    KBLU
    printf "task_create(%s, prio=%x, stack=%x, entry=%x, arg=%x)\n", $r0, $r1, $r2, $r3, *(int*)$sp
    KRESET
    c
  end
end

# log msleep calls
define msleep_log
  commands
    silent
    print_current_location
    printf "*** msleep(%d)\n", $r0
    c
  end
end

# assert
define assert_log
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
end

# take_semaphore
define take_semaphore_log
  commands
    silent
    print_current_location
    printf "take_semaphore(0x%x, %d)\n", $r0, $r1
    c
  end
end

define take_semaphore_ret_log
  commands
    silent
    print_current_location
    printf "take_semaphore => %d\n", $r0
    c
  end
end

define give_semaphore_log
  commands
    silent
    print_current_location
    printf "give_semaphore(0x%x)\n", $r0
    c
  end
end

define register_interrupt_log
  commands
    silent
    print_current_location
    if $r0
      printf "register_interrupt(%s, 0x%x, 0x%x, 0x%x)\n", $r0, $r1, $r2, $r3
    else
      printf "register_interrupt(null, 0x%x, 0x%x, 0x%x)\n", $r1, $r2, $r3
    end
    c
  end
end
