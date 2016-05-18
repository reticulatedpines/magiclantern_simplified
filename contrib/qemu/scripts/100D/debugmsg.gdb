# DryosDebugMsg - QEMU hook
br *0x4A74
commands
  silent
  set *0xCF999001 = *0xCF999001
  c
end

