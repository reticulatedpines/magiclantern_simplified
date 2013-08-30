Dual ISO
========

Increases dynamic range by sampling the sensor at two different ISOs, switching ISO for every other line pair.
This trick cleans up shadow noise, resulting in a dynamic range improvement of around 3 stops,
at the cost of reduced vertical resolution, aliasing and moire.

Works for both raw photos (CR2) and raw videos (DNG sequences). You need to postprocess these files with a tool called **cr2hdr**.

After postprocessing, you get a DNG that looks like a dark ISO 100 shot,
but you can bring the exposure back up and be delighted by how little noise is present in those recovered shadows.
