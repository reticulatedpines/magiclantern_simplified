# PowerShot SX740 HS 
This is current platform support for SX740.102
Stubs and constants are based on R.180

## Caveats, hacks
`TRASH` button is shared with `FULLSCREEN` button on SX740.

There's also useless `WIRELESS` button - as function is also accessible via main menu.

Thus `Trash` button is now mapped to `WIRELESS` button.

## Test features

`test_features.c` provides a way to enable CR3 RAW shooting on this model.

Please note that most programs won't accept this RAW as they don't know what to do with it.

As-is, UART will be spammed by Canon GUI code that doesn't know how to interpret RAW config, but it will make photos fine.

To revert to JPEG, just use normal Canon menu to change picture quality.
