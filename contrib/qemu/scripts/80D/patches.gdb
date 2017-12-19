# ./run_canon_fw.sh 80D -s -S & arm-none-eabi-gdb -x 80D/patches.gdb
# Only patches required for emulation

source patch-header.gdb

# UTimer (fixme)
set *(int*)0xFE5998C6 = 0x4770

# experimental patches
# they probably do more harm than good - figure out what's up with them
if 1
  # startupPrepareCapture: pretend OmarInit was completed
  set *(int*)0xFE0D91C8 = 0x4770

  # SHT_CAPTURE_PATH_InitializeCapturePath
  set *(int*)0xFE1C0BF4 = 0x4770

  # startupPrepareDevelop
  set *(int*)0xFE0D52C2 = 0x4770
end

source patch-footer.gdb
