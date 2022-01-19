' https://discord.com/channels/671072748985909258/761652283724922880/792460167433551902
' LED GPIO dumper for EOS R6
dim gpio_base      = 0xD2239000
dim led_obj_ptr    = 0x4914
dim led_obj_offset = 0x24
dim led_count_max  = 56

private sub save_led_gpio_addresses(fileName)
  RemoveFile(fileName)

  f = OpenFileCREAT(fileName)
  CloseFile(f)

  f = OpenFileWR(fileName)
  pLedObjectBase = *led_obj_ptr

  ledKind = 0

  do while ledKind < led_count_max
    pLedObject = pLedObjectBase + ledKind * led_obj_offset
    ledGpioOffset = pLedObject + 8

    WriteFileString(f, "[0x%08X] LED %d: 0x%08X\n", pLedObject, ledKind, gpio_base + *ledGpioOffset)
    ledKind = ledKind + 1
  loop

  CloseFile(f)
end sub

private sub Initialize()
  save_led_gpio_addresses("B:/LED_GPIOS.TXT")
end sub