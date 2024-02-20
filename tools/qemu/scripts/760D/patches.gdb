# ./run_canon_fw.sh 760D -s -S & arm-none-eabi-gdb -x 760D/patches.gdb -ex quit
# Only patches required for emulation

source patch-header.gdb

# experimental patches
# they probably do more harm than good - figure out what's up with them
if 1
  # startupPrepareCapture: pretend OmarInit was completed
  set *(int*)0xFE0CFA7A = 0x4770

  # EstimatedSize
  b *0xFE20310A
  commands
    silent
    print_current_location
    printf "EstimatedSize %d\n", $r0
    set $r0 = 0x7D0
    c
  end
end

source patch-footer.gdb
