
Dual ISO
========

:Author: a1ex
:License: GPL
:Summary: Dynamic range improvement by using two ISOs in one picture
:Forum: http://www.magiclantern.fm/forum/index.php?topic=7139.0

Increases dynamic range by sampling the sensor at two different ISOs, switching ISO for every other line pair.
This trick cleans up shadow noise, resulting in a dynamic range improvement of around 3 stops,
at the cost of reduced vertical resolution, aliasing and moire.

Works for both raw photos (CR2) and raw videos (DNG sequences). You need to postprocess these files with a tool called **cr2hdr**.

After postprocessing, you will get a DNG that looks like a dark ISO 100 shot,
but you can bring the exposure back up and be delighted by how little noise is present in those recovered shadows.

Quick start
-----------

* Start at ISO 100 in Canon menu
* Expose to the right by changing shutter and aperture
* If the image is still dark, enable dual ISO
* Adjust recovery ISO: higher values = cleaner shadows, but more artifacts
* Try not to go past ISO 1600; you will not see any major improvements, 
  but you will get more interpolation artifacts and hot pixels.

Tips and tricks
---------------

* Do not use dual ISO for regular scenes that don't require a very high dynamic range.
* Raw zebras are aware of dual ISO: weak zebras are displayed where only the high ISO is overexposed,
  strong (solid) zebras are displayed where both ISOs are overexposed.
* Raw histogram will display only the low-ISO half of the image (since the high-ISO data is used
  for cleaning up shadow noise).
* For optimal exposure (minimal noise without clipped highlights), try both dual ISO and ETTR.
* Do not be afraid of less aggressive settings like 100/400. They are almost as good as 100/1600 
  regarding shadow noise, but with much less aliasing artifacts.
* Be careful with long exposures, you may get lots of hot pixels.

