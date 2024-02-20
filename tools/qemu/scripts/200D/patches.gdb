# ./run_canon_fw.sh 80D -s -S & arm-none-eabi-gdb -x 80D/patches.gdb -ex quit
# Only patches required for emulation

source patch-header.gdb

# experimental patches
# they probably do more harm than good - figure out what's up with them
if 1
    # startupPrepareCapture: pretend OmarInit was completed
    #set *(int*)0xFE0D91C8 = 0x4770

    # SHT_CAPTURE_PATH_InitializeCapturePath
    #set *(int*)0xFE1C0BF4 = 0x4770

    # startupPrepareDevelop
    #set *(int*)0xFE0D52C2 = 0x4770

    # EstimatedSize, this is setup with some struct on stack,
    # and we see strange values passed in, 0x51.
    # A better fix would be to understand what inits the struct.
#    b *0xe01c91f0
#    commands
#        silent
#        print_current_location
#        printf "EstimatedSize %d\n", ($sp + 0x18)
#        set *($r0 + 8) = 0x7d0
#        c
#    end
#    b *0xe01c923a
#    commands
#        silent
#        print_current_location
#        printf "EstimatedSize %d\n", ($sp + 0x18)
#        set *($r0 + 8) = 0x7d0
#        c
#    end
#    b *0xe01c9280
#    commands
#        silent
#        print_current_location
#        printf "EstimatedSize %d\n", ($sp + 0x18)
#        set *($r0 + 8) = 0x7d0
#        c
#    end

    rwatch *0x8488
    break *0xdf00d328
    commands
        silent
        c
    end

    #watch *0xa15ac
    # writes to 0xa15ac
    #b *0xe03f98f4
    # further up the call chain
#    b *0xe0040556
#    b *0xe00402dc

    # task_trampoline_caller logging, can we get more info out?
    # at this location, r4 points to a task struct
    b *0xdf00339a
    commands
        silent
        printf " ==== Creating task in queue\n"
        printf "unknown_01: %x\t\t", *($r4 + 0x0)
        printf "unknown_02: %x\n", *($r4 + 0x04)
        printf "run_prio: %x\t\t", *($r4 + 0x08)
        printf "entry: %x\n", *($r4 + 0x0c)
        printf "arg: %x\t\t\t", *($r4 + 0x10)
        printf "waitObjId: %x\n", *($r4 + 0x14)
        printf "unknown_03: %x\t\t", *($r4 + 0x18)
        printf "stackStartAddr: %x\n", *($r4 + 0x1c)
        printf "stackSize: %x\t\t", *($r4 + 0x20)
        printf "name: %s\n", *($r4 + 0x24)
        printf "unknown_04: %x\t\t", *($r4 + 0x28)
        printf "unknown_05: %x\n", *($r4 + 0x2c)
        printf "self: %x\t\t", *($r4 + 0x30)
        printf "unknown_06: %x\n", *($r4 + 0x34)
        printf "unknown_07: %x\t\t", *($r4 + 0x38)
        printf "unknown_08: %x\n", *($r4 + 0x3c)
        printf "taskId: %x\t\t", *($r4 + 0x40)
        printf "unknown_09: %x\n", *($r4 + 0x44)
        printf "unknown_0a: %x\t\t", *(char *)($r4 + 0x48)
        printf "currentState: %x\n", *(char *)($r4 + 0x49)
        printf "unknown_0b: %x\t\t", *(char *)($r4 + 0x4a)
        printf "yieldRequest: %x\n", *(char *)($r4 + 0x4b)
        printf "unknown_0c: %x\t\t", *(char *)($r4 + 0x4c)
        printf "sleepReason: %x\n", *(char *)($r4 + 0x4d)
        printf "unknown_0d: %x\t\t", *(char *)($r4 + 0x4e)
        printf "unknown_0e: %x\n", *(char *)($r4 + 0x4f)
        printf "requested_cpu: %x\t", *(char *)($r4 + 0x50)
        printf "assigned_cpu: %x\n", *(char *)($r4 + 0x51)
        printf "unknown_0f: %x\t\t", *(char *)($r4 + 0x52)
        printf "unknown_10: %x\n", *(char *)($r4 + 0x53)
        printf "context?: %x\t", *($r4 + 0x54)
        printf "unknown_11: %x\n\n", *($r4 + 0x58)
        # force tasks to cpu0 (should the 2nd idle task be excepted?)
        #if $_streq(*(char **)($r4 + 0x24), "init1")
        #    print "Found 'init1' task"
        #    set *(char *)($r4 + 0x50) = 0
        #end
        c
    end

    # what inits this early rom->ram copy?
    #watch *0xdf002800
    # code at 0xe0040088


    # find how cpu1 gets to failed shell state
#    watch *0x6dd38
#    watch *0x6dd48
#    b *0xe000498a thread 2
#    b *0xe0004964 thread 2
#    b *0xe000496c thread 2
#    b *0xe0004976 thread 2
#    b *0xe0004980 thread 2
    # firmware_entry
#    b *0xe0004988 thread 2
    b *0xe0040128 thread 2
#    commands
#        silent
#        set *0x6dd48 = 0x20101231
#        c
#    end
    b *0xdf006976 thread 2


    # this function can initialise something that cpu1 wants
#    b *0xdf002818

    # assert triggers soon after:
#    b *0xe016df38
    # reads what's maybe some base address into r4
    #b *0xe03f9946
    #commands
        #silent
        # this is some memory that is probably safe to write into,
        # I think it's used for buffering images
        #printf "r5: %d\n", $r5
        #set *$r5 = 0x56500000
        #c
    #end

  # This reads what look like pointers to ram from 0xa15ac.
  #
  # It's an array of 20 pointers, each pointer separated by
  # 0x90, so presumably an array of structs.
  #
  # fake those with values from real cam.  This is probably
  # a very bad idea as the ram they point to won't be init
  # as required.
#  b *0xe03f9940
#  commands
#    silent
#    set *(0xa15ac + 0x00) = 0x474a80
#    set *(0xa15ac + 0x04) = 0x474b10
#    set *(0xa15ac + 0x08) = 0x474ba0
#    set *(0xa15ac + 0x0c) = 0x474c30
#    set *(0xa15ac + 0x10) = 0x474cc0
#    set *(0xa15ac + 0x14) = 0x474d50
#    set *(0xa15ac + 0x18) = 0x474de0
#    set *(0xa15ac + 0x1c) = 0x474e70
#    set *(0xa15ac + 0x20) = 0x474f00
#    set *(0xa15ac + 0x24) = 0x474f90
#    set *(0xa15ac + 0x28) = 0x475020
#    set *(0xa15ac + 0x2c) = 0x4750b0
#    set *(0xa15ac + 0x30) = 0x475140
#    set *(0xa15ac + 0x34) = 0x4751d0
#    set *(0xa15ac + 0x38) = 0x475260
#    set *(0xa15ac + 0x3c) = 0x4752f0
#    c
#  end

end

source patch-footer.gdb
