EOS Utility tether (EOS M)
==========================

EOS M 2.0.2 only. Lets Canon EOS Utility 2 offer "Camera settings/Remote shooting"
and live view over USB. RAM only, nothing is flashed.

It runs the Canon event procedure FA_SetRemoteSpecialMode(1), which registers the
remote shooting PTP opcodes, and reports the 650D model ID (0x80000301) in the
camera model property, which is what EOS Utility 2 uses to enable the feature.

:Author: Laptopsftw
:License: GPL
