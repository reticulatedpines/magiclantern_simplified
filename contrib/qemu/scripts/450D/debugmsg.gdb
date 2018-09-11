# ./run_canon_fw.sh 450D -d debugmsg
# ./run_canon_fw.sh 450D -d debugmsg -s -S & arm-none-eabi-gdb -x 450D/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
#symbol-file ../magic-lantern/platform/450D.110/magiclantern
#symbol-file ../magic-lantern/platform/450D.110/autoexec
#symbol-file ../magic-lantern/platform/450D.110/stubs.o

macro define CURRENT_TASK 0x355C0
macro define CURRENT_TASK_NAME (((int*)CURRENT_TASK)[0] ? ((char***)CURRENT_TASK)[0][13] : CURRENT_TASK)
macro define CURRENT_ISR  0

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xFFD07654
# DebugMsg_log

b *0xFFD15914
assert_log

b *0xFFCFFAB4
task_create_log

b *0xFFCFF768
msleep_log

b *0xFFCFDC18
register_interrupt_log

b *0xFFCF98C4
register_func_log

b *0xFFD0A2B0
CreateStateObject_log

if 0
  b *0xFFCF4BE0
  rtc_read_log
end

cont
