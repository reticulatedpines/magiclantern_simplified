# GDB scripts for tracing stuff around the firmware
# This file contains generic routines that can be used on all cameras.
# Requires arm-none-eabi-gdb >= 7.7 (gcc-arm-none-eabi-4_9-2015q3 or later)

# To use gdb, start emulation with, for example:
#    ./run_canon_fw.sh 60D -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb

# misc preferences
set pagination off
set output-radix 16

set architecture arm
set tcp connect-timeout 300

if $_isvoid($TCP_PORT)
    target remote localhost:1234
else
    eval "target remote localhost:%d", $TCP_PORT
end

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
#      macro define CURRENT_ISR  (MEM(0x648) ? MEM(0x64C) >> 2 : 0)
#
# RTC_VALID_FLAG:
#   Only needed if you use load_default_date_time_log.
#   In the RTC initialization routine, this flag
#   is set to 1 if date/time was read successfully.
#
# NUM_CORES:
#   Defaults to 1; only needed on multi-core machines.
#
# PRINT_CALLSTACK:
#   Set to 1 if you want to print call stack for every single debug message:
#   macro define PRINT_CALLSTACK 1
#   It will be displayed before each message, in print_current_location.
#
################################################################################

# dummy definitions
macro define CURRENT_TASK   ((int)0xFFFFFFFF)
macro define CURRENT_ISR    ((int)0xFFFFFFFF)
macro define RTC_VALID_FLAG ((int)0xFFFFFFFF)
macro define NUM_CORES      1
macro define PRINT_CALLSTACK 0

# some of the firmware-specific constants can be found by pattern matching
define find_rom_string
  if $_isvoid($_)
    find /1 0xFE000000, 0xFFFFFFF0, $arg0
  end
  if $_isvoid($_)
    find /1 0xE0000000, 0xFFFFFFF0, $arg0
  end
end
document find_rom_string
Helper to find some constant string in the ROM.
This is a trick to use strings without defining the address of "malloc", like GDB normally requires.
end

if $_isvoid($NULL_STR)
  # only look this up if not defined in CAM/debugmsg.gdb
  find_rom_string "(null)"
  set $NULL_STR = $_
end

# helper to dereference strings
macro define STR(x) ((x) ? (x) : $NULL_STR)

# helper to read an uint32_t from memory (used in ML as well)
macro define MEM(x) (*(unsigned int*)(x))

# helper to print a hex char (lowest 4 bits)
macro define HEX_DIGIT(x) (char)((((x)&0xF) < 10) ? 48 + ((x)&0xF) : 55 + ((x)&0xF))

define hook-quit
  set confirm off
  show convenience
  named_func_hook_quit
  state_objects_hook_quit
  kill inferiors 1
  KRESET
end
document hook-quit
Called at the end of the debugging session.
end

# Helpers to silence all GDB messages
define silence_start
  set logging file /dev/null
  set logging redirect on
  set logging on
end
document silence_start
Turn off all GDB messages.
end

define silence_end
  set logging off
end
document silence_end
Resume printing GDB messages.
end

# color output to terminal
define KRED
    printf "%c[1;31m", 0x1B
end
document KRED
Red text (with ANSI escape codes).
end

define KCYN
    printf "%c[1;36m", 0x1B
end
document KCYN
Cyan text (with ANSI escape codes).
end

define KBLU
    printf "%c[1;34m", 0x1B
end
document KBLU
Blue text (with ANSI escape codes).
end

define KGRN
    printf "%c[1;32m", 0x1B
end
document KGRN
Green text (with ANSI escape codes).
end

define KYLW
    printf "%c[1;33m", 0x1B
end
document KYLW
Yellow text (with ANSI escape codes).
end

define KRESET
    printf "%c[0m", 0x1B
end
document KRESET
Back to normal text (reset ANSI color attributes).
end

# task name for DryOS (only if CURRENT_TASK is a valid pointer; otherwise, empty string)
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][9] : CURRENT_TASK)

define print_callstack
  set $_ = *0xC0123430
end
document print_callstack
Helper to trigger a stack trace. Requires -d callstack, for example:
./run_canon_fw 1300D,firmware="boot=0" -d callstack -s -S & arm-none-eabi-gdb -x 1300D/debugmsg.gdb
end

# print current task name and return address
# optional argument: custom return address
define print_current_location
  KRESET
  if CURRENT_ISR == 0xFFFFFFFF
    printf "Please define CURRENT_ISR.\n"
  end
  if CURRENT_TASK == 0xFFFFFFFF
    printf "Please define CURRENT_TASK.\n"
  end

  if PRINT_CALLSTACK == 1
    print_callstack
  end

  if NUM_CORES > 1
    printf "[CPU%d] ", ($_thread-1)
  end

  printf "["
  if CURRENT_ISR > 0
    KRED
    if CURRENT_ISR >= 0x100
      printf "    INT-%03Xh", CURRENT_ISR
    else
      printf "     INT-%02Xh", CURRENT_ISR
    end
  else
    if $_thread == 1
      KCYN
    else
      KYLW
    end
    printf "%12s", CURRENT_TASK_NAME
  end
  if $argc == 1
    printf ":%08x ", $arg0
  else
    printf ":%08x ", $lr - 4
  end
  KRESET
  printf "] "
end
document print_current_location
Helper to print current location (context info for each debug message):
- CPU ID (for multi-core machines only)
- program counter (PC register)
- task name (where applicable)
end

define print_current_location_with_callstack
  if PRINT_CALLSTACK != 1
    print_callstack
  end
  print_current_location
end
document print_current_location_with_callstack
Helper to print current location, including a stack trace.
end

define print_current_location_placeholder
  if NUM_CORES > 1
    printf "       "
  end
  printf "                         "
end
document print_current_location_placeholder
Helper to print spaces of the same size as the location info.
Useful to align multi-line messages.
end

define try_expand_ram_struct
    if $arg0 > 0x1000 && $arg0 < 0x1000000
        print_current_location_placeholder
        printf "*0x%x = { %x %x %x %x %x ... }\n", $arg0, MEM($arg0), MEM($arg0+4), MEM($arg0+8), MEM($arg0+12), MEM($arg0+16)
    end
end
document try_expand_ram_struct
Helper to print for unknown data structures.
end

define print_formatted_string
  # count how many % characters we have
  # (gdb is very picky about the number of arguments...)
  # note: 60D uses %S incorrectly at some point, so we don't format that string
  set $num = 0
  set $i = 0
  set $badfmt = 0
  set $chr = *(char*)($arg0+$i)
  set $nxt = *(char*)($arg0+$i+1)
  while $chr
    set $num = $num + ( $chr == '%' && $nxt != '%' )
    set $badfmt = $badfmt + ( $chr == '%' && ($nxt == 'S' || $nxt == 'R' ))
    set $badfmt = $badfmt + ( $chr == '"' )
    set $i = $i + 1
    set $chr = $nxt
    set $nxt = *(char*)($arg0+$i+1)
  end
  
  # fixme: is gdb's nested if/else syntax that ugly?
  # or, even better: a nicer way to format these strings?
  if $num == 0 || $badfmt
    printf "%s\n", $arg0
  else
  if $num == 1
    eval "printf \"%s\n\", $arg1", $arg0
  else
  if $num == 2
    eval "printf \"%s\n\", $arg1, $arg2", $arg0
  else
  if $num == 3
    eval "printf \"%s\n\", $arg1, $arg2, $arg3", $arg0
  else
  if $num == 4
    eval "printf \"%s\n\", $arg1, $arg2, $arg3, $arg4", $arg0
  else
  if $num == 5
    eval "printf \"%s\n\", $arg1, $arg2, $arg3, $arg4, $arg5", $arg0
  else
  if $num == 6
    eval "printf \"%s\n\", $arg1, $arg2, $arg3, $arg4, $arg5, $arg6", $arg0
  else
  if $num == 7
    eval "printf \"%s\n\", $arg1, $arg2, $arg3, $arg4, $arg5, $arg6, $arg7", $arg0
  else
  if $num == 8
    eval "printf \"%s\n\", $arg1, $arg2, $arg3, $arg4, $arg5, $arg6, $arg7, $arg8", $arg0
  else
    KRED
    printf "%s [FIXME: %d args]\n", $arg0, $num
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
end
document print_formatted_string
Helper to print formatted strings (a la printf).
end

define DebugMsg_log
  commands
    silent
    print_current_location
    printf "(%02x:%02x) ", $r0, $r1
    print_formatted_string $r2 $r3 MEM($sp) MEM($sp+4) MEM($sp+8) MEM($sp+12) MEM($sp+16) MEM($sp+20) MEM($sp+24)
    c
  end
end
document DebugMsg_log
Log calls to DebugMsg(int class, int level, const char * fmt, ...).
Used mostly in EOS firmware.
end

# DebugMsg-like calls with only one extra argument
define DebugMsg1_log
  commands
    silent
    print_current_location
    printf "(%02x) ", $r0
    print_formatted_string $r1 $r2 $r3 MEM($sp) MEM($sp+4) MEM($sp+8) MEM($sp+12) MEM($sp+16) MEM($sp+20)
    c
  end
end
document DebugMsg1_log
Log calls to DebugMsg1(int context, const char * fmt, ...).
Used mostly in PowerShot firmware.
end

define printf_log
  commands
    silent
    print_current_location
    print_formatted_string $r1 $r2 $r3 MEM($sp) MEM($sp+4) MEM($sp+8) MEM($sp+12) MEM($sp+16) MEM($sp+20)
    c
  end
end
document printf_log
Log calls to plain printf.
end

# helper to decompose a bitfield value
define print_bits
    set $i = 0
    set $b = $arg0
    while $i < 32
        if $b & (1 << $i)
            printf "%d", $b & (1 << $i)
            set $b &= ~(1 << $i)
            if $b
                printf "|"
            end
        end
        set $i = $i + 1
    end
end
document print_bits
Helper to decompose a bitfield value.
Example: print_bits 12
Outputs: 4|8
end

# Export named functions to IDC (for IDA)
#########################################

set $named_func_first_time = 1

define named_func_add
  set logging file named_functions.idc
  set logging redirect on
  set logging on
  if $named_func_first_time == 1
    set logging off
    set logging overwrite on
    set logging on
    printf "/* List of named functions identified during execution. */\n"
    set logging off
    set logging overwrite off
    set logging on
    printf "/* Generated from QEMU+GDB. */\n"
    printf "\n"
    printf "#include <idc.idc>\n"
    printf "\n"
    printf "static MakeAutoName(ea,name)\n"
    printf "{\n"
    printf "    auto p; while ((p = strstr(name, \" \")) >= 0) name[p] = \"_\";\n"
    printf "    if (!hasUserName(GetFlags(ea))) {\n"
    printf "      if (!(MakeNameEx(ea,name,SN_AUTO|SN_CHECK))) {\n"
    printf "      if (!(MakeNameEx(ea,name+\"_0\",SN_AUTO|SN_CHECK))) {\n"
    printf "      if (!(MakeNameEx(ea,name+\"_1\",SN_AUTO|SN_CHECK))) {\n"
    printf "      if (!(MakeNameEx(ea,name+\"_2\",SN_AUTO|SN_CHECK))) {\n"
    printf "      if (!(MakeNameEx(ea,name+\"_3\",SN_AUTO|SN_CHECK))) {\n"
    printf "         MakeRptCmt(ea,name); }}}}}\n"
    printf "    } else {\n"
    printf "      Message(\"Already named: %%X %%s -- %%s\\n\", ea, name, Name(ea));\n"
    printf "      MakeRptCmt(ea,name);\n"
    printf "    }\n"
    printf "}\n"
    printf "\n"
    printf "static MakeAutoNamedFunc(ea,name)\n"
    printf "{\n"
    printf "    SetReg(ea, \"T\", ea & 1);\n"
    printf "    MakeCode(ea & ~1);\n"
    printf "    MakeFunction(ea & ~1, BADADDR);\n"
    printf "    MakeAutoName(ea & ~1, name);\n"
    printf "}\n"
    printf "\n"
    printf "static main()\n"
    printf "{\n"
    set $named_func_first_time = 0
  end

  printf "  MakeAutoNamedFunc(0x%08X, \"", $arg0

  # name prefix
  if $arg1
    printf "%s", $arg1
    if $argc >= 3
      printf "_"
    end
  end

  # name suffix
  # fixme: cannot pass arbitrary strings as arguments, but chars work fine
  if $argc == 3
    printf "%c", $arg2
  end
  if $argc == 4
    printf "%c%c", $arg2, $arg3
  end
  if $argc == 5
    printf "%c%c%c", $arg2, $arg3, $arg4
  end
  if $argc == 6
    printf "%c%c%c%c", $arg2, $arg3, $arg4, $arg5
  end
  if $argc == 7
    printf "%c%c%c%c%c", $arg2, $arg3, $arg4, $arg5, $arg6
  end
  if $argc == 8
    printf "%c%c%c%c%c%c", $arg2, $arg3, $arg4, $arg5, $arg6, $arg7
  end
  if $argc == 9
    printf "%c%c%c%c%c%c%c", $arg2, $arg3, $arg4, $arg5, $arg6, $arg7, $arg8
  end
  if $argc == 10
    printf "%c%c%c%c%c%c%c%c", $arg2, $arg3, $arg4, $arg5, $arg6, $arg7, $arg8, $arg9
  end

  if $argc <= 10
    printf "\");"
  else
    set logging off
    KRED
    printf "FIXME: too many args\n"
    KRESET
    quit
  end
  printf "\n"
  set logging off
end
document named_func_add
Helper to add a named function into named_functions.idc.
Names can come from anywhere (register_func, task_create etc)
Syntax: named_func_add function_address name_string [ optional suffix chars ]
end

# all of this just to close the brace :)
define named_func_hook_quit
  if $named_func_first_time == 0
    set logging file named_functions.idc
    set logging redirect on
    set logging on
    printf "}\n"
    set logging off
    KRED
    printf "\nnamed_functions.idc saved.\n"
    KRESET
    printf "If it looks good, consider renaming or moving it, for future use.\n\n"
  end
end
document named_func_hook_quit
Helper to finish writing named_functions.idc.
end

# Named function code ends here
# calls to named_func_add be made from various loggers
######################################################

# Export state object definitions as Python code
################################################

set $state_objects_first_time = 1

define state_object_add
  set logging file state_objects.py
  set logging redirect on
  set logging on
  if $state_objects_first_time == 1
    set logging off
    set logging overwrite on
    set logging on
    printf "# State object list: (address, states, inputs, name, pc)\n"
    set logging off
    set logging overwrite off
    set logging on
    printf "# Generated from QEMU+GDB.\n"
    printf "\n"
    printf "States = [\n"
    set $state_objects_first_time = 0
  end

  printf "  (0x%08X, %2d, %2d, '%s', 0x%08X),\n", $arg0, $arg1, $arg2, $arg3, $pc
  set logging off
end
document state_object_add
Helper to add a state object definition into state_objects.py.
Syntax: state_object_add state_matrix_address, num_states, num_inputs, state_machine_name
end

# all of this just to close the bracket :)
define state_objects_hook_quit
  if $state_objects_first_time == 0
    set logging file state_objects.py
    set logging redirect on
    set logging on
    printf "]\n"
    set logging off
    KRED
    printf "\nstate_objects.py saved.\n"
    KRESET
    printf "If it looks good, consider renaming or moving it, for future use.\n\n"
  end
end
document state_objects_hook_quit
Helper to finish writing state_objects.py.
end

# State object definitions end here
# calls to state_object_add be made from CreateStateObject_log or other loggers
###############################################################################

# log task_create calls
define task_create_log
  commands
    silent
    print_current_location
    KBLU
    printf "task_create(%s, prio=%x, stack=%x, entry=%x, arg=%x)\n", STR($r0), $r1, $r2, $r3, MEM($sp)
    KRESET
    named_func_add $r3 $r0 't' 'a' 's' 'k'
    c
  end
end
document task_create_log
Log calls to task_create(name, prio, stack, entry, arg).
end

define task_switch_log
  commands
    silent
    print_current_location
    printf "Task switch\n"
    c
  end
end
document task_switch_log
Log DryOS task (context) switches.
Usage:
  watch *CURRENT_TASK
  task_switch_log
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
document msleep_log
Log calls to msleep(int milliseconds).
end

# assert
define assert_log
  commands
    silent
    print_current_location_with_callstack
    printf "["
    KRED
    printf "ASSERT"
    KRESET
    printf "] "
    printf "%s at %s:%d, %x\n", STR($r0), STR($r1), $r2, $lr
    c
  end
end
document assert_log
Log calls to ASSERT(message, file, line), including a stack trace.
Tip: run emulation with -d callstack.
end

define assert0_log
  commands
    silent
    print_current_location_with_callstack
    printf "["
    KRED
    printf "ASSERT"
    KRESET
    printf "] "
    printf " at %s:%d\n", STR($r0), $r1
    c
  end
end
document assert0_log
Log calls to ASSERT(message, line), including a stack trace.
Tip: run emulation with -d callstack.
end

# semaphores

define create_semaphore_log
  commands
    silent
    print_current_location
    KBLU
    printf "create_semaphore('%s', %d)\n", STR($r0), $r1
    KRESET
    set $sem_cr_name = $r0
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
      if $sem_cr_name == -1234
        KRED
        # fixme: create_semaphore is not atomic,
        # so if two tasks create semaphores at the same time, we may mix them up
        # (maybe call cli/sei from gdb, or is this check enough?)
        print "create semaphore: race condition?"
        KRESET
      end
      printf "Created semaphore 0x%x: %x '%s'\n", $r0, $sem_cr_name, STR($sem_cr_name)
      eval "set $sem_%x_name = $sem_cr_name", $r0
      set $sem_cr_name = -1234
      c
    end
    silence_end
    c
  end
end
document create_semaphore_log
Log calls to create_semaphore(const char * name, int initial_value).
We keep track of these to print their names from other MQ functions.
end

define create_semaphore_n3_log
  commands
    silent
    print_current_location
    KBLU
    printf "create_semaphore(%d, %d, '%s')\n", $r0, $r1, STR($r2)
    set $sem_cr_name = $r2
    KRESET
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
      if $sem_cr_name == -1234
        KRED
        # fixme: create_semaphore is not atomic,
        # so if two tasks create semaphores at the same time, we may mix them up
        # (maybe call cli/sei from gdb, or is this check enough?)
        print "create semaphore: race condition?"
        KRESET
      end
      printf "Created semaphore 0x%x: %x '%s'\n", $r0, $sem_cr_name, STR($sem_cr_name)
      eval "set $sem_%x_name = $sem_cr_name", $r0
      set $sem_cr_name = -1234
      c
    end
    silence_end
    c
  end
end
document create_semaphore_n3_log
Log calls to create_semaphore(int unknown1, int unknown 2, const char * name).
We keep track of these to print their names from other MQ functions.
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
document delete_semaphore_log
Log calls to delete_semaphore(struct semaphore * sem).
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
document print_sem_name
Helper to print the name of a semaphore.
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
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
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
    silence_end
    c
  end
end
document take_semaphore_log
Log calls to take_semaphore(struct semaphore * sem, int timeout).
This call is blocking; any tasks waiting for it will be listed at shutdown.
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
document give_semaphore_log
Log calls to give_semaphore(struct semaphore * sem).
end

# message queues

define create_msg_queue_log
  commands
    silent
    print_current_location
    KBLU
    printf "create_msg_queue('%s', %d)\n", STR($r0), $r1
    KRESET
    set $mq_cr_name = $r0
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
      if $mq_cr_name == -1234
        KRED
        # fixme: create_msg_queue is not atomic,
        # so if two tasks create message queues at the same time, we may mix them up
        # (maybe call cli/sei from gdb, or is this check enough?)
        print "create message queue: race condition?"
        KRESET
      end
      printf "Created message queue 0x%x: %x '%s'\n", $r0, $mq_cr_name, STR($mq_cr_name)
      eval "set $mq_%x_name = $mq_cr_name", $r0
      set $mq_cr_name = -1234
      c
    end
    silence_end
    c
  end
end
document create_msg_queue_log
Log calls to create_msg_queue(const char * name, int count).
We keep track of these to print their names from other MQ functions.
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
document print_mq_name
Helper to print the name of a message queue.
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
document post_msg_queue_log
Log calls to post_msg_queue(struct msg_queue * queue, int msg)
and also try_post_msg_queue(struct msg_queue * queue, int msg, int unknown)
end

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
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
      if $r0
        KRED
      else
        KGRN
      end
      printf "try_receive_msg_queue => "
      KRESET
      printf "%d (pc=%x)\n", $r0, $pc
      eval "try_expand_ram_struct $mq_%s_buf", CURRENT_TASK_NAME
      eval "try_expand_ram_struct MEM($mq_%s_buf)", CURRENT_TASK_NAME
      eval "set $task_%s = \"ready\"", CURRENT_TASK_NAME
      c
    end
    silence_end
    c
  end
end
document try_receive_msg_queue_log
Log calls to try_receive_msg_queue(struct msg_queue *queue, void *buffer, int timeout).
This call is blocking; any tasks waiting for it will be listed at shutdown.
end

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
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
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
    silence_end
    c
  end
end
document receive_msg_queue_log
Log calls to receive_msg_queue(struct msg_queue *queue, void *buffer).
This call is blocking; any tasks waiting for it will be listed at shutdown.
end

# interrupts

define register_interrupt_log
  commands
    silent
    print_current_location
    if $r0 && ((char*)$r0)[0]
      printf "register_interrupt(%s, 0x%x, 0x%x, 0x%x)\n", $r0, $r1, $r2, $r3
      if (unsigned int) $r2 > 0x1000
        named_func_add $r2 $r0 'I' 'S' 'R'
      end
    else
      printf "register_interrupt(null, 0x%x, 0x%x, 0x%x)\n", $r1, $r2, $r3
    end
    c
  end
end
document register_interrupt_log
Log interrupt registration (some of them are registered by name in Canon firmware).
These functions will be listed in named_functions.idc.
end

# eventprocs (functions that can be called by name)
# RegisterEventProcedure on some models

# new-style, with 3 arguments
define register_func_log
  commands
    silent
    print_current_location
    KBLU
    printf "register_func('%s', %x, %x)\n", $r0, $r1, $r2
    KRESET
    if (unsigned int) $r2 > (unsigned int) 0xE0000000
        # some functions are registered indirectly, using a wrapper
        # the actual function is passed as argument
        named_func_add $r2 $r0
    else
        named_func_add $r1 $r0
    end
    c
  end
end
document register_func_log
Log functions registered by name in Canon firmware (aka "event procedures").
These functions will be listed in named_functions.idc.
This should be used on DIGIC 4 and newer models; for VxWorks, see register_func_old_log.
end

# old-style, with 2 arguments (some VxWorks models only)
define register_func_old_log
  commands
    silent
    print_current_location
    KBLU
    printf "register_func('%s', %x)\n", $r0, $r1
    KRESET
    named_func_add $r1 $r0
    c
  end
end
document register_func_old_log
Same as register_func_log, but for VxWorks models.
end

define call_by_name_log
  commands
    silent
    print_current_location
    KYLW
    printf "call('%s', %x)\n", $r0, $r1
    KRESET
    c
  end
end
document call_by_name_log
Log calls to event procedures (named functions).
FIXME: only the first argument is printed.
end

define register_cmd_log
  commands
    silent
    print_current_location
    KBLU
    printf "register_cmd('%s', %x, '%s')\n", $r2, $r3, MEM($sp)
    KRESET
    named_func_add $r3 $r2
    c
  end
end
document register_cmd_log
Log named functions registered to the DryOS shell.
These functions will be listed in named_functions.idc.
end

define mpu_decode
  set $buf = $arg0
  if $argc == 2
    set $size = $arg1
  else
    set $size = ((char*)$buf)[0]
  end
  set $i = 0
  while $i < $size
    printf "%02x ", ((char*)$buf)[$i]
    set $i = $i + 1
  end
end
document mpu_decode
Helper to print MPU messages.
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
document mpu_send_log
Log messages sent to the MPU. See "MPU communication" in HACKING.rst.
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
document mpu_recv_log
Log messages received from the MPU. See "MPU communication" in HACKING.rst.
end

define mpu_analyze_recv_data_log
  commands
    silent
    #print_current_location
    #printf "AnalyzeMpuReceiveData %x %x %x %x\n", $r0, $r1, $r2, $r3
    print_current_location
    printf "MPU property: %02x %02x ", MEM($r1), MEM($r1+4)
    mpu_decode MEM($r1+8) MEM($r1+12)
    printf "\n"
    c
  end
end
document mpu_analyze_recv_data_log
Log calls to AnalyzeMpuReceiveData and list MPU property IDs.
These will show MPU property messages when they are actually processed.
They are received in SIO3_ISR and queued until the PropMgr task is able
to handle them, so it may be hard to match them with other debug messages
without logging this function.
Known MPU property IDs:
https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/known_spells.py
end

define prop_lookup_maybe_log
  commands
    silent
    print_current_location
    printf "prop_lookup_maybe %x %x %x %x\n", $r0, $r1, $r2, $r3
    c
  end
end
document prop_lookup_maybe_log
Log calls to prop_lookup_maybe.
This is called called many times; string: "NOT PROPERTYLIST ID".
end

define mpu_prop_lookup_log
  commands
    silent
    print_current_location
    #printf "mpu_prop_lookup (%02x %02x) %x, %x, %x, %x %x %x %x\n", $r3, MEM($sp), $r0, $r1, $r2, $r3, MEM($sp), MEM($sp+4), MEM($sp+8)
    set $mpl_r0 = $r0
    set $mpl_id1 = $r3
    set $mpl_id2 = MEM($sp)
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
      KYLW
      printf "mpu_prop_lookup (%02x %02x) => %x\n", $mpl_id1, $mpl_id2, **(int**)$mpl_r0
      KRESET
      c
    end
    silence_end
    c
  end
end
document mpu_prop_lookup_log
Log calls to mpu_prop_lookup.
This is called right after DivideCameraInitData and in many other
places in a loop, right after a memcpy from a ROM table with MPU IDs.
end

define prop_print_data
  set $buf = $arg0
  set $size = $arg1 / 4
  set $i = 0
  while $i < $size
    printf "%08x ", ((unsigned int *)$buf)[$i]
    set $i = $i + 1
  end
  if $arg1 % 4 == 3
    printf "%06x ", ((unsigned int *)$buf)[$i] & 0xFFFFFF
  end
  if $arg1 % 4 == 2
    printf "%04x ", ((unsigned int *)$buf)[$i] & 0xFFFF
  end
  if $arg1 % 4 == 1
    printf "%02x ", ((unsigned int *)$buf)[$i] & 0xFF
  end
end
document prop_print_data
Helper to pretty-print property data.
end

define prop_request_change_log
  commands
    silent
    print_current_location
    KRED
    printf "prop_request_change"
    KRESET
    printf " %08X { ", $r0
    prop_print_data $r1 $r2
    printf "}\n"
    c
  end
end
document prop_request_change_log
Log calls to prop_request_change (arguments: property ID, data, size).
end

define prop_deliver_log
  commands
    silent
    print_current_location
    KRED
    printf "prop_deliver"
    KRESET
    printf " %08X { ", MEM($r0)
    prop_print_data $r1 $r2
    printf "}\n"
    c
  end
end
document prop_deliver_log
Log calls to prop_deliver (arguments: pointer to property ID, data, size).
end

define try_post_event_log
  commands
    silent
    print_current_location
    printf "TryPostEvent('%s', '%s', 0x%x, 0x%x, 0x%x)\n", STR(MEM($r0)), STR(MEM($r1)), $r2, $r3, MEM($sp)
    try_expand_ram_struct $r3
    try_expand_ram_struct MEM($r3)
    try_expand_ram_struct MEM($r3+4)
    try_expand_ram_struct MEM($r3+8)
    try_expand_ram_struct MEM($r3+12)
    try_expand_ram_struct MEM($r3+16)
    try_expand_ram_struct MEM($sp)
    c
  end
end
document try_post_event_log
Log calls to TryPostEvent (many arguments, including pointers to data structures).
end

define delayed_call_print_name
  if $arg0
    printf "SetTimerAfter"
  else
    printf "SetHPTimerAfterNow"
  end
end
document delayed_call_print_name
Helper for logging calls to SetTimerAfter (0) or SetHPTimerAfterNow (1).
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
    silence_start
    tbreak *$r1
    commands
      silent
      print_current_location
      delayed_call_print_name $arg
      printf " calling CBR %x(%x,%x)\n", $pc, $r0, $r1
      c
    end
    silence_end
    c
  end
end
document delayed_call_log
Helper for logging calls to SetTimerAfter (0) or SetHPTimerAfterNow (1).
end

define SetTimerAfter_log
  delayed_call_log 0
end
document SetTimerAfter_log
Log calls to SetTimerAfter (arguments: delay_ms, cbr, cbr_overrun, arg).
end

define SetHPTimerAfterNow_log
  delayed_call_log 1
end
document SetHPTimerAfterNow_log
Log calls to SetHPTimerAfterNow (arguments: delay_us, cbr, cbr_overrun, arg).
end

define SetHPTimerNextTick_log
  commands
    silent
    print_current_location
    printf "SetHPTimerNextTick(last_expiry=%d, offset=%d, cbr=%x, overrun=%x, arg=%x)\n", $r0, $r1, $r2, $r3, MEM($sp)
    c
  end
end
document SetHPTimerNextTick_log
Log calls to SetHPTimerNextTick (arguments: last_expiry, offset, cbr, cbr_overrun, arg).
See selftest.mo, mlv_play.mo and edmac.mo for usage examples.
end

define CancelTimer_log
  commands
    silent
    print_current_location
    printf "CancelTimer(%x)\n", $r0
    c
  end
end
document CancelTimer_log
Log calls to CancelTimer (argument: timer object).
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
document engine_resource_description
Helper to describe resource IDs used by some ResLock.
end

define engine_resources_list
  set $i = 0
  while $i < $arg1
    print_current_location_placeholder
    printf "   %2d) %8x ", $i, ((int*)$arg0)[$i]
    engine_resource_description ((int*)$arg0)[$i]
    printf "\n"
    set $i = $i + 1
  end
end
document engine_resources_list
Helper to list resource IDs used by some ResLock.
end

define CreateResLockEntry_log
  commands
    silent
    print_current_location
    KBLU
    printf "CreateResLockEntry(%x, %x)\n", $r0, $r1
    KRESET
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
      printf "Created ResLock 0x%x:'\n", $r0
      engine_resources_list ((int*)$r0)[5] ((int*)$r0)[6]
      c
    end
    silence_end
    c
  end
end
document CreateResLockEntry_log
Log calls to CreateResLockEntry (arguments: resource ID list, count).
This will list all resource IDs used by this lock.
end

define LockEngineResources_log
commands
    silent
    print_current_location
    KYLW
    printf "LockEngineResources(%x)\n", $r0
    KRESET
    eval "set $task_%s = \"wait_rlk 0x%08X\"", CURRENT_TASK_NAME, $r0
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
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
    silence_end
    c
  end
end
document LockEngineResources_log
Log calls to LockEngineResources (arguments: resource lock object).
This call is blocking; any tasks waiting for it will be listed at shutdown.
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
document AsyncLockEngineResources_log
Log calls to AsyncLockEngineResources (arguments: resource lock object, cbr_function, arg).
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
document UnLockEngineResources_log
Log calls to UnLockEngineResources (arguments: resource lock object).
end

define StartEDmac_log
  commands
    silent
    print_current_location
    KBLU
    printf "StartEDmac(%d, %x)\n", $r0, $r1
    KRESET
    c
  end
end
document StartEDmac_log
Log calls to StartEDmac (arguments: channel, flags).
EDMAC channels are configured in advance (SetEDmac etc).
end

define SetEDmac_log
  commands
    silent
    print_current_location
    KBLU
    printf "SetEDmac(%d, 0x%x, 0x%x, 0x%x)\n", $r0, $r1, $r2, $r3
    if $r2
      print_current_location_placeholder
      printf "{ %dx%d %dx%d %dx%d %d %d %d %d %d }\n", MEM($r2+0x14), MEM($r2+0x1c), MEM($r2+0x18), MEM($r2+0x20), MEM($r2+0x24), MEM($r2+0x28), MEM($r2+0x00), MEM($r2+0x04), MEM($r2+0x08), MEM($r2+0x0c), MEM($r2+0x10)
      # xa*ya xb*yb xn*yn off1a off1b off2a off2b off3
    end
    KRESET
    c
  end
end
document SetEDmac_log
Log calls to SetEDmac (arguments: channel, buffer, struct edmac_info *, flags).
EDMAC info structure: raw numbers are printed, but not interpreted, see:
- https://www.magiclantern.fm/forum/index.php?topic=18315.0 (EDMAC transfer model)
- https://bitbucket.org/hudson/magic-lantern/src/unified/modules/edmac/edmac_util.c (edmac_format_size)
end

# date/time helpers

define print_date_time
    # arg0: struct tm *
    printf "%04d/%02d/%02d %02d:%02d:%02d", \
      ((int*)$arg0)[5] + 1900, ((int*)$arg0)[4] + 1, ((int*)$arg0)[3], \
      ((int*)$arg0)[2], ((int*)$arg0)[1], ((int*)$arg0)[0]
end
document print_date_time
Print date/time from a "struct tm *".
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
document set_date_time
Set date/time in a "struct tm *", e.g. in LoadCalendarFromRTC.
end

# no longer needed - we have RTC emulation
define load_default_date_time_log
  commands
    silent
    print_current_location
    printf "load_default_date_time(%x)\n", $r0
    set $tm = $r0
    print_date_time $tm
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
      printf "load_default_date_time => "
      print_date_time $tm
      printf "\n"
      print_current_location $pc
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
    silence_end
    c
  end
end
document load_default_date_time_log
Old workaround for missing RTC emulation; no longer needed.
Usage (550D 109):
  b *0xFF0638FC
  load_default_date_time_log
  macro define RTC_VALID_FLAG (*(int*)0x26C4)
end

define rtc_read_log
  commands
    silent
    print_current_location
    printf "RTC read register %x\n", $r1
    log_result
    c
  end
end
document rtc_read_log
Log calls to rtc_read (low-level function called from LoadCalendarFromRTC & co.)
end

define rtc_write_log
  commands
    silent
    print_current_location
    printf "RTC write register %x %x\n", $r0, $r1
    c
  end
end
document rtc_write_log
Log calls to rtc_write (low-level function called from LoadCalendarFromRTC & co.)
end

# state objects

define CreateStateObject_log
  commands
    silent
    print_current_location
    KBLU
    printf "CreateStateObject(%s, 0x%x, inputs=%d, states=%d)\n", $r0, $r2, $r3, MEM($sp)
    KRESET

    # enumerate all functions from this state machine
    set $state_name = (char *) $r0
    set $state_matrix = (int *) $r2
    set $max_inputs = $r3
    set $max_states = MEM($sp)
    set $old_state = 0

    # log this state object
    state_object_add $state_matrix $max_states $max_inputs $state_name

    while $old_state < $max_states
      set $input = 0
      while $input < $max_inputs
        set $next_state   = $state_matrix[($old_state + $max_states * $input) * 2]
        set $next_func    = $state_matrix[($old_state + $max_states * $input) * 2 + 1]
        if $next_func
          #printf "(%d) --%d--> (%d) %x %s_S%d_I%d\n", $old_state, $input, $next_state, $next_func, $state_name, $old_state, $input
          named_func_add $next_func $state_name 'S' 48+$old_state/10 48+$old_state%10 '_' 'I' 48+$input/10 48+$input%10
        end
        set $input = $input + 1
      end
      set $old_state = $old_state + 1
    end

    # note: I could have used log_result instead of this block, but wanted to get something easier to grep
    silence_start
    tbreak *($lr & ~1)
    commands
      silent
      print_current_location $pc
      KBLU
      printf "CreateStateObject => %x at %x\n", $r0, $pc
      KRESET
      c
    end
    silence_end
    c
  end
end
document CreateStateObject_log
Log calls to CreateStateObject (arguments: name, unknown, matrix, inputs, states)
and name each function from the state object matrix by the state and input IDs,
e.g. DisplayState_S01_I25 or SCSState_S04_I13.

State machine diagrams:
https://a1ex.bitbucket.io/ML/states/index.html

What is a StateObject?
Rather than have subroutines with millions of switch statements - it is often simpler to describe a process with a set of states and inputs.
A program can then been written with the help of a 'State Machine'. It uses a matrix of Inputs and States to move from one state to another.
A StateObject is a structure that Canon firmware uses to keep track of a state machine.

To find state objects:
( ./run_canon_fw.sh 60D,firmware="boot=0" -d ramw -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb ) |& grep --text CreateStateObject -A 1 | grep 'CreateStateObject\|ram'

To find CreateStateObject:
./run_canon_fw.sh 60D,firmware="boot=0" -d calls |& grep -a PropState
end

define state_transition_log
  commands
    silent
    print_current_location
    set $stateobj = $r0
    set $input    = $r2
    # see state-object.h; how to define structs in gdb?
    set $state_name   = ((char**)$stateobj)[1]
    set $state_matrix = ((int**) $stateobj)[4]
    set $max_states   = ((int*)  $stateobj)[6]
    set $old_state    = ((int*)  $stateobj)[7]
    set $next_state   = $state_matrix[($old_state + $max_states * $input) * 2]
    set $next_func    = $state_matrix[($old_state + $max_states * $input) * 2 + 1]
    KYLW
    printf "%s: (%d) --%d--> (%d)", $state_name, $old_state, $input, $next_state
    KRESET
    printf "      %x (x=%x z=%x t=%x)\n", $next_func, $r1, $r3, MEM($sp)
    c
  end
end
document state_transition_log
Log calls to state_transition (StateObject transitions triggered by inputs)
This function is placed by CreateStateObject in the state object structure
See state_transition_log in dm-spy-extra.c (dm-spy-experiments)
end

# PTP

define ptp_register_handler_log
  commands
    silent
    print_current_location
    KBLU
    printf "ptp_register_handler(%x, %x, %x)\n", $r0, $r1, $r2
    KRESET
    if $r0 >= 0x1000 && $r0 <= 0xFFFF
      named_func_add $r1 0 'P' 'T' 'P' '_' HEX_DIGIT($r0>>12) HEX_DIGIT($r0>>8) HEX_DIGIT($r0>>4) HEX_DIGIT($r0)
    else
      KRED
      printf "FIXME: invalid PTP ID\n"
      KRESET
    end
    c
  end
end
document ptp_register_handler_log
Log calls to ptp_register_handler (arguments: PTP ID, function, arg).
PTP functions are auto-named by PTP ID (e.g. PTP_101C).
PTP IDs can be looked up in PIMA 15740:2000, ISO 15740:2005 etc.
end

# ENGIO, ADTG, CMOS

define EngDrvOut_log
  commands
    silent
    print_current_location
    KGRN
    printf "EngDrvOut(0x%X, 0x%X)\n", $r0, $r1
    KRESET
    c
  end
end
document EngDrvOut_log
Log calls to EngDrvOut (arguments: register, value).
These registers are plain MMIO writes, i.e. directly visible with "-d io".
end

define engio_write_log
  commands
    silent
    print_current_location
    KGRN
    printf "engio_write(0x%X)\n", $r0
    set $a = $r0
    while *(int*)$a != -1
        print_current_location_placeholder
        printf "    [0x%X] <- 0x%X\n", *(int*)$a, *(int*)($a+4)
        set $a = $a + 8
    end
    KRESET
    c
  end
end
document engio_write_log
Log calls to engio_write (argument: list of 32-bit register/value pairs,
terminated with FFFFFFFF), listing the value of each ENGIO register.
These registers are plain MMIO writes, i.e. directly visible with "-d io".
end

define adtg_write_log
  commands
    silent
    print_current_location
    KGRN
    printf "adtg_write("
    print_bits $r0
    printf ", 0x%X)\n", $r1
    set $a = $r1
    while *(int*)$a != -1
        print_current_location_placeholder
        printf "    ADTG"
        print_bits $r0
        printf "[%04X] <- 0x%X\n", *(unsigned int *) $a >> 16, *(unsigned int *) $a & 0xFFFF
        set $a = $a + 4
    end
    KRESET
    c
  end
end
document adtg_write_log
Log calls to adtg_write ([REG] @@@@@@@@@@@@ Start ADTG[CS:%lx]),
listing the CS bits and the value of each ADTG register.
end

define cmos_write_log
  commands
    silent
    print_current_location
    KGRN
    printf "cmos_write(0x%X)\n", $r0
    set $a = $r0
    while *(short*)$a != -1
        print_current_location_placeholder
        printf "    CMOS[%X] <- 0x%X\n", *(unsigned short *) $a >> 12, *(unsigned short *) $a & 0xFFF
        set $a = $a + 2
    end
    KRESET
    c
  end
end
document cmos_write_log
Log calls to cmos_write ([REG] ############ Start CMOS),
listing the value of each CMOS register.
end

# Generic helpers
#################

define log_result
  silence_start
  tbreak *($lr & ~1)
  commands
    silent
    print_current_location $pc
    printf " => 0x%x\n", $r0
    c
  end
  silence_end
end
document log_result
Log the return value of any given function.
It sets up a temporary breakpoint on the LR register.
Usage:
  b *0xFF001234
  commands
    silent
    print_current_location
    printf "foobar(%x, %x)\n", $r0, $r1
    log_result
    c
  end
end

define generic_log
  commands
    silent
    print_current_location
    KYLW
    printf "call 0x%X(%x, %x, %x, %x)\n", $pc, $r0, $r1, $r2, $r3
    KRESET
    c
  end
end
document generic_log
Log the first 4 arguments of any given function.
Usage:
  b *0xFF001234
  generic_log
end


define generic_log_with_result
  commands
    silent
    print_current_location
    KYLW
    printf "call 0x%X(%x, %x, %x, %x)\n", $pc, $r0, $r1, $r2, $r3
    KRESET
    log_result
    c
  end
end
document generic_log_with_result
Log the first 4 arguments and the return value of any given function.
Usage:
  b *0xFF001234
  generic_log_with_result
end


# hexdump formatted with xxd
# https://stackoverflow.com/questions/9233095/memory-dump-formatted-like-xxd-from-gdb
define xxd
  if $argc == 1
    dump binary memory dump.tmp ((void*)$arg0) ((void*)$arg0)+0x100
  else
    dump binary memory dump.tmp ((void*)$arg0) ((void*)$arg0)+$arg1
  end
  eval "shell xxd -e -o 0x%X dump.tmp", ((void*)$arg0)
end

document xxd
Memory dump formatted with xxd. Temporary file: dump.tmp.
Syntax: xxd startaddr [size]
end
