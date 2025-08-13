# Building Magic Lantern

## Overview

The Magic Lantern build system is a fairly standard GNU Make system, with some calls out to Python 3 scripts.  It should be safe to use parallel make jobs, e.g. `make -j6`.

People have succeeded in building ML on Windows (via WSL), OSX, and Linux.

The standard output is a zip file, that can be extracted to a memory card ready to run on a camera.  This contains a variety of assets, such as bitmap images, fonts, Lua scripts.  It also includes binary modules, optional components that can be selected to load by the user at runtime.  Finally, there is one primary binary, always named autoexec.bin.

Autoexec.bin is a name checked for by DryOS, and as explained in section 3, can be used to load our code (given some other conditions being true).

There are three globally important files:
```
/Makefile.globals
/platform/Makefile
/modules/Makefile
```

There is also a set of per cam files, one per cam, e.g.:
```
/platform/5D3.123/Makefile
```


### Examples

```
cd platform/5D3.123
make -j6
```
This builds for 5D3 only, and will also trigger a build of all modules as required.
6 jobs are used, typically speed-up is high up to the number of cores on the system, and still useful up to the number of threads.
Running `make clean` here only cleans this cam, not modules.
Build output is produced in `platform/5D3.123/build`, the main file being `magiclantern.zip`.


```
cd platform
make -j6
```
This builds for all cams listed in the ML\_CAMS var inside `platform/Makefile` (essentially, all cams known to work well; the default set of cams to produce builds for).
Running `make clean` here cleans `platform/build` and all per cam build dirs.  Modules are not cleaned.
Build output is produced in `platform/build`.


```
cd modules/bench
make -j6
```
This builds the benchmark module only.  Running `make clean` here cleans only this module.
Output goes to `modules/bench/build/`.


```
cd modules
make -j6
```
This builds all modules, but no cams.  Running `make clean` here cleans `build/` and all module build dirs.
Output goes to `modules/build/`.


### Environment variables

The build system can produce different kinds of builds based on environment variables.  This can be a confusing pattern.  Work in 2024 reduced the number of available env vars, a few relevant ones remain:

`CONFIG_QEMU`: if set to y, various fixups to make Magic Lantern code work better in Qemu are performed, as well as extra debugging statements only visible in Qemu.  These builds *will not* run on physical cams.  They will typically hang the cam until battery is removed.  The extra debug output is very useful when diagnosing ML loader problems in qemu, if you're working in this area it's recommended you always run such tests before making a build for a physical cam.

`FATAL_WARNINGS`: if set to y, compiler warnings are elevated to errors.  Good for dev work.  It is expected that all supported cams and modules will compile with this setting.  If not, a bug can be raised and it will be fixed (assuming the build environment was sane).

Thus, to use both options and build for one cam:
```
cd platform/5D3.123
make CONFIG_QEMU=y FATAL_WARNINGS=y -j6
```

Which env vars were used for a build are not tracked as build dependencies.  This means if you change the options in use, you must manually run `make clean` over the relevant parts.  If you don't, you may have stale binary components, which can lead to dangerous behaviour.

<div style="page-break-after: always; visibility: hidden"></div>
