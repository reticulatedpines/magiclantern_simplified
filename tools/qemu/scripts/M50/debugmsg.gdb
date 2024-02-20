# ./run_canon_fw.sh M50 -d debugmsg
# ./run_canon_fw.sh M50 -d debugmsg -s -S & arm-none-eabi-gdb -x M50/debugmsg.gdb

source -v debug-logging.gdb

# To get debugging symbols from Magic Lantern, uncomment one of these:
symbol-file ../magic-lantern/platform/M50.101/magiclantern
#symbol-file ../magic-lantern/platform/M50.101/autoexec
#symbol-file ../magic-lantern/platform/M50.101/stubs.o

macro define CURRENT_TASK 0x1028
macro define CURRENT_ISR  (*(int*)0x100C ? (*(int*)0x1010) : 0)
macro define NUM_CORES 2
macro define NULL_STR 0xE0041041

# GDB hook is very slow; -d debugmsg is much faster
# ./run_canon_fw.sh will use this address, don't delete it
# b *0xE0577EC4
# DebugMsg_log

b *0xE078645C
assert_log

b *0xE0545DD6
task_create_log

# what's the difference?!
b *0xE0545FFA
task_create_log

b *0xE05777C4
register_interrupt_log

b *0xE0572140
register_func_log

if 0
  b *0xE0572758
  create_semaphore8_log

  b *0xE057281E
  take_semaphore_log
end

b *0xE05591FA
CreateStateObject_log

b *0xE02A8994
mpu_send_log

b *0xE016F2D6
generic_log

b *0xE074F014
generic_log


b *0xE00508B4
generic_log

b *0xE0577ADC
generic_log

b *0xE0670468
commands
  silent
  print_current_location
  printf "I2C_Write(%x, %x, %x, %x)\n", $r0, $r1, $r2, $r3
  set $r0 = 0
  set $pc = $lr
  c
end

b *0xE05734AA
commands
  silent
  print_current_location
  printf "!! NIGHTMARE !! S_PROPAD_INVALIDPARAMETER 20003\n"
  set $r0 = 0
  set $pc = $lr
  c
end

b *0xE0044912
commands
  silent
  print_current_location
  printf "DataLoad_wait hack\n"
  set $r1 = 1000
  c
end

b *0xE004A48C
commands
  silent
  print_current_location
  printf "lens init?\n"
  set $r0 = 0
  set $pc = $lr
  c
end

b *0xE01E12D4
commands
  silent
  print_current_location
  printf "RemCPUSwChk\n"
  set $r0 = 0
  set $pc = $lr
  c
end

b *0xE016F2D6
commands
  silent
  print_current_location
  printf "conductor\n"
  set $r0 = 0
  set $pc = $lr
  c
end

if 0
b *0xE01E1274
commands
  silent
  print_current_location
  printf "SwitchCheck\n"
  set $r0 = 5
  set $r1 = 0
  set $pc = 0xE07599BC
  c
end
end

b *0xE07599BC
commands
  silent
  print_current_location
  KRED
  printf "LED drive %x %x\n", $r0, $r1
  KRESET
  log_result
  c
end

if 0
b *0xE05E98D2
commands
  silent
  print_current_location
  printf "rtc drv\n"
  set $r0 = 0
  set $pc = $lr
  c
end
end

#b *0x1b1d40
#b *0x1b1c40
#b my_cstart
#b my_dcache_clean

#b *0xE0004cea thread 2
#b *0xe00400fe
#commands
#    dump binary memory 40000000.bin 0x40000000 0x40100000
#    dump binary memory DF000000.bin 0xDF000000 0xDF100000
#end

# i2c_read
b *0xE06703C8
generic_log

# i2c_write
b *0xE0670468
generic_log

# hpcopy
if 0
b *0xE02E029E
commands
    silent
    print_current_location
    KRED
    printf "HPCopy(%x, %x, %x)\n", $r0, $r1, $r2
    KRESET
    set $r2 = 0x4
    tbreak *($lr & ~1)
    commands
        silent
        print_current_location
        KRED
        printf "HPCopy ret %x\n", $r0
        KRESET
    end
end
#generic_log
end


# HPCopy
# Hardware protocol looks complex; emulating from GDB for now
b *0xE02E029E
commands
  silent
  print_current_location
  KRED
  printf "HPCopy(%x, %x, %x)\n", $r0, $r1, $r2
  KRESET

  # execute plain memcpy instead (same arguments)
  set $pc = 0xE065E861

  # we need to return 0 on success, unlike memcpy
  tbreak *($lr & ~1)
  commands
    silent
    set $r0 = 0
    c
  end
  c
end

b *0xE01E12D4

b *0xE0659924
commands
  silent
  print_current_location
  KRED
  printf "Wakeup\n"
  KRESET
  c
end

b *0xE01E1274
commands
  silent
  print_current_location
  KRED
  printf "SwitchCheck skipping\n"
  KRESET
  set $pc = $lr
  c
end

b *0xE01A4B4E
commands
  silent
  print_current_location
  KRED
  printf "RTCMgrState_S00_I00 skipping\n"
  KRESET
  set $pc = $lr
  c
end

b *0xE01E3752
commands
  silent
  print_current_location
  KRED
  printf "PhySw stuff skipping\n"
  KRESET
  set $pc = $lr
  c
end

b *0xE0040C90
commands
  silent
  print_current_location
  KRED
  printf "take_sem WaitCCInit\n"
  KRESET
  set $r1 = 1000
  c
end

b *0xE00EE244
commands
  silent
  print_current_location
  KRED
  printf "SubCPU something skipping\n"
  KRESET
  set $pc = $lr
  c
end

#b *0xE00504B6


cont
