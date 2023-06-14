# PowerShot SX70 HS
This is current platform support for SX70 HS 1.1.1.
Stubs and constants are based on R.180

Please note that code is less stable than R / SX740HS.
I double-verified the stubs and have no idea why.

## Caveats, hacks
`TRASH` button is shared with DOWN button on d-pad and changes depending on context.
This makes it unsuitable for use to enter Magic Lantern menu.

There's also useless `WIRELESS` button - as this function is also accessible via main menu.

Thus `Trash` button is now mapped to `WIRELESS` button.
