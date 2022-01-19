# Canon basic examples

These samples has been tested successfully on R6.

Introduction

just rename them to *extend.m*

to setup card, simplest is to use Kitor sdcard image : https://www.magiclantern.fm/forum/index.php?topic=25305.msg230427#msg230427 

## dumping

- extend_dump_mem.m : dump specific memory range
- extend_dump_rom.m : dump ROM

## cpuinfo (srsa / CHDK)

- cpuinfo.s : compiles to cpuinfo.elf
- extend_cpu_info.m : use cpuinfo.bin 
- CPUINFO.DAT : output of extend_cpu_info.m
- cpuinfo_out.txt : CPUINFO.DAT parsed as text

see https://www.magiclantern.fm/forum/index.php?topic=25305.msg230668#msg230668

## Led / GPIO (coon)

extend_led_base.m
extend_led_gpio.m
LED_BASE.TXT
LED_GPIOS.TXT

See https://discord.com/channels/671072748985909258/761652283724922880/792460312561188905

and https://discord.com/channels/671072748985909258/761652283724922880/792460167433551902

## Bootdisk

extend_bootdisk.m : to enable or disable Bootdisk (to boot magiclantern.bin on prepared card, with EOScard)



