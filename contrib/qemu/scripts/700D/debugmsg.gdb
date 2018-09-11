# ./run_canon_fw.sh 700D -d debugmsg
# ./run_canon_fw.sh 700D -d debugmsg -s -S & arm-none-eabi-gdb -x 700D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/700D.115/magiclantern
#symbol-file ../magic-lantern/platform/700D.115/autoexec
#symbol-file ../magic-lantern/platform/700D.115/stubs.o

macro define CURRENT_TASK 0x233DC
macro define CURRENT_ISR  (MEM(0x670) ? MEM(0x674) >> 2 : 0)

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0x395C
# DebugMsg_log

b *0x1900
assert_log

b *0x6868
task_create_log

b *0x13344
register_interrupt_log

b *0xFF138B0C
register_func_log

b *0x16890
CreateStateObject_log

if 0
  b *0xFF132368
  rtc_read_log
end

cont
