# GDB scripts for tracing stuff around the firmware
# This file contains generic routines that can be used on all cameras.
# Requires arm-none-eabi-gdb >= 7.7 (gcc-arm-none-eabi-4_9-2015q3 or later)

# To use gdb, start emulation with, for example:
#    ./run_canon_fw.sh 60D -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb

set remotetimeout 20
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
# RTC_VALID_FLAG:
#   Only needed if you use load_default_date_time_log.
#   In the RTC initialization routine, this flag
#   is set to 1 if date/time was read successfully.
#
################################################################################

# dummy definitions
macro define CURRENT_TASK   ((int)0xFFFFFFFF)
macro define CURRENT_ISR    ((int)0xFFFFFFFF)
macro define RTC_VALID_FLAG ((int)0xFFFFFFFF)

# misc preferences
set pagination off
set output-radix 16

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
    set $chr = *(char*)($r2+$i)
    set $nxt = *(char*)($r2+$i+1)
    while $chr
      set $num = $num + ( $chr == '%' && $nxt != '%' )
      set $badfmt = $badfmt + ( $chr == '%' && $nxt == 'S' )
      set $badfmt = $badfmt + ( $chr == '"' )
      set $i = $i + 1
      set $chr = $nxt
      set $nxt = *(char*)($r2+$i+1)
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

# message queues

define create_msg_queue_log
  commands
    silent
    print_current_location
    KBLU
    printf "create_msg_queue('%s', %d)\n", $r0, $r1
    KRESET
    set $mq_cr_name = $r0
    tbreak *($lr & ~1)
    commands
      silent
      if $mq_cr_name == -1234
        KRED
        # fixme: create_msg_queue is not atomic,
        # so if two tasks create message queues at the same time, we may mix them up
        # (maybe call cli/sei from gdb, or is this check enough?)
        print "create message queue: race condition?"
        KRESET
      end
      printf "*** Created message queue 0x%x: %x '%s'\n", $r0, $mq_cr_name, $mq_cr_name
      eval "set $mq_%x_name = $mq_cr_name", $r0
      set $mq_cr_name = -1234
      c
    end
    c
  end
end

# todo: delete_msg_queue_log

define print_mq_name
 eval "set $mq_name = $mq_%x_name", $arg0
 if $_isvoid($mq_name)
   KRED
   printf " /* mq not created!!! */"
   KRESET
 else
 if $mq_name == -1
   KRED
   printf " /* mq deleted!!! */"
   KRESET
 else
 if $mq_name
  printf " "
  KCYN
  printf "'%s'", $mq_name
  KRESET
 end
 end
 end
end

# int post_msg_queue(struct msg_queue * queue, int msg);
# int try_post_msg_queue(struct msg_queue * queue, int msg, int unknown);
define post_msg_queue_log
  commands
    silent
    print_current_location
    KCYN
    printf "post_msg_queue"
    KRESET
    printf "(0x%x", $r0
    print_mq_name $r0
    printf ", 0x%x)\n", $r1
    try_expand_ram_struct $r1
    c
  end
end

# int try_receive_msg_queue(struct msg_queue *queue, void *buffer, int timeout);
define try_receive_msg_queue_log
  commands
    silent
    print_current_location
    KYLW
    printf "try_receive_msg_queue"
    KRESET
    printf "(0x%x", $r0
    print_mq_name $r0
    printf ", %x, timeout=%d)\n", $r1, $r2
    eval "set $task_%s = \"wait_mq  0x%08X\"", CURRENT_TASK_NAME, $r0
    eval "set $mq_%s_buf = 0x%x", CURRENT_TASK_NAME, $r1
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location
      if $r0
        KRED
      else
        KGRN
      end
      printf "try_receive_msg_queue => "
      KRESET
      printf "%d (pc=%x)\n", $r0, $pc
      eval "try_expand_ram_struct $mq_%s_buf", CURRENT_TASK_NAME
      eval "try_expand_ram_struct *(int*)$mq_%s_buf", CURRENT_TASK_NAME
      eval "set $task_%s = \"ready\"", CURRENT_TASK_NAME
      c
    end
    c
  end
end

# int receive_msg_queue(struct msg_queue *queue, void *buffer);
define receive_msg_queue_log
  commands
    silent
    print_current_location
    KYLW
    printf "receive_msg_queue"
    KRESET
    printf "(0x%x", $r0
    print_mq_name $r0
    printf ", %x)\n", $r1
    eval "set $task_%s = \"wait_mq  0x%08X\"", CURRENT_TASK_NAME, $r0
    eval "set $mq_%s_buf = 0x%x", CURRENT_TASK_NAME, $r1
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location
      if $r0
        KRED
      else
        KGRN
      end
      printf "receive_msg_queue => "
      KRESET
      printf "%d (pc=%x)\n", $r0, $pc
      eval "try_expand_ram_struct $mq_%s_buf", CURRENT_TASK_NAME
      eval "set $task_%s = \"ready\"", CURRENT_TASK_NAME
      c
    end
    c
  end
end

# interrupts

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

# eventprocs (functions that can be called by name)
define register_func_log
  commands
    silent
    print_current_location
    KBLU
    printf "register_func('%s', %x, %x)\n", $r0, $r1, $r2
    KRESET
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

define engine_resource_description
    # gdb bug? shifting by 16 gives 0
    set $class = $arg0 & 0xFFFF0000
    set $entry = $arg0 & 0xFFFF
    if $class == 0x00000000
        printf "EDMAC write channel"
    end
    if $class == 0x00010000
        printf "EDMAC read channel"
    end
    if $class == 0x00020000
        printf "EDMAC write connection 0x%x", $entry
    end
    if $class == 0x00030000
        printf "EDMAC read connection 0x%x", $entry
    end
    if $class == 0x00050000
        printf "Image processing module"
    end
    if $class == 0x00110000
        printf "Bitmap/ImagePBAccessHandle"
    end
end

define engine_resources_list
  set $i = 0
  while $i < $arg1
    printf "    %2d) %8x ", $i, ((int*)$arg0)[$i]
    engine_resource_description ((int*)$arg0)[$i]
    printf "\n"
    set $i = $i + 1
  end
end

define CreateResLockEntry_log
  commands
    silent
    print_current_location
    KBLU
    printf "CreateResLockEntry(%x, %x)\n", $r0, $r1
    KRESET
    tbreak *($lr & ~1)
    commands
      silent
      printf "*** Created ResLock 0x%x:'\n", $r0
      engine_resources_list ((int*)$r0)[5] ((int*)$r0)[6]
      c
    end
    c
  end
end

define LockEngineResources_log
commands
    silent
    print_current_location
    KYLW
    printf "LockEngineResources(%x)\n", $r0
    KRESET
    eval "set $task_%s = \"wait_rlk 0x%08X\"", CURRENT_TASK_NAME, $r0
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location
      if $r0
        KRED
      else
        KGRN
      end
      printf "LockEngineResources => %x\n", $r0
      KRESET
      eval "set $task_%s = \"ready\"", CURRENT_TASK_NAME
      c
    end
    c
  end
end

define AsyncLockEngineResources_log
  commands
    silent
    print_current_location
    KYLW
    printf "AsyncLockEngineResources(%x, cbr=%x, arg=%x)\n", $r0, $r1, $r2
    KRESET
    c
  end
end

define UnLockEngineResources_log
  commands
    silent
    print_current_location
    KCYN
    printf "UnLockEngineResources(%x)\n", $r0
    KRESET
    c
  end
end

# date/time helpers

define print_date_time
    # arg0: struct tm *
    printf "%04d/%02d/%02d %02d:%02d:%02d", \
      ((int*)$arg0)[5] + 1900, ((int*)$arg0)[4] + 1, ((int*)$arg0)[3], \
      ((int*)$arg0)[2], ((int*)$arg0)[1], ((int*)$arg0)[0]
end

define set_date_time
    # args: struct tm *, year, month, day, hour, minute, second
    set ((int*)$arg0)[5] = $arg1 - 1900
    set ((int*)$arg0)[4] = $arg2 - 1
    set ((int*)$arg0)[3] = $arg3
    set ((int*)$arg0)[2] = $arg4
    set ((int*)$arg0)[1] = $arg5
    set ((int*)$arg0)[0] = $arg6
end

define load_default_date_time_log
  commands
    silent
    print_current_location
    printf "load_default_date_time(%x)\n", $r0
    set $tm = $r0
    print_date_time $tm
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location
      printf "load_default_date_time => "
      print_date_time $tm
      printf "\n"
      print_current_location
      printf "overriding date/time to : "
      set_date_time $tm 2015 01 15 13 37 00
      if RTC_VALID_FLAG == 0xFFFFFFFF
        printf "Please define RTC_VALID_FLAG.\n"
      else
        set RTC_VALID_FLAG = 1
      end
      print_date_time $tm
      printf "\n"
      c
    end
    c
  end
end
