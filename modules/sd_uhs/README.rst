SD overclocking
===============

Change SD UHS access speeds, partly via accessing the SD device directly,
partly via patching ROM used for normal DryOS configuration of the device.
On DIGIC 4 SD bodies, the module reprograms PLL1 at 0xC04000BC instead.

This is not well understood, mostly empirically determined.  There is
definite risk.  Field tests suggest it is not very risky in practice;
very few reports of damaged cards, some reports of lost data.  Problems
are more likely to occur at higher speeds.  Some cards tolerate higher
speeds much better than others.

:Author: a1ex, Danne, theBilalFkahouri, wavesoft
:License: GPL
:Forum: http://www.magiclantern.fm/forum/index.php?topic=12862.0

