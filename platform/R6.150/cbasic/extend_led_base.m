' https://discord.com/channels/671072748985909258/761652283724922880/792460312561188905
' Get R6 LED base

private sub save_led_base(fileName)
  RemoveFile(fileName)

  f = OpenFileCREAT(fileName)
  CloseFile(f)

  f = OpenFileWR(fileName)

  pLedBase = 0x4F54
  WriteFileString(f, "LED base 1: [0x%08X]: 0x%08X\n", pLedBase, *pLedBase)

  pLedBase = 0x4F5C
  WriteFileString(f, "LED base 2: [0x%08X]: 0x%08X\n", pLedBase, *pLedBase)

  CloseFile(f)
end sub

private sub Initialize()
  save_led_base("B:/LED_BASE.TXT")
end sub