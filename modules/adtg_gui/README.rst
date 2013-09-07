ADTG register GUI
=================

ADTG and CMOS register editing GUI.

** Warning: this is not a toy; it can destroy your sensor. **

This is a tool for reverse engineering the meaning of ADTG/CMOS registers (low-level sensor control).

:License: GPL
:Author: a1ex
:Credits: g3gg0 (adtg_log)
:Summary: ADTG/CMOS register editing GUI (reverse engineering tool)

Usage 1/3
---------

To use it, you need to set CONFIG_GDB=y in Makefile.user. There are no binaries available (and shouldn't be, for safety reasons).

All intercepted registers are displayed after Canon code touches them:

* [photo mode] first enable logging - simply open the ADTG registers menu - then take a picture, for example, then look in menu again
* [LiveView] some registers are updated continuously, but there are a lot more that are updated when changing video modes or when going in and out of LiveView (so, to see everything, first open ADTG menu to enable logging, then go to LiveView, then look in menu again)

Usage 2/3
---------

For registers that we have some idea about what they do, it displays a short description:

* you can add help lines if you understand some more registers

You can display diffs:

* e.g. enable logging, take a pic, select "show modified registers", change ISO, take another pic, then look in the menu

Usage 3/3
---------

You can override any register:

* if you don't have dual ISO yet on your camera, just change CMOS[0] manually, then take pics ;)
* you can find some funky crop modes (e.g. if you change the line skipping factor)
* now it's easier than ever to kill your sensor for science

If in doubt, **take the battery out. Quickly!** (well, that's what I do)

Tip: some registers use NRZI values (they are displayed with a N), others use normal values. If the value doesn't make sense (e.g. something affects brightness, but it seems kinda random, not gradual changes), try flipping the is_nrzi flag from known_regs. You can't do it from the menu yet.
