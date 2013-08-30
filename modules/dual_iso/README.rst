Dual ISO
========

Increases dynamic range by sampling half of sensor lines at ISO 100 and the other half at ISO 1600 (or other user-defined values).
This trick cleans up shadow noise, resulting in a dynamic range improvement of around 3 stops,
at the cost of reduced vertical resolution, aliasing and moire.

Works for both raw photos and raw videos. You need to postprocess CR2 files with cr2hdr and raw video files with raw2dng.

After postprocessing, you get a DNG that looks like an ISO 100 shot,
with very clean shadows that you can boost in post.
You will not get any radioactive HDR look by default - for that you will need Photomatix :)
 
