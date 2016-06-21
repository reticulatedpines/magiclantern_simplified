# GDB scripts for tracing stuff around the firmware
# This file contains generic routines that can be used on all cameras.
# Requires arm-none-eabi-gdb >= 7.7

# To use gdb, start emulation with:
#    ./run_canon_fw 60D -s -S
target remote localhost:1234

################################################################################
#
# In your main gdb script, you need to set some firmware addresses.
#
# CURRENT_TASK:
#   From task_create, look for a global pointer to current task structure
#      macro define CURRENT_TASK 0x1234
#
# CURRENT_ISR:
#   From interrupt handler (PC=0x18), find an expression that evaluates to
#   the current interrupt ID if any is running, or 0 if a normal task is running.
#   - on DIGIC 4/5, interrupt ID is MEM(0xC0201004) >> 2
#   - on DIGIC 6,   interrupt ID is MEM(0xD4011000)
#   To find the expression, look at the interrupt handler code (PC=0x18).
#   Example for 70D:
#      macro define CURRENT_ISR  (*(int*)0x648 ? (*(int*)0x64C) >> 2 : 0)
#
################################################################################

# dummy definitions
macro define CURRENT_TASK   ((int)0xFFFFFFFF)
macro define CURRENT_ISR    ((int)0xFFFFFFFF)


# misc preferences
set pagination off
set radix 16

define hook-quit
  set confirm off
  show convenience
  kill inferiors 1
  KRESET
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

define KGRN
    printf "%c[1;32m", 0x1B
end

define KYLW
    printf "%c[1;33m", 0x1B
end

define KRESET
    printf "%c[0m", 0x1B
end

macro define CURRENT_TASK_NAME ((char***)CURRENT_TASK)[0][9]

# print current task name and return address
define print_current_location
  KRESET
  if CURRENT_ISR == 0xFFFFFFFF
    printf "Please define CURRENT_ISR.\n"
  end
  if CURRENT_TASK == 0xFFFFFFFF
    printf "Please define CURRENT_TASK.\n"
  end

  printf "["
  if CURRENT_ISR > 0
    KRED
    printf "   INT-%02Xh:%08x ", CURRENT_ISR, $r14-4
  else
    KCYN
    printf "%10s:%08x ", CURRENT_TASK_NAME, $r14-4
  end
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
      set $badfmt = $badfmt + ( *(char*)($r2+$i) == '"' )
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

# semaphores

define create_semaphore_log
  commands
    silent
    print_current_location
    KBLU
    printf "create_semaphore('%s', %d)\n", $r0, $r1
    KRESET
    set $sem_cr_name = $r0
    tbreak *($lr & ~1)
    commands
      silent
      if $sem_cr_name == -1234
        KRED
        # fixme: create_semaphore is not atomic,
        # so if two tasks create semaphores at the same time, we may mix them up
        # (maybe call cli/sei from gdb, or is this check enough?)
        print "create semaphore: race condition?"
        KRESET
      end
      printf "*** Created semaphore 0x%x: %x '%s'\n", $r0, $sem_cr_name, $sem_cr_name
      eval "set $sem_%x_name = $sem_cr_name", $r0
      set $sem_cr_name = -1234
      c
    end
    c
  end
end

define delete_semaphore_log
  commands
    silent
    print_current_location
    printf "delete_semaphore(%x)\n", $r0
    eval "set $sem_%x_name = -1", $r0
    c
  end
end

define print_sem_name
 eval "set $sem_name = $sem_%x_name", $arg0
 if $_isvoid($sem_name)
   KRED
   printf " /* sem not created!!! */"
   KRESET
 else
 if $sem_name == -1
   KRED
   printf " /* sem deleted!!! */"
   KRESET
 else
 if $sem_name
  printf " "
  KCYN
  printf "'%s'", $sem_name
  KRESET
 end
 end
 end
end

define take_semaphore_log
  commands
    silent
    print_current_location
    KYLW
    printf "take_semaphore"
    KRESET
    printf "(0x%x", $r0
    print_sem_name $r0
    printf ", %d)\n", $r1
    eval "set $task_%s = \"wait_sem 0x%08X\"", CURRENT_TASK_NAME, $r0
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location
      if $r0
        KRED
      else
        KGRN
      end
      printf "take_semaphore => "
      KRESET
      printf "%d (pc=%x)\n", $r0, $pc
      eval "set $task_%s = \"ready\"", CURRENT_TASK_NAME
      c
    end
    c
  end
end

define give_semaphore_log
  commands
    silent
    print_current_location
    KCYN
    printf "give_semaphore"
    KRESET
    printf "(0x%x", $r0
    print_sem_name $r0
    printf ")\n"
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

define mpu_decode
  set $buf = $arg0
  set $size = ((char*)$buf)[0]
  set $i = 0
  while $i < $size
    printf "%02x ", ((char*)$buf)[$i]
    set $i = $i + 1
  end
end

define mpu_send_log
  commands
    silent
    print_current_location
    KBLU
    printf "mpu_send( %02x ", ($r1 + 2) & 0xFE
    mpu_decode $r0
    printf ")\n"
    KRESET
    c
  end
end

define mpu_recv_log
  commands
    silent
    print_current_location
    KBLU
    printf "mpu_recv( %02x ", ((char*)$r0)[-1]
    mpu_decode $r0
    printf ")\n"
    KRESET
    c
  end
end

define try_expand_ram_struct
    if $arg0 > 0x1000 && $arg0 < 0x1000000
        printf "                       "
        printf "*0x%x = { %x %x %x %x %x ... }\n", $arg0, *(int*)$arg0, *(int*)($arg0+4), *(int*)($arg0+8), *(int*)($arg0+12), *(int*)($arg0+16)
    end
end

define try_post_event_log
  commands
    silent
    print_current_location
    printf "TryPostEvent('%s', '%s', 0x%x, 0x%x, 0x%x)\n", *(int*)$r0, *(int*)$r1, $r2, $r3, *(int*)$sp
    try_expand_ram_struct $r3
    try_expand_ram_struct *(int*)($r3)
    try_expand_ram_struct *(int*)($r3+4)
    try_expand_ram_struct *(int*)($r3+8)
    try_expand_ram_struct *(int*)($r3+12)
    try_expand_ram_struct *(int*)($r3+16)
    c
  end
end

define delayed_call_print_name
  if $arg0
    printf "SetTimerAfter"
  else
    printf "SetHPTimerAfterNow"
  end
end

# for SetTimerAfter/SetHPTimerAfterNow
define delayed_call_log
  # passing strings to printf via $arg0 doesn't work
  # workaround: 0=SetTimerAfter, 1=SetHPTimerAfterNow (ugly, but at least it works)
  # another problem: $arg0 disappears in a commands block => save it here
  set $arg = $arg0
  commands
    silent
    print_current_location
    delayed_call_print_name $arg
    printf "(%d, cbr=%x, overrun=%x, arg=%x)\n", $r0, $r1, $r2, $r3
    if $r1 != $r2
      KRED
      printf "not handled: cbr != overrun\n"
      KRESET
    end
    tbreak *$r1
    commands
      silent
      print_current_location
      delayed_call_print_name $arg
      printf " calling CBR %x(%x,%x)\n", $pc, $r0, $r1
      c
    end
    c
  end
end

define SetTimerAfter_log
  delayed_call_log 0
end

define SetHPTimerAfterNow_log
  delayed_call_log 1
end

define SetHPTimerNextTick_log
  commands
    silent
    print_current_location
    printf "SetHPTimerNextTick(last_expiry=%d, offset=%d, cbr=%x, overrun=%x, arg=%x)\n", $r0, $r1, $r2, $r3, *(int*)$sp
    c
  end
end

define CancelTimer_log
  commands
    silent
    print_current_location
    printf "CancelTimer(%x)\n", $r0
    c
  end
end
