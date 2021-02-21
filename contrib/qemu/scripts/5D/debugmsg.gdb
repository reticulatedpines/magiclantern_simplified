# ./run_canon_fw.sh 5D -d debugmsg
# ./run_canon_fw.sh 5D -d debugmsg -s -S & arm-none-eabi-gdb -x 5D/debugmsg.gdb
# fixme: this used to crash GDB (Cannot access memory at address 0xe7fddef6)
# but... why was it trying to do that?
# The crash happened after setting up the MMU (or MPU?) at FF810B60,
# where the background region is disabled.

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/5D.111/magiclantern
#symbol-file ../magic-lantern/platform/5D.111/autoexec
#symbol-file ../magic-lantern/platform/5D.111/stubs.o

macro define CURRENT_TASK 0x2D2C4
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  (MEM(0x10464) ? MEM(0x2f994) : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFFB20698
# DebugMsg_log

b *0xFFB2DA9C
assert_log

b *0xFFB18B40
task_create_log

b *0xFFB187F4
msleep_log

b *0xFFB16CDC
register_interrupt_log

b *0xFFB12930
register_func_old_log

b *0xFFB22574
CreateStateObject_log

b *0xFFB4066C
commands
  silent
  if $r3 == 0xF800002D
    KYLW
    printf "Workaround to prevent GDB from crashing...\n"
    # before: F8000000 - F87FFFFF
    # after:  E0000000 - FFFFFFFF (background region for the entire ROM area)
    set $r3 = 0xE0000039
    KRESET
  end
  c
end

cont
