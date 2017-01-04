Serial Flash Dumper
===================

This module will attempt to dump the serial flash of the device.
It is intended for DIGIC 5 and 6 cameras, which store their properties
on a non-DMA flash chip, accessed over SPI.

Usage: Run from Debug menu.

For QEMU, copy ML/LOGS/SFDATA.BIN to qemu/CAM/SFDATA.BIN (near ROM1.BIN).

:Author: nkls
:License: GPL
:Summary: Serial flash dumper.
