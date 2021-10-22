# EOS M50 1.1.0
This is current platform support for M50.110.
Stubs and constants are based on R.180 and 200D.101

## Caveats, hacks
`TRASH` button is shared with `DOWN` button on M50. This is similar to M and M2.
Canon code will not emit any keycode for `TRASH` keypress in LV mode, unless you assigned a function to this key in C.Fn.
Assigned function changes keycode sent...

M and M2 uses touchscreen two-finger tap to enter ML menus. Unfortunately touch events looks different in modern era - single finger touch and two finger touch send the same keycode with (probably) different event details.
This means that existing `CONFIG_TOUCHSCREEN` is not applicable.

With that in mind, I temporary mapped ML menus to `M.Fn` button (see `BGMT_TRASH` in `gui.h`).
