# CreateSemaphore IN
br *0x72F8
commands
  silent
  set *0xCF999010 = *0xCF999010
  c
end

# CreateSemaphore OUT
br *0x7350
commands
  silent
  set *0xCF999014 = *0xCF999014
  c
end

# TakeSemaphore IN
br *0x7458
commands
  silent
  set *0xCF999018 = *0xCF999018
  c
end

# TakeSemaphore OUT
br *0x750C
commands
  silent
  # Semaphore id
  set $r1 = $r4
  set *0xCF99901C = *0xCF99901C
  c
end


# GiveSemaphore
br *0x7544
commands
  silent
  set *0xCF999020 = *0xCF999020
  c
end

