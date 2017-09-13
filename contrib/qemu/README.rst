How to run Magic Lantern in QEMU?
=================================

This guide shows you how to emulate Magic Lantern (or plain Canon firmware) in QEMU.

Eager to get started? Scroll down to `Installation`_.


Current state
-------------

What works:

- Canon GUI with menu navigation - most DIGIC 4 and 5 models
- Limited support for DIGIC 2, 3, 6 and 7 models
- Limited support for some PowerShot models
- Limited support for secondary DryOS cores (such as Eeko or 5D4 AE processor)
- File I/O works on most models (both SD and CF); might be unreliable
- Bootloader emulation works on all supported models (from DIGIC 2 to DIGIC 7)
- Loading AUTOEXEC.BIN / DISKBOOT.BIN from the virtual SD/CF card (from DIGIC 2 to DIGIC 6)
- Unmodified autoexec.bin works on many single-core camera models
  (and, with major limitations, on dual-core models)
- ML modules and Lua scripts (within the limitations of the emulation)
- DryOS timer (heartbeat) and task switching (all supported models)
- UART emulation (DryOS shell aka Dry-shell or DrySh on DIGIC 4 and 5 models)
- Deterministic execution with the ``-icount`` option (SD models only)
- Cache hacks are emulated to some extent (but "uninstalling" them does not work)
- EDMAC memcpy, including geometry parameters (matches the hardware closely, but not perfectly)
- Debugging with GDB:

  - assembly level for Canon code
  - source level for ML code (if compiled with -ggdb3 or similar)
  - ML stubs can be loaded as debugging symbols for Canon code (todo: also import from IDA or other systems)
  - predefined GDB scripts (log calls to DebugMsg, task_create, register_interrupt and a few others)
  - front-ends tested: cgdb (``splitgdb.sh``), DDD, gdbgui

- Debug messages at QEMU console:

  - Use qprintf / qprint / qprintn / qdisas for printing to QEMU console
  - Compile Magic Lantern with ``CONFIG_QEMU=y``
  - By default (``CONFIG_QEMU=n``), the debug messages are not compiled
    (therefore not increasing the size of the executable that runs on the camera)

- Log various actions of the guest operating system (Canon firmware, ML):

  - execution trace: ``-d exec,nochain -singlestep``
  - I/O trace: ``-d io``
  - log various components: ``-d mpu/sflash/sdcf/uart/int``
  - list all available items: ``-d help`` 

- Built-in instrumentation (eos/dbi):

  - log all debug messages from Canon: ``-d debugmsg``
  - log all memory accesses: ``-d rom/ram/romr/ramw/etc``
  - log all function calls: ``-d calls``, ``-d calls,tail``
  - log all DryOS/VxWorks context switches
  - track all function calls to provide a stack trace: ``-d callstack``
  - export all called functions to IDC script: ``-d idc``
  - identify memory blocks copied from ROM to RAM: ``-d romcpy``
  - check for memory errors (a la valgrind): ``-d memchk``

What does not work (yet):

- LiveView (WIP, very hard);
- Still photo capture (WIP - the capture process itself works);
- Image review (WIP);
- Dual core emulation aka IPC (WIP);
- Shutdown is not clean (TODO);
- Touch screen (TODO);
- Flash reprogramming (TODO, low priority);
- Most hardware devices (audio chip, RTC, I2C, ADTG, FPGAs, JPCORE, image processing engine...);
- Properties that require MPU communication (very hard; may require emulating the MPU code);
- Lens communication (done via MPU); initial lens info is replayed on startup on some models, but that's pretty much it;
- Cache behavior is not emulated (very hard; feel free to point us to code that can be reused);
- Native Windows build (QEMU can be compiled on Windows => contribution welcome).

Common issues and workarounds:

- ML menu cannot be opened on some models

  - issue: these models do not have a dedicated DELETE button, or it's handled in a different way;
  - workaround: edit ML source code to assign a different button (sorry for that...)

- dm-spy-experiments: saving the log and anything executed afterwards may not work

  - issue: cache hacks are not emulated very well
  - workaround: compile with CONFIG_QEMU=y


Installation
------------

It is recommended to install from the `qemu <https://bitbucket.org/hudson/magic-lantern/branch/qemu>`_
branch, as it will contain the latest developments. The installation will be performed
outside the main magic-lantern directory, therefore you will be able to emulate any ML branch
without additional gymnastics (you will **not** have to merge ``qemu`` into your working branch or worry about it).

.. code:: shell

  /path/to/magic-lantern$  hg update qemu -C
  /path/to/magic-lantern$  cd contrib/qemu
  /path/to/magic-lantern/contrib/qemu$  ./install.sh
  
  # follow the instructions; you will have to supply your ROM files and compile QEMU:
  
  /path/to/qemu$  cp /path/to/sdcard/ML/LOGS/ROM*.BIN 60D/   # replace camera model with yours
  /path/to/qemu$  cd qemu-2.5.0
  /path/to/qemu/qemu-2.5.0$  ../configure_eos.sh
  /path/to/qemu/qemu-2.5.0$  make -j2
  /path/to/qemu/qemu-2.5.0$  cd ..
  
  # test your installation
  # the pre-installed SD/CF images come with a small autoexec.bin
  # (the "portable display test") that works on most supported models
  
  /path/to/qemu$  ./run_canon_fw.sh 60D,firmware="boot=1"
  
  # back to your working directory
  
  /path/to/magic-lantern$  hg update your-working-branch -C
  /path/to/magic-lantern$  cd platform/60D.111
  /path/to/magic-lantern/platform/60D.111$ make clean; make CONFIG_QEMU=y
  /path/to/magic-lantern/platform/60D.111$ make install_qemu
  
  # back to QEMU
  /path/to/qemu$  ./run_canon_fw.sh 60D,firmware="boot=1"

For reference, you may also look at `our test suite <https://builds.magiclantern.fm/jenkins/view/QEMU/job/QEMU-tests/lastSuccessfulBuild/console>`_,
where QEMU is installed from scratch every time the tests are run.
These logs can be very useful for troubleshooting.


Running Canon firmware
----------------------

From the QEMU directory, use the ``run_canon_fw.sh`` script and make sure
the `boot flag <http://magiclantern.wikia.com/wiki/Bootflags>`_ is disabled:

.. code:: shell

  /path/to/qemu$  ./run_canon_fw.sh 60D,firmware="boot=0"

Some models may need additional patches to run - these are stored under ``CAM/patches.gdb``.
To emulate these models, you will also need arm-none-eabi-gdb:

.. code:: shell

  /path/to/qemu$  ./run_canon_fw.sh 700D,firmware="boot=0" -s -S & arm-none-eabi-gdb 700D/patches.gdb

You'll probably want to see a few internals as well. To get started, try these:

.. code:: shell

  /path/to/qemu$  ./run_canon_fw.sh 60D,firmware="boot=0" -d debugmsg
  /path/to/qemu$  ./run_canon_fw.sh 60D,firmware="boot=0" -d io
  /path/to/qemu$  ./run_canon_fw.sh 60D,firmware="boot=0" -d io,int
  /path/to/qemu$  ./run_canon_fw.sh 60D,firmware="boot=0" -d io,debugmsg
  /path/to/qemu$  ./run_canon_fw.sh 60D,firmware="boot=0" -d help

Running Magic Lantern
---------------------

As you already know, Magic Lantern runs from the SD or CF card. For emulation,
we provide two card images (sd.img and cf.img) which you can mount on your operating system
and copy files on them. If these images use a FAT filesystem (they do, by default), we prefer 
`mtools <https://www.gnu.org/software/mtools/>`_ for automated tasks
(such as copying files to/from the card images without mounting them).

To install Magic Lantern to the virtual card, you may:

- mount the card image (sd.img or cf.img) as /whatever/EOS_DIGITAL,
  then run ``make install`` from your platform directory:

  .. code:: shell

    # from the magic-lantern directory
    cd platform/60D.111
    make clean; make
    # make sure your virtual card is mounted (this step is operating system specific)
    make install
    # make sure your virtual card is no longer mounted

- use ``make install_qemu`` from your platform directory
  (requires mtools, but you do not have to mount your card images)

  .. code::

    # from the magic-lantern directory
    cd platform/60D.111
    make clean; make
    make install_qemu

The included card images are already bootable for EOS firmwares (but not for PowerShots).

After you have copied Magic Lantern to the card, you may run it from the ``qemu`` directory
(near the ``magic-lantern`` one, at the same level). It's probably best to use a second terminal,
to avoid changing the directory between ML and QEMU.

.. code:: shell

  # from the qemu directory
  ./run_canon_fw.sh 60D,firmware="boot=1"
  
  # or, if your camera requires patches.gdb:
  ./run_canon_fw.sh 700D,firmware="boot=1" -s -S & arm-none-eabi-gdb 700D/patches.gdb


Incorrect firmware version?
```````````````````````````

If your camera model requires ``patches.gdb``, you may be in trouble:
many of these scripts will perform temporary changes the ROM. However,
at startup, ML computes a simple signature of the firmware,
to make sure it is started on the correct camera model and firmware version
(and print an error message otherwise, with portable display routines).
These patches will change the firmware signature - so you'll get an error message
telling you the firmware version is incorrect (even though it is the right one).

To work around this issue, you may edit src/fw-signature.h
and comment out the signature for your camera to disable this check.
Then, run ML as you already know:

.. code:: shell

  ./run_canon_fw.sh EOSM2,firmware="boot=1" -s -S & arm-none-eabi-gdb EOSM2/patches.gdb

The mere presence of a ``patches.gdb`` script in your camera subdirectory
does not automatically mean you'll get the above issue. Some patches are optional
(to fix minor annoyances such as the date/time dialog at startup - 500D, 550D, 600D, 60D),
or they may modify Canon code in a way that does not change the firmware signature (700D).



Running ML Lua scripts
----------------------

- Install ML on the virtual SD card

  .. code:: shell

    # from the qemu directory
    wget https://builds.magiclantern.fm/jenkins/job/lua_fix/415/artifact/platform/60D.111/magiclantern-lua_fix.2017Sep11.60D111.zip
    unzip magiclantern-lua_fix.2017Sep11.60D111.zip -d ml-tmp
    ./mtools_copy_ml.sh ml-tmp
    rm -rf ml-tmp/

- Run QEMU

  .. code:: shell

    ./run_canon_fw.sh 60D,firmware="boot=1"
   
- enable Debug -> Load modules after crash (workaround for incomplete shutdown emulation)
- close ML menu and reboot the virtual camera
- enable the Lua module
- close ML menu and reboot
- run the Hello World script

TODO: make api_test.lua run, fix bugs, polish the guide.

Using multiple firmware versions
--------------------------------

In most cases, Magic Lantern only supports one firmware version, to keep things simple.
However, there may be good reasons to support two firmware versions
(for example, on the 5D Mark III, there are valid reasons to choose
both `1.1.3 <http://www.magiclantern.fm/forum/index.php?topic=14704.0>`_
and `1.2.3 <http://www.magiclantern.fm/forum/index.php?topic=11017.0>`_)
or you may want to test both versions when porting Magic Lantern
`to a newer Canon firmware <https://www.magiclantern.fm/forum/index.php?topic=19417.0>`_.

The invocation looks like this (notice the ``113``):

.. code:: shell

  ./run_canon_fw.sh 5D3,firmware="113;boot=0" -s -S & arm-none-eabi-gdb -x 5D3/debugmsg.gdb

And the directory layout should be like this:

.. code::

  /path/to/qemu/5D3/113/ROM0.BIN
  /path/to/qemu/5D3/113/ROM1.BIN
  /path/to/qemu/5D3/123/ROM0.BIN
  /path/to/qemu/5D3/123/ROM1.BIN
  /path/to/qemu/5D3/113/ROM0.BIN
  /path/to/qemu/5D3/debugmsg.gdb  # common to both versions
  /path/to/qemu/5D3/patches.gdb   # common to both versions

Compare to a camera model where only one firmware version is supported:

.. code::

  /path/to/qemu/60D/ROM0.BIN
  /path/to/qemu/60D/ROM1.BIN

Automation
----------

QEMU monitor
````````````

By default, the QEMU monitor console is available by default as a UNIX socket.
That means, during emulation you can interact with it:

- with netcat (for quick commands or from a script):

  .. code:: shell

    echo "log io" | nc -U qemu.monitor

- with socat (for interactive console):

  .. code:: shell

    socat - UNIX-CONNECT:qemu.monitor

You can redirect the monitor console to stdio with... ``-monitor stdio``.

Taking screenshots
``````````````````

The easiest way is to use the ``screendump`` command from QEMU monitor.
In the following example, we'll redirect the monitor to stdio
and take a screenshot after 10 seconds.

.. code:: shell

  ( 
    sleep 10
    echo screendump snap.ppm
    echo quit
  ) | (
    ./run_canon_fw.sh 60D,firmware='boot=0' \
        -monitor stdio
  )

Another option is to use the VNC interface:

.. code:: shell

  ./run_canon_fw.sh 60D,firmware='boot=0' \
        -vnc :1234 &
  sleep 10
  vncdotool -s :1234 capture snap.png
  echo "quit" | nc -U qemu.monitor

Sending keystrokes
``````````````````

From QEMU monitor:

.. code::

  (qemu) help
  sendkey keys [hold_ms] -- send keys to the VM (e.g. 'sendkey ctrl-alt-f1', default hold time=100 ms)

.. code:: shell

  ( 
    sleep 10
    sendkey m
    sleep 1
    echo screendump menu.ppm
    echo quit
  ) | (
    ./run_canon_fw.sh 60D,firmware='boot=0' \
        -monitor stdio
  )

From VNC:

.. code:: shell

  vncdotool -h | grep key
  key KEY		send KEY to server, alphanumeric or keysym: ctrl-c, del
  keyup KEY		send KEY released
  keydown KEY		send KEY pressed

.. code:: shell

  ./run_canon_fw.sh 60D,firmware='boot=0' \
        -vnc :1234 &
  sleep 10
  vncdotool -s :1234 key m
  sleep 1
  vncdotool -s :1234 capture snap.png
  echo "quit" | nc -U qemu.monitor


Running multiple ML builds from a single command
````````````````````````````````````````````````

You may run ML builds from multiple models, unattended,
with the ``run_ml_all_cams.sh`` script:

.. code:: shell

  env ML_PLATFORMS="500D.111/ 60D.111/" \
      TIMEOUT=10 \
      SCREENSHOT=1 \
      ./run_ml_all_cams.sh

Internally, this is how the emulator is invoked:

.. code:: shell

  ( 
    sleep 10
    echo screendump 60D.111.ppm
    echo quit
  ) | (
    arm-none-eabi-gdb -x 60D/patches.gdb & 
    ./run_canon_fw.sh 60D,firmware='boot=1' \
        -display none -monitor stdio  -s -S
  ) &> 60D.111.log


This script is very customizable (see the source code for available options).

More examples:

- `EOSM2 hello world <https://builds.magiclantern.fm/jenkins/view/QEMU/job/QEMU-EOSM2/18/console>`_
- running ML from the dm-spy-experiments branch in the emulator (`QEMU-dm-spy <https://builds.magiclantern.fm/jenkins/view/QEMU/job/QEMU-dm-spy/65/consoleFull>`_)
- running the FA_CaptureTestImage test based on the minimal ML target (`QEMU-FA_CaptureTestImage <https://builds.magiclantern.fm/jenkins/view/QEMU/job/QEMU-FA_CaptureTestImage>`_)

Debugging
---------

TODO `(see EOS M2 example) <http://www.magiclantern.fm/forum/index.php?topic=15895.msg186173#msg186173>`_

Instrumentation
---------------

`TODO (see QEMU forum thread) <http://www.magiclantern.fm/forum/index.php?topic=2864.msg184125#msg184125>`_

Hacking
-------

This is bleeding-edge development used primarily for reverse engineering.
You will want to modify the sources, sooner or later.

How is this code organazized?
`````````````````````````````
.. code:: shell

  magic-lantern/contrib/qemu/eos -> qemu/qemu-2.5.0/hw/eos/  (emulation sources)
  magic-lantern/contrib/qemu/eos/dbi -> qemu/qemu-2.5.0/hw/eos/dbi (instrumentation)
  magic-lantern/src/backtrace.[ch] -> qemu/qemu-2.5.0/hw/eos/dbi/backtrace.[ch] (shared with ML)
  magic-lantern/contrib/qemu/scripts -> qemu/ (helper scripts, such as run_canon_fw.sh)
  magic-lantern/contrib/qemu/scripts/*/*.gdb -> qemu/*/*.gdb (GDB scripts)
  magic-lantern/contrib/qemu/tests -> qemu/tests (guess)

MMIO handlers: eos_handle_whatever (with io_log for debug messages).

Useful: eos_get_current_task_name/id/stack, eos_mem_read/write.

Adding support for a new camera model
`````````````````````````````````````

Start by editing ``hw/eos/model_list.c``, where you'll need to add an entry
for your camera model. The simplest one would be:

.. code:: C

    {
        .name                   = "5DS",
        .digic_version          = 6,
    },

Then, run it and follow the errors:

.. code:: shell

  ./run_canon_fw.sh 5DS
  ...
  BooMEMIF NG MR05=00000000 FROM=00000001
  BTCM Start Master

What's that? Looks like some sort of error message, and indeed, it is.
In Canon parlance, NG means "not good" - see for example ``NG AllocateMemory``
on the "out of memory" code path. Let's check whether this error message has to do
with I/O activity (usually that's where most emulation issues come from):

.. code:: shell

  ./run_canon_fw.sh 5DS -d io
  ...
  [DIGIC6]   at 0xFE020CC8:FE020B5C [0xD203040C] <- 0x500     : MR (RAM manufacturer ID)
  [DIGIC6]   at 0xFE020CC8:FE020B5C [0xD203040C] <- 0x20500   : MR (RAM manufacturer ID)
  [DIGIC6]   at 0xFE020CC8:FE020B5C [0xD203040C] -> 0x0       : MR (RAM manufacturer ID)
  MEMIF NG MR05=00000000 FROM=00000001
  BTCM Start Master

OK, so the message appears to be related to these I/O registers.
Look up the code that's handling them (search for "RAM manufacturer ID").
You'll find it in eos.c:eos_handle_digic6, at the register 0xD203040C
(as expected), and you'll find it uses a model-specific constant:
``s->model->ram_manufacturer_id``. Let's look it around to see what's up with it:

.. code:: C

  .name                   = "80D",
  .ram_manufacturer_id    = 0x18000103,   /* RAM manufacturer: Micron */

  .name                   = "750D",
  .ram_manufacturer_id    = 0x14000203,

  .name                   = "5D4",
  .ram_manufacturer_id    = 0x18000401,

Good - it's now clear you'll have to find this constant. You have many choices here:

- disassemble the ROM near the affected address,
  and try to understand what value Canon code expects from this register
- use pattern matching and find it from nearby model
- try the values from another camera model, hoping for the best
- trial and error

Let's go for the last one (probably the easiest). If you look at the code,
you may notice the "5" corresponds to the least significant byte in this RAM ID.
If you didn't, don't worry - you can just try something like 0x12345678:

and the error message will tell you the answer right away:

.. code::

  MEMIF NG MR05=00000078 FROM=00000001

You now have at most 4 test runs to find this code :)

A more complete example: the `EOS M2 walkthrough <http://www.magiclantern.fm/forum/index.php?topic=15895.msg185103#msg185103>`_
shows how to add support for this camera from scratch, until getting Canon GUI to boot (and more!)

Although this model is already supported in the repository,
you can always roll back to an older changeset and follow the tutorial.

Any good docs on QEMU internals?
````````````````````````````````

- http://nairobi-embedded.org/category/qemu.html
- http://blog.vmsplice.net
- QEMU mailing list (huge!)
- Xilinx QEMU

MPU spells
``````````

`TODO <http://www.magiclantern.fm/forum/index.php?topic=2864.msg166938#msg166938>`_

Committing your changes
```````````````````````

After editing the sources outside the magic-lantern directory, 
first make sure you are on the ``qemu`` branch:

.. code:: shell

  # from magic-lantern directory
  hg up qemu -C

Then copy your changes back into ML tree:

.. code:: shell

  # from qemu directory
  ./copy_back_to_contrib.sh

Then commit as usual, from the ``contrib/qemu`` directory.

Reverting your changes
``````````````````````

If you want to go back to an older changeset, or just undo your edits
done outside the magic-lantern directory, you may run the install script
again. It will not re-download QEMU, but unfortunately you will have to
recompile QEMU from scratch (which is very slow).

If you have changed only the ``eos`` files, to avoid a full recompilation
you may try a script along these lines:

.. code:: shell

    #!/bin/bash
    
    QEMU_PATH=${QEMU_PATH:=qemu-2.5.0}
    ML=${ML:=magic-lantern}
    
    cp -v ../$ML/contrib/qemu/eos/* $QEMU_PATH/hw/eos/
    cp -v ../$ML/contrib/qemu/eos/mpu_spells/* $QEMU_PATH/hw/eos/mpu_spells/
    cp -v ../$ML/contrib/qemu/eos/dbi/* $QEMU_PATH/hw/eos/dbi/
    cp -v ../$ML/src/backtrace.[ch] $QEMU_PATH/hw/eos/dbi/
    cp -vr ../$ML/contrib/qemu/tests/* tests/
    cp -vr ../magic-lantern/contrib/qemu/scripts/* .


Test suite
``````````

Most Canon cameras are very similar - which is why one is able to run the same codebase
from DIGIC 2 (original 5D) all the way to DIGIC 5 (and soon 6) - yet, every camera model has its own quirks
(not only on the firmware, but also on the hardware side). Therefore, it's hard to predict whether a tiny change in the emulation, to fix a quirk for camera model X,
will have a negative impact on camera model Y. The test suite tries to answer this,
and covers the following:

- Bootloader code (to make sure AUTOEXEC.BIN is loaded from the card)
- Portable display test (all EOS models)
- Portable ROM dumper (EOS models with bootloader file write routines)
- Menu navigation (on supported models) - depends on user settings from the ROM
- Card formatting (and restoring ML)
- Call/return trace until booting the GUI (a rigid test that may have to be updated frequently)
- Call/return trace on bootloader (likely independent on firmware version and user settings)
- Callstack consistency with call/return trace (at every DebugMsg call)
- File I/O (whether the firmware creates a DCIM directory on startup)
- FA_CaptureTestImage (basic image capture process, without compression or CR2 output)
- HPTimer (difficult to get right)
- GDB scripts (just a few basics)
- DryOS shell (UART)
- PowerShot models (limited tests)
- Secondary DryOS cores (limited tests)

Limitations:

- The tests are tied to our copies of the ROMs (which also store various user settings);
  unfortunately, these ROMs are not public (see `ML FAQ <http://wiki.magiclantern.fm/faq>`_).
  
  Workarounds:
  
  - run the test suite for your camera model(s) only, e.g. ``./run_tests.sh 5D3 60D 70D``
  - inspect the test results (e.g. screenshots) manually, and compare them to our results from Jenkins to decide whether they are correct or not
  - if you have made changes to the emulation, just ask us to test them.

  Saving the ROM right after clearing camera settings may or may not give repeatable results (not tested).

- The test suite is very slow (30-60 minutes, even on decent hardware)

  Workarounds:

  - run the test suite for a small number of camera model(s): ``./run_tests.sh 5D3 60D 70D``
  - run only the test(s) you are interested in (add ``if false; then`` ... ``fi`` in the source)
  - leave the tests running overnight.

  If you have ideas to improve it, we are listening.

To avoid committing (large) reference screenshots or log files,
a lot of expected test results are stored as MD5 sums. That's a bit rigid,
but it does the job for now. Where appropriate, we also have grep-based
tests or custom logic on log files.

The expected test results ("needles") are updated manually
(e.g. ``md5sum disp.ppm > disp.md5``). Suggestions welcome.

Code coverage?
``````````````

`Yes <https://builds.magiclantern.fm/jenkins/view/QEMU/job/QEMU-coverage/>`_.

History
-------

:2008: `SD1100 boot (CHDK) <http://chdk.wikia.com/wiki/GPL_Qemu>`_
:2009: `5D2/7D boot (Trammell) <http://magiclantern.wikia.com/wiki/Emulation>`_
:2012: `TriX_EOS (g3gg0) <http://www.magiclantern.fm/forum/index.php?topic=2882.0>`_
:2013: `Initial Hello World <http://www.magiclantern.fm/forum/index.php?topic=2864.msg26022#msg26022>`_
:2013: `g3gg0 ports TriX changes to QEMU <http://www.magiclantern.fm/forum/index.php?topic=2864.msg29748#msg29748>`_
:2013: `Antony Pavlov submits initial DIGIC support to QEMU mainline <https://lists.gnu.org/archive/html/qemu-devel/2013-08/msg04509.html>`_
:2013: `Nikon Hacker is light years ahead us <http://www.magiclantern.fm/forum/index.php?topic=8823.0>`_ (we are not competing; it was just a fun notice that motivated us)
:2014: `DryOS task scheduler running! <http://www.magiclantern.fm/forum/index.php?topic=2864.msg117430#msg117430>`_ (also ML menu and modules, but with massive hacks - emulating only a very small part of Canon firmware)
:2015: `Portable display test and Linux PoC working! <http://www.magiclantern.fm/forum/index.php?topic=2864.msg144760#msg144760>`_
:2015: `Canon GUI boots on 60D! <http://www.magiclantern.fm/forum/index.php?topic=2864.msg148240#msg148240>`_ (no menus yet, but most Canon tasks are working!)
:2015: `100D emulation, serial flash and GDB scripts from nkls <http://www.magiclantern.fm/forum/index.php?topic=2864.msg153064#msg153064>`_
:2016: `More EOS models boot Canon GUI (no menus yet) <http://www.magiclantern.fm/forum/index.php?topic=2864.msg168603#msg168603>`_
:2016: `Low-level button codes and GUI modes understood <http://www.magiclantern.fm/forum/index.php?topic=2864.msg169517#msg169517>`_
:2016: `Users start wondering why the heck are we spending most of our time on this <http://www.magiclantern.fm/forum/index.php?topic=2864.msg169970#msg169970>`_
:2016: `Leegong from Nikon Hacker starts documenting MPU messages <http://www.magiclantern.fm/forum/index.php?topic=17596.msg171304#msg171304>`_
:2017: `500D menu navigation! (Greg) <http://www.magiclantern.fm/forum/index.php?topic=2864.msg179867#msg179867>`_
:2017: `nkls solves an important issue that was very hard to track down! <http://www.magiclantern.fm/forum/index.php?topic=2864.msg183311#msg183311>`_
:2017: `Menu navigation works on most D4 and 5 models <http://www.magiclantern.fm/forum/index.php?topic=2864.msg181786#msg181786>`_
:2017:  Working on `Mac (dfort) <http://www.magiclantern.fm/forum/index.php?topic=2864.msg184981#msg184981>`_ 
        and `Windows 10 / Linux subsystem (g3gg0) <http://www.magiclantern.fm/forum/index.php?topic=20214.0>`_
:2017: `EOS M2 porting walkthrough <http://www.magiclantern.fm/forum/index.php?topic=15895.msg185103#msg185103>`_



Happy hacking!
