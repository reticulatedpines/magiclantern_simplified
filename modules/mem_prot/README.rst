Memory Protection
=================

This module enables ARM memory protection and configures it to
protect the first 4KiB so that NULL pointer accesses will get caught.
As Canon's own code often causes NULL pointers to be dereferenced, 
this will happen quite often - especially in LV mode.

:License: GPL
:Summary: Protect from NULL pointer accesses
:Authors: g3gg0
