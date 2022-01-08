# EOS R
This is current platform support for EOS R 1.8.0 ( 7.3.9 )

## Caveats
There are **TWO DIFFERENT VERSIONS** of 1.8.0 ROM:  7.3.9 and 7.4.0. Thanks, Canon!

First one - **all the development is based on this version**:
[https://gdlp01.c-wss.com/gds/8/0400006288/01/eosr-v180-win.zip](https://gdlp01.c-wss.com/gds/8/0400006288/01/eosr-v180-win.zip)
```
K424 ICU Firmware Version 1.8.0 ( 7.3.9 )
ICU Release DateTime Jul 27 2020 18:48:59
```
Second one, newer:
[https://gdlp01.c-wss.com/gds/8/0400006288/02/eosr-v180-win.zip](https://gdlp01.c-wss.com/gds/8/0400006288/02/eosr-v180-win.zip)
```
K424 ICU Firmware Version 1.8.0 ( 7.4.0 )
ICU Release DateTime Feb 4 2021 17:01:29
```

**You need to have a correct version installed for Magic Lantern to run.** At least it is possible to "downgrade" 1.8.0 7.4.0 to 1.8.0 7.3.9 without any "card swap" tricks - camera will just accept older firmware.
## How to check which "minor" version am I running right now?
Run this Canon Basic script [1]:
```
private sub Initialize()
    CamInfo_Debug(1)
end sub
```

It will create `CAM_INFO.XML` on your card. In that file you can find:

```
<FirmwareVer>
    <Internal>0.7.3.9</Internal>
    <Major>1.8.0</Major>
</FirmwareVer>
```

[1] How to run Canon Basic scripts: [https://www.magiclantern.fm/forum/index.php?topic=25305.0](https://www.magiclantern.fm/forum/index.php?topic=25305.0)
