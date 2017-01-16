# ./run_canon_fw.sh EOSM -s -S & arm-none-eabi-gdb -x EOSM/debugmsg.gdb

source -v debug-logging.gdb

macro define CURRENT_TASK 0x3DE78
macro define CURRENT_ISR  (*(int*)0x670 ? (*(int*)0x674) >> 2 : 0)

b *0x40D4
DebugMsg_log

b *0x1900
assert_log

b *0x7048
task_create_log

# patch DL to avoid DL ERROR messages
set *(int*)0xFF1BE4AC = 0xe3a00015

# patch localI2C_Write to always return 1 (success)
set *(int*)0xFF3460C4 = 0xe3a00001
set *(int*)0xFF3460C8 = 0xe12fff1e

b *0xFF132DD0
load_default_date_time_log
macro define RTC_VALID_FLAG (*(int*)0x3E9F8)

if 0
    b *0x6ab8
    take_semaphore_log

    b *0x6ba4
    give_semaphore_log

    b *0x6958
    create_semaphore_log

    b *0x69b4
    create_semaphore_log
end

cont
