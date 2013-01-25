// Custom bracketing sequence
// ISO 3200 1/10 f1.8
// ISO 100 1/10 f8
// ISO 100 1" f8
// ISO 100 10" f8

console_hide();

set_aperture(1.8);
set_iso(3200);
set_shutter(1./10);
takepic();

set_aperture(8);
set_iso(100);
takepic();

set_shutter(1);
takepic();

set_shutter(10);
takepic();

