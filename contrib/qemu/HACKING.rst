.. If you see this (unformatted) text on Bitbucket, please try reloading the page.

EOS firmware in QEMU - development and reverse engineering guide
================================================================

For "user" documentation and installation guide, please see the main `README.rst <README.rst>`_.

-----------

This is bleeding-edge development used primarily for reverse engineering.
You will want to modify the sources, sooner or later.

.. contents::

How is this code organazized?
`````````````````````````````

By default, the install script sets up QEMU by creating a ``qemu-eos`` subdirectory
"near" the ``magic-lantern`` directory (so both directories will end up at the same level).
The default directory structure looks like this::

  .
  ├── magic-lantern/            # Magic Lantern working directory
  │   │
  │   ├── src/                      # main ML source files
  │   │
  │   ├── platform/                 # camera-specific (platform) directories
  │   │   ├── 5D3.113                   # camera model . firmware version
  │   │   ├── EOSM.202                  # here you would run "make", "make install_qemu" etc
  │   │   └── ...
  │   │
  │   ├── minimal/                  # minimal targets, for experiments
  │   │   ├── hello-world           # these will use other files from the platform directory
  │   │   ├── qemu-frsp             # compile with "make MODEL=EOSM" or "make MODEL=5D3 FW_VERSION=123"
  │   │   └── ...
  │   │
  │   ├── installer/                # for building ML-SETUP.FIR (which enables/disables the boot flag)
  │   └── ...
  │
  └── qemu-eos/                 # QEMU working directory
      │
      ├── qemu-2.5.0/               # QEMU sources and binaries
      │
      ├── 60D/                      # camera-specific subdirectories
      │   ├── ROM*.BIN                  # ROM files (user-supplied)
      │   └── debugmsg.gdb              # GDB script
      ├── EOSM2/
      │   ├── ROM*.BIN                  # ROM files (user-supplied)
      │   ├── SFDATA.BIN                # serial flash contents (recent models use one)
      │   ├── patches.gdb               # patches required for emulation (for tricky models)
      │   └── debugmsg.gdb              # GDB script
      ├── 5D3/
      │   ├── 113/                      # firmware-specific files
      │   │   └── ROM*.BIN              # ROM files (user-supplied)
      │   ├── 123/
      │   │   └── ROM*.BIN              # ROM files (user-supplied)
      │   └── debugmsg.gdb              # GDB script (common)
      │
      ├── ...
      ├── tests/                    # our test suite
      │   ├── 60D/                      # model-specific test files
      │   └── ...                       # (expected and actual results)
      │
      ├── run_canon_fw.sh           # main script for running the emulation
      └── ...                       # other scripts / utilities

The sources are stored in the Magic Lantern tree, under ``contrib/qemu``. Our modifications to QEMU sources
are stored as a patch file (``qemu-2.5.0.patch``), while the new files are stored directly. The install script
copies the following files:

.. code:: shell

  magic-lantern/contrib/qemu/eos/ -> qemu-eos/qemu-2.5.0/hw/eos/  (emulation sources)
  magic-lantern/contrib/qemu/eos/mpu_spells/ -> qemu-eos/qemu-2.5.0/hw/eos/mpu_spells/  (MPU messages, button codes)
  magic-lantern/contrib/qemu/eos/dbi/ -> qemu-eos/qemu-2.5.0/hw/eos/dbi/ (instrumentation)
  magic-lantern/src/backtrace.[ch] -> qemu-eos/qemu-2.5.0/hw/eos/dbi/backtrace.[ch] (shared with ML)
  magic-lantern/contrib/qemu/scripts/ -> qemu-eos/ (helper scripts, such as run_canon_fw.sh)
  magic-lantern/contrib/qemu/scripts/*/debugmsg.gdb -> qemu-eos/*/debugmsg.gdb (GDB scripts for reverse engineering)
  magic-lantern/contrib/qemu/scripts/*/patches.gdb -> qemu-eos/*/patches.gdb (patches required for emulation — only on some models)
  magic-lantern/contrib/qemu/tests -> qemu-eos/tests (guess)

Customizing directories
'''''''''''''''''''''''

Once QEMU is compiled into some subdirectory (such as ``/path/to/qemu-eos/qemu-2.5.0/``),
it will no longer work elsewhere (you will not be able to rename/move this directory
without a full reconfiguration and recompilation).

Should you want to customize these paths, you may set the following environment variables:

- ``QEMU_DIR``: defaults to ``qemu-eos`` (QEMU working directory, created near ``magic-lantern``)
- ``QEMU_NAME``: defaults to ``qemu-2.5.0`` (a subdirectory under your ``qemu-eos`` directory)
- ``ML_PATH``: defaults to ``../magic-lantern``, relative to your ``qemu`` directory.

Tip: after installation, you may change ``ML_PATH`` to emulate ML from other directories, located anywhere in the filesystem.

When using ``make install_qemu``, the Makefiles will also find the QEMU working directory from ``QEMU_DIR``.

Basic concepts
``````````````

Some parts were adapted from `Jake Sandler's excellent operating system tutorial for Raspberry Pi <https://jsandler18.github.io>`_.

Memory-mapped I/O, peripherals and registers
''''''''''''''''''''''''''''''''''''''''''''

Adapted from https://jsandler18.github.io/extra/peripheral.html

**Memory-mapped I/O** or **MMIO** is a way of interacting with hardware devices
by reading from and writing to predefined memory addresses.
All interactions with the DIGIC hardware happen using MMIO.

.. _peripheral:

A **Peripheral** is a `hardware device <https://barrgroup.com/Embedded-Systems/Books/Programming-Embedded-Systems/Peripherals-Device-Drivers>`_
used in embedded systems, in addition to processor and memory. Some peripherals, such as timers or interrupt controller,
are often included in the same chip as the processor; others, such as the real-time clock or LCD controller, are usually external.
The firmware interacts with peripherals through specific MMIO address(es) in the memory space.

Each peripheral has a (hardcoded) range of memory addresses. On Canon hardware, this region is generally located
somewhere within ``0xC0000000 - 0xDFFFFFFF`` (with variations: ``C0000000 - CFFFFFFF``, ``C0000000 - C0FFFFFF`` and so on).

A **Register** is a 32-bit wide (4-byte) location in some peripheral's address range, used to control that peripheral.
These registers are at predefined offsets from the peripheral’s base address.
It is quite common for at least one register to be a control register,
where each bit in the register corresponds to a certain behavior that the hardware should have.
Another common register is a write register, where anything written in it gets sent off to the hardware.
Some peripherals also have a status register (which may be either read-only or shared with a control register).

For example, there are 8 DMA channels placed at ``0xC0A10000-0xC0A100FF``,
``0xC0A20000-0xC0A200FF``, ..., ``0xC0A80000-0xC0A800FF``. All these DMA channels
share the same behavior; moreover, they are controlled by registers located in the above ranges.
For example, at offset ``0x08`` you will find the control register (``0xC0A10008``, ``0xC0A20008``, ..., ``0xC0A80008``),
offset ``0x18`` is the source address, ``0x1C`` is the destination address
and offset ``0x20`` is the transfer size (see ``eos_handle_dma`` in ``eos.c``).

Figuring out where all the peripherals are, what registers they have
and how to use them, is difficult — there's no documentation on DIGIC hardware.
One may start analyzing Canon code that uses these peripherals (what values are written to them,
what values are expected to be read, what the hardware is supposed to do with them)
and by `cross-checking the register values with those obtained on physical hardware`__ (by logging what Canon code does).
Generally, the behavior of these peripherals is common across many camera models; very often,
compatibility is maintained across many generations of the hardware. For example, a 20-bit microsecond timer
("DIGIC timer") can be read from register ``0xC0242014`` on all EOS and PowerShot models from DIGIC 2 to DIGIC 5.

__ `Cross-checking the emulation with actual hardware`_

See `Working out all the way to Canon GUI`_ for some examples of figuring out what certain peripherals are supposed to do.

Hardware interfaces are generally compatible between EOS and PowerShot models. For example,
EDMAC (image processing DMA) works the same at hardware level on both EOS and PowerShot
(therefore, the same emulation code can be reused for both platforms);
however, the front-end functions used in the firmware are different
(that makes porting CHDK on EOS models or Magic Lantern on PowerShot models a non-trivial task).

Documentation for certain off-the-shelf peripherals (such as RTC, audio chip, serial flash)
is available (`Datasheets <http://magiclantern.wikia.com/wiki/Datasheets>`_,
`Circuit boards <http://magiclantern.wikia.com/wiki/Circuit_boards>`_ and `photo-parts.ua <https://photo-parts.com.ua/parts/?part=550D>`_).
For this purpose, high-resolution pictures of (your) camera mainboards are always welcome.

MMIO register activity can be logged by running the emulation with ``-d io``.

What we know about these registers can be found in emulator sources, starting at the ``eos_handlers`` table,
and on the `Register Map <http://magiclantern.wikia.com/wiki/Register_Map>`_ wiki page.

Interrupts and exceptions
'''''''''''''''''''''''''

Adapted from https://jsandler18.github.io/extra/interrupts.html

An **Exception** is an event that is triggered when something exceptional occurs
during normal program execution. Examples of such exceptional occurrences include hardware devices
presenting new data to the CPU, user code asking to perform a privileged action, and a bad instruction
was encountered.

On ARM processors, when an exception occurs, a specific address is loaded into the program counter register,
branching execution to this point. At this location, the firmware contains branch instructions
to routines that handle the exceptions. This set of addresses, also known as the Vector Table,
usually starts at address 0 (in RAM) or 0xFFFF0000 (configuration known as `HIVECS <https://developer.arm.com/docs/ddi0363/e/programmers-model/exceptions/exception-vectors>`_), but on recent models
it can be located anywhere in the system memory.

Below is a table that describes the exceptions interesting to us:

========  ============================  ===========================================================
Offset    Exception name                What happened
========  ============================  ===========================================================
0x00      Reset                         Execution starts here at power on (see `Initial firmware analysis`_)
0x04      Undefined Instruction         Attempted to execute an invalid instruction
0x0C      Prefetch Abort                Attempted to read an instruction from non-executable memory
0x10      Data Abort                    Attempted to read data from a privileged memory region
**0x18**  **Interrupt Request (IRQ)**   Hardware wants to make the CPU aware of something
0x1C      Fast Interrupt Request (FIQ)  One select hardware can do the above faster than all others
========  ============================  ===========================================================


An **Interrupt Request** or **IRQ** is a notification to the processor
that something happened to some hardware that the processor should know about.
This can take many forms, for example, a character was received on the serial line
or a file I/O transfer was completed. The operating system (DryOS, VxWorks) uses a periodic timer interrupt
(`heartbeat <https://sites.google.com/site/rtosmifmim/home/timer-functions>`_) that usually fires every 10ms;
many other peripherals use interrupts to signal various events.

In order to find out which hardware devices are allowed to trigger interrupts,
and which device triggered an interrupt, we need to look at the interrupt controller
(``eos_handle_intengine``, which comes in many shapes and sizes, depending on camera generation).

For emulation purposes, we need to know when the firmware expects an interrupt for each peripheral
(for example, after completing a DMA transfer, or when a timer overflows, or when a `secondary CPU`__ wants to talk, and so on)
and how to react to MMIO activity from the interrupt handling routine
(for example, the firmware may check the status of the peripheral to figure out why the interrupt was triggered, or what to do next).

__ `Secondary processors`_

Interrupt activity can be logged by running the emulation with ``-d int``.
When troubleshooting interrupt issues, you will also want to log MMIO activity,
as well as some additional messages that are hidden by default: ``-d io,int,v``.

The interrupt IDs are mostly common across EOS models, but there are exceptions.
Model-specific interrupts can be found in ``model_list.c``, while generic ones
are hardcoded throughout the source.

A good janitor project would be to `document all the registers, interrupts and other model-specific constants
<http://www.magiclantern.fm/forum/index.php?topic=14656.0>`_,
in a way that's easy to read, reuse and doesn't go out of sync with the source code.

GPIO ports
''''''''''

These work much like the I/O ports on a Raspberry Pi or Arduino —
signal lines that you can switch high or low from software (outputs)
or whose input state (high or low) can be read by the processor (inputs).

__ `WriteProtect switch`_

Example: the SD card LED is driven by a GPIO output (by setting specific bits within the GPIO's register).
The `write-protect switch`__ state is read from a GPIO input (by reading back other bits).
Events from `hot-pluggable devices`__ (USB, external monitors, microphone etc) are usually detected
by reading some GPIO registers in a loop (but they may as well expect interrupts, e.g. ``MICDetectISR``).

__ `HotPlug events`_

GPIO ports are also used as `chip select <https://en.wikipedia.org/wiki/Chip_select>`_ signals
for various hardware devices that use the SPI protocol (examples below),
or as signalling lines to `secondary processors`_ for communication purposes.

Usual register values for driving GPIO ports: ``0x46/0x44``, ``0x138800/0x838C00``, ``0xD0002/0xC0003``.
More details `on the wiki <http://magiclantern.wikia.com/wiki/Register_Map#GPIO_Ports>`_.

Serial communication
''''''''''''''''''''

Some peripherals use the well-known
`I2C and SPI <https://www.byteparadigm.com/applications/introduction-to-i2c-and-spi-protocols/>`_ interfaces.
While their low-level communication uses MMIO registers and (sometimes) interrupts, one has to understand
the high-level protocol in order to emulate — or interact with — these peripherals.

Examples:

- `RTC chip <http://www.magiclantern.fm/forum/index.php?topic=2864.msg190823#msg190823>`_ (real-time clock)
- `ADTG and CMOS registers <http://magiclantern.wikia.com/wiki/ADTG>`_ (image capture hardware)
- `TFT SIO registers <http://www.magiclantern.fm/forum/index.php?topic=21108>`_ (built-in LCD controller)
- `HDMI CEC <http://www.magiclantern.fm/forum/index.php?topic=12022.msg136689#msg136689>`_ (Ctrl over HDMI)
- `Touch screen controller <http://www.magiclantern.fm/forum/index.php?topic=15895.msg187011#msg187011>`_
- `MPU communication`_ (see below).

Secondary processors
''''''''''''''''''''

Canon cameras are generally multiprocessor systems. Since our understanding of all these processors
is quite limited, we attempt to emulate only one of them at a time (at least for the time being)
and model the secondary processors as regular `peripherals`__.

__ `peripheral`_

Common secondary processors:

- the `MPU`__ (I/O microcontroller on EOS models, `TX19A <http://magiclantern.wikia.com/wiki/Tx19a>`_ on DIGIC 4)
- the `Eeko <http://www.magiclantern.fm/forum/index.php?topic=13408.msg175656#msg175656>`_ (on DIGIC 5, emulated as ``5D3eeko``)
  and `Omar <http://www.magiclantern.fm/forum/index.php?topic=13408.msg194424#msg194424>`_ (on DIGIC 6)
  cores likely used for image processing
- the `JPCORE <http://www.magiclantern.fm/forum/index.php?topic=18443.msg177082#msg177082>`_ (JPEG/LJ92 and H.264 encoders, likely CPU-based)
- the AE processor on 5D Mark IV (``K349AE``, emulated as ``5D4AE``)
- the secondary ARM core on 7D (``K250M``, emulated as ``7DM``), 7D Mark II (``K289S``, emulated as ``7D2S``) and other Dual DIGIC models
- the `Zico <http://chdk.setepontos.com/index.php?topic=11316.msg129104#msg129104>`_ 
  `GPU <http://chdk.setepontos.com/index.php?topic=12788.0>`_ on DIGIC 6 and 7 models (Xtensa)
- the `lens MCU <http://www.magiclantern.fm/forum/index.php?topic=20969>`_ (firmware upgradeable on recent models).

__ `MPU communication`_


Adding support for a new camera model
`````````````````````````````````````

Initial firmware analysis
'''''''''''''''''''''''''

1) Find the ROM load address and the code start address.
   If unknown, use an initial guess to disassemble (even 0),
   then look for code jumping to or referencing some absolute address
   and make an educated guess from there.

   DIGIC 5 and earlier models will start the bootloader at ``0xFFFF0000`` (HIVECS)
   and will jump to main firmware at ``0xFF810000``, ``0xFF010000`` or ``0xFF0C0000``.
   There is one main ROM (ROM1) at ``0xF8000000``, 4/8/16/32 MiB mirrored until ``0xFFFFFFFF``,
   and there may be a second ROM (ROM0) at ``0xF0000000``, mirrored until ``0xF8000000 - 1 = 0xF7FFFFFF``.
   Some DIGIC 5 models also use a serial flash for storing properties (persistent settings).

   DIGIC 6 models will start at ``*(uint32_t*)0xFC000000``,
   bootloader is at 0xFE020000 and main firmware starts at 0xFE0A0000. There is
   a 32 MiB ROM mirrored at 0xFC000000 and 0xFE000000 (there may be others).
   There is a serial flash as well, used for storing properties.

   DIGIC 7 models will start at ``0xE0000000`` in ARM mode
   and will jump to main firmware at ``0xE0040000`` in Thumb mode.
   There is a 32 MiB ROM (ROM0) at ``0xE0000000``, mirrored until ``0xEFFFFFFF``,
   and an unusually slow 16 MiB ROM (ROM1) at ``0xF0000000``, mirrored until ``0xFFFFFFFF``.
   No serial flash was identified.

   The ROM load address is the one you have used when dumping it (usually one of the mirrors).
   The memory map is printed when starting QEMU — you'll see where each ROM is loaded
   and where are the mirrored copies, if any.

   The MPU/MMU configuration (printed in QEMU as soon as the guest code
   changes the relevant registers) is very useful for finding the memory map
   on new models -- see the ARM Architecture Reference Manual (aka ARM ARM)
   for the CPU you are interested in:

   - DIGIC 2..5: ARM946E-S `[1] <http://chdk.setepontos.com/index.php?topic=9801.msg99865#msg99865>`_ -- `arm_arm.pdf <http://www.scss.tcd.ie/~waldroj/3d1/arm_arm.pdf>`_;
   - DIGIC 6: Cortex R4 `[2] <http://chdk.setepontos.com/index.php?topic=11316.msg124273#msg124273>`_ -- `ARM ARM v7 A&R <https://www.cs.utexas.edu/~simon/378/resources/ARMv7-AR_TRM.pdf>`_ and `Cortex R4 TRM <http://infocenter.arm.com/help/topic/com.arm.doc.ddi0363g/DDI0363G_cortex_r4_r1p4_trm.pdf>`_;
   - DIGIC 7: Cortex A9 `[3] <http://chdk.setepontos.com/index.php?topic=13014.msg131110#msg131110>`_ -- `ARM ARM v7 A&R <https://www.cs.utexas.edu/~simon/378/resources/ARMv7-AR_TRM.pdf>`_ and `Cortex A9 TRM <http://infocenter.arm.com/help/topic/com.arm.doc.ddi0388f/DDI0388F_cortex_a9_r2p2_trm.pdf>`_.

   |

2) (Re)load the code in the disassembler at the correct address:

   - `Loading into IDA <http://www.magiclantern.fm/forum/index.php?topic=6785.0>`_
   - `Tutorial: finding stubs (with disassemble.pl) <http://www.magiclantern.fm/forum/index.php?topic=12177.0>`_
   - `Loading into ARMu <http://www.magiclantern.fm/forum/index.php?topic=9827.0>`_
   - Other disassemblers will also work (the list is open).

   |

3) Add a very simple definition for your camera and get an `initial test run`_.
   Try to guess some missing bits from the error messages, if possible.

4) Code blocks copied from ROM to RAM

   .. code:: shell
  
     ./run_canon_fw.sh EOSM2,firmware="boot=0" -d romcpy |& grep ROMCPY
    [ROMCPY] 0xFFFF0000 -> 0x0        size 0x40       at 0xFFFF0980
    [ROMCPY] 0xFFFE0000 -> 0x100000   size 0xFF2C     at 0xFFFF0FCC
    [ROMCPY] 0xFFD1F0E4 -> 0x1900     size 0xB70A0    at 0xFF0C000C
    [ROMCPY] 0xFF0C0E04 -> 0x4B0      size 0x1E8      at 0xFF0C0D70

   You may extract these blobs with e.g.:

   .. code:: shell

     dd if=ROM1.BIN of=EOSM2.0x1900.BIN bs=1 skip=$((0xD1F0E4)) count=$((0xB70A0))

   If you are analyzing the main firmware, load ``EOSM2.0x1900.BIN`` as an additional binary file
   (in IDA, choose segment 0, offset 0x1900). Do the same for the blob copied at 0x4B0.

   If you are analyzing the bootloader, extract and load the first two blobs in the same way.
   Other models may have slightly different configurations, so YMMV.

5) Export the functions called during your test run:

   .. code:: shell

     ./run_canon_fw.sh EOSM2,firmware="boot=0" -d idc
     ...
     EOSM2.idc saved.

   Load the IDC script into IDA, or convert it if you are using a different disassembler.

   Locate ``task_create``, ``register_func``, ``register_interrupt`` and ``CreateStateObject``
   and add GDB stubs for them in ``CAM/debugmsg.gdb``. Run the firmware under GDB
   to identify some (thousands of) named functions.

   .. code:: shell

     ./run_canon_fw.sh EOSM2,firmware="boot=0" -d debugmsg -s -S & arm-none-eabi-gdb -x EOSM2/debugmsg.gdb
     ...
     named_functions.idc saved.

   Load the new IDC script into IDA and start hacking!

   Repeat this procedure as the emulation is getting better, to identify new functions.

   |

Initial test run
''''''''''''''''

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
In Canon parlance, NG means "not good" — see for example ``NG AllocateMemory``
on the "out of memory" code path. Let's check whether this error message has to do
with I/O activity (usually that's where most emulation issues come from):

.. code:: shell

  ./run_canon_fw.sh 5DS -d io
  ...
  [DIGIC6]   at 0xFE020CD0:FE020B5C [0xD203040C] <- 0x500     : MR (RAM manufacturer ID)
  [DIGIC6]   at 0xFE020CDC:FE020B5C [0xD203040C] <- 0x20500   : MR (RAM manufacturer ID)
  [DIGIC6]   at 0xFE020CE4:FE020B5C [0xD203040C] -> 0x0       : MR (RAM manufacturer ID)
  MEMIF NG MR05=00000000 FROM=00000001
  BTCM Start Master

OK, so the message appears to be related to these I/O registers.
Look up the code that's handling them (search for "RAM manufacturer ID").
You'll find it in eos.c:eos_handle_digic6, at the register 0xD203040C
(as expected), and you'll find it uses a model-specific constant:
``s->model->ram_manufacturer_id``. Let's look around to see what's up with it:

.. code:: C

  .name                   = "80D",
  .ram_manufacturer_id    = 0x18000103,   /* RAM manufacturer: Micron */

  .name                   = "750D",
  .ram_manufacturer_id    = 0x14000203,

  .name                   = "5D4",
  .ram_manufacturer_id    = 0x18000401,

Good — it's now clear you'll have to find this constant. You have many choices here:

- disassemble the ROM near the affected address,
  and try to understand what value Canon code expects from this register
- use pattern matching and find it based on a similar camera model
- try the values from another camera model, hoping for the best
- trial and error

Let's go for the last one (probably the easiest). If you look at the code,
you may notice the "5" corresponds to the least significant byte in this RAM ID.
If you didn't, don't worry — you can just try something like 0x12345678:

.. code:: C

    {
        .name                   = "5DS",
        .digic_version          = 6,
        .ram_manufacturer_id    = 0x12345678,
    },

and the new error message will tell you the answer right away::

  MEMIF NG MR05=00000078 FROM=00000001

You now have at most 4 test runs to find this code :)

A more complete example: the `EOS M2 walkthrough <http://www.magiclantern.fm/forum/index.php?topic=15895.msg185103#msg185103>`_
shows how to add support for this camera from scratch, right through to getting the Canon GUI to boot (and more!)

Although this model is already supported in the repository,
you can always roll back to an older changeset (``3124887``) and follow the tutorial.

Working out all the way to Canon GUI
````````````````````````````````````

This might be a short journey (such as finding a typo or tweaking some MMIO register), or a long one (lots of things to adjust).
It's hard to tell in advance how much work it's going to be (each camera model has its own quirks),
but here's a short overview of Canon EOS boot process.

Overview of Canon EOS boot process
''''''''''''''''''''''''''''''''''

There are at least two (different) code blobs in Canon firmware:
the bootloader (what runs at power on) and the main firmware.
Generally, you cannot call bootloader functions from main firmware, or viceversa
(except maybe for trivial functions that do not use any global variables).

The start addresses for bootloader and main firmware can be found at
`Initial firmware analysis`_.

The bootloader has the following functionality:

- initialize the RAM configuration (memory protection regions, cache setup etc)
- jump to main firmware if everything is alright
- load AUTOEXEC.BIN or firmware updates, if the boot flags are configured for this
- fallback to factory menus if the hardware or the main firmware are somehow out of order
- handshaking with other CPU cores, if any.

Note: the EOS M5 has `two bootloaders <http://chdk.setepontos.com/index.php?topic=13014.msg131205#msg131205>`_, one of them running DryOS!

Getting the bootloader to run
'''''''''''''''''''''''''''''

There are two major goals here:

- launch the main firmware
- initialize the SD or CF card to load ``AUTOEXEC.BIN``.

The first goal is a lot easier, so let's start with that. What can go wrong?

- bootloader gets stuck in a loop somewhere
- bootloader executes some factory tool

Both of these are likely affected by some MMIO register. Run the emulation with ``-d io``
and try to figure out what registers might change the code paths taken by the bootloader.

Easiest case: code gets stuck reading some MMIO register. Solutions:

- look in the disassembly at the code where the register is read, and figure out what value it expects
- try random values (it may even work for simple handshakes)

Example for 5D3: comment out register ``0xC0400204`` (``case 0x204`` under ``C0400000``,
introduced in `b79cd7a <https://bitbucket.org/hudson/magic-lantern/commits/b79cd7a>`_)
and run with ``-d io``::

  [BASIC]    at 0xFFFF066C:00000000 [0xC0400204] -> 0x0       : ???
  (infinite loop repeating the same message over and over)

Just for kicks, let's see what happens if we return random values::

  ./run_canon_fw.sh 5D3,firmware="boot=0" -d io |& grep 0xC0400204
  [BASIC]    at 0xFFFF0554:00000000 [0xC0400204] -> 0x9474BA98: ???
  [BASIC]    at 0xFFFF066C:00000000 [0xC0400204] -> 0xCD84DC39: ???
  [BASIC]    at 0xFFFF066C:00000000 [0xC0400204] -> 0x9BC36796: ???

As soon as the random value matches what the firmware expects, emulation continues. In our case, the test was::

  FFFF066C   LDR R1, [R0]
  FFFF0670   AND R1, R1, #2
  FFFF0674   CMP R1, #2

Easy, right?

Harder case: the value of some MMIO register steers the code on a path you don't want.

Example for 1300D, before changeset `cbf042b <https://bitbucket.org/hudson/magic-lantern/commits/cbf042b>`_
(to try this, manually undo the linked change):

After adding the basic definition, the bootloader shows a factory menu, rather than jumping to main firmware.

.. code:: C

    {
        .name                   = "1300D",
        .digic_version          = 4,
        .rom0_size              = 0x02000000,
        .rom1_size              = 0x02000000,
        .firmware_start         = 0xFF0C0000,
    },

It does not get stuck anywhere, the factory menu works (you can navigate it on the serial console), so what's going on?

Run the emulation with ``-d io``, look at all MMIO register reads (any of these might steer the program on a different path)
and analyze the disassembly where these registers are read.

.. code:: shell

  ./run_canon_fw.sh 1300D -d io
  ...
  [*unk*]    at 0xFFFF066C:FFFF00C4 [0xC0300000] -> 0x0       : ???
  [*unk*]    at 0xFFFF0680:FFFF00C4 [0xC0300000] <- 0x1550    : ???
  [*unk*]    at 0xFFFF068C:FFFF00C4 [0xC0300208] <- 0x1       : ???
  [GPIO]     at 0xFFFF0694:FFFF00C4 [0xC022F48C] -> 0x10C     : 70D/6D SD detect?
  [FlashIF]  at 0x00000108:FFFF00C4 [0xC00000D0] -> 0x0       : ???
  [FlashIF]  at 0x00000114:FFFF00C4 [0xC00000D0] <- 0xE0000   : ???
  [FlashIF]  at 0x0000011C:FFFF00C4 [0xC00000D8] <- 0x0       : ???
  [GPIO]     at 0x00000128:FFFF00C4 [0xC022F4D0] <- 0x3000    : ???
  [FlashIF]  at 0x0000012C:FFFF00C4 [0xC00000D0] -> 0x0       : ???
  [FlashIF]  at 0x00000130:FFFF00C4 [0xC00000D0] -> 0x0       : ???
  [FlashIF]  at 0x00000134:FFFF00C4 [0xC00000D0] -> 0x0       : ???
  System & Display Check & Adjustment program has started.

If the number of registers is small, consider trial and error, or some sort of brute-forcing.
For more complex cases, look into advanced RE tools that use SMT solvers or similar black magic,
or try to understand what the code does (and how to get it back on track).

In this particular case, it's easy to guess
(exercise: give it a try, pretending you haven't already seen the solution).

In a few cases, the bootloader may use interrupts as well
(for example, 7D uses interrupts for IPC — communication between the two DIGIC cores).
To analyze them, place a breakpoint at 0x18 and see what happens from there.

The second goal — loading ``AUTOEXEC.BIN`` from the card — requires emulation of the SD or CF card.
If it doesn't already work, look at MMIO activity (``-d io,sdcf``) and try to make sense of the SD or CF
initialization sequences (both protocols are documented online). The emulation has to be able
to read arbitrary sectors from the virtual card — once you provide the low-level block transfer
functionality, Canon firmware would be able to handle the rest (filesystem drivers etc).
In other words, you shouldn't have to adjust anything in order to emulate EXFAT, for example.

Getting the main firmware to run
''''''''''''''''''''''''''''''''

Step by step:

- get debug messages

  - identify DebugMsg (lots of calls, format string is third argument), add the stub to ``CAM/debugmsg.gdb``, run with ``-d debugmsg``
  - identify other functions used to print errors (uart_printf, variants of DebugMsg with format string at second argument etc — look for strings)
  - identify any other strings that might be helpful (tip: run with ``-d calls`` and look for something that makes even a tiny bit of sense)
  
  |

- make sure DryOS timer (heartbeat) runs (**important!**):

  - look for MMIO activity that might set up a timer at 10ms or nearby
  - figure out what interrupt is expects (run with ``-d io,int,v`` and look for "Enabled interrupt XXh", usually right before the timer configuration)
  - make sure you get periodical interrupts when running with ``-d io,int``, even when all DryOS tasks are idle

  Example: 1300D (comment out ``dryos_timer_id`` and ``dryos_timer_interrupt`` from the 1300D section
  in ``model_list.c`` to get the state before `7f1a436 <https://bitbucket.org/hudson/magic-lantern/commits/7f1a436#chg-contrib/qemu/eos/model_list.c>`_)::

    [INT]      at 0xFE0C3E10:FE0C0C18 [0xC0201010] <- 0x9       : Enabled interrupt 09h
    ...
    [TIMER]    at 0xFE0C0C54:FE0C0C54 [0xC0210108] <- 0x270F    : Timer #1: will trigger after 10 ms
    [TIMER]    at 0xFE0C3F5C:FE0C0C68 [0xC0210110] <- 0x1       : Timer #1: interrupt enable?
    [TIMER]    at 0xFE0C3F5C:FE0C0C68 [0xC0210100] <- 0x1       : Timer #1: starting
    ...

  Caveat: the emulation may go **surprisingly far *without* DryOS timer** — as far as running the GUI
  with bugs that are almost impossible to explain (such as menu selection bar being behind the logical selection by exactly 1 position).
  To see it with your own eyes, set ``dryos_timer_interrupt = 0x09`` (correct is ``0x0A``) on 60D (maybe also on other models).

  Therefore, please do not assume this works, even if you think it does — double-check!

- get some tasks running

  - identify ``task_create`` (in ``debugmsg.gdb`` — same as in ML ``stubs.S``) and run the firmware under GDB
  - identify the pointer to current DryOS task

    This is called ``current_task_addr`` in ``model_list.c``, ``CURRENT_TASK`` in ``debugmsg.gdb`` or ``current_task`` in ML stubs —
    see `debug-logging.gdb <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/scripts/debug-logging.gdb#debug-logging.gdb>`_
    for further hints.

  - identify where the current interrupt is stored
  
    Look in the interrupt handler — breakpoint at 0x18 to find it — and find ``CURRENT_ISR`` in
    `debug-logging.gdb <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/scripts/debug-logging.gdb#debug-logging.gdb>`_,
    or current_interrupt in ML stubs.
    If you can't find it, you may set it to 0, but if you do, please take task names with a grain of salt if they are printed from some interrupt handler.

  - run with ``-d tasks`` and watch the DryOS task switches.

  |

- optional, sometimes helpful: enable the serial console and the DryOS shell (debug with ``-d io,int,uart``)
- make the startup sequence run (see `EOS firmware startup sequence`_)
- these may need tweaking: WriteProtect switch, HotPlug events (usually GPIOs)
- make sure the GUI tasks are starting (in particular, GuiMainTask)
- identify button codes (`extract_button_codes.py <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/extract_button_codes.py>`_)
- make sure the display is initialized, identify the image buffers etc.

EOS firmware startup sequence
'''''''''''''''''''''''''''''

Please note: this section does not apply to recent EOS models (M3 and newer); these models use PowerShot firmware.

If you've looked at enough `startup logs <http://www.magiclantern.fm/forum/index.php?topic=2388>`_,
you've probably noticed they are **not deterministic** (they don't always execute in the same order,
even on two runs performed under identical conditions). The EOS firmware starts many things in parallel;
there is a Sequencer object (SEQ) with a notification system that uses some binary flags
to know where things are finished. Let's look at its debug messages::

   ./run_canon_fw.sh 60D,firmware="boot=0" -d debugmsg |& grep -E --text Notify.*Cur
   [        init:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 0, 0x10000, Flag = 0x10000)
   [    PowerMgr:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 1, 0x20000002, Flag = 0x2)
   [     Startup:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 1, 0x20000000, Flag = 0x20000000)
   [     FileMgr:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 2, 0x10, Flag = 0x10)
   [     Startup:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 3, 0xe0110, Flag = 0x40000)
   [     Startup:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 3, 0xa0110, Flag = 0x80000)
   [     Startup:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 3, 0x20110, Flag = 0x100)
   [      RscMgr:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 3, 0x20010, Flag = 0x20000)
   [     FileMgr:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 3, 0x10, Flag = 0x10)
   [     Startup:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 4, 0x110, Flag = 0x100)
   [     FileMgr:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 4, 0x10, Flag = 0x10)
   [     Startup:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 5, 0x80200200, Flag = 0x80000000)
   [ GuiMainTask:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 5, 0x200200, Flag = 0x200000)
   [       DpMgr:ff02b9f8 ] (00:03) [SEQ] NotifyComplete (Cur = 5, 0x200, Flag = 0x200)
   ...

Notice the pattern? Every time a component is initialized, it calls ``NotifyComplete`` with some binary flag.
The bits from this flag are cleared from the middle number, so this number must indicate what processes
still have to do their initialization. Once this number reaches 0 (not printed),
the startup sequence advances to the next stage.

**What if it gets stuck?**

You will need to figure it out. Difficulty: anywhere within [0 — infinity); a great dose of luck will help.

Let's look at an example — 1300D::

   ./run_canon_fw.sh 1300D,firmware="boot=0" -d debugmsg |& grep --text -E Notify.*Cur
   [        init:fe0d4054 ] (00:03) [SEQ] NotifyComplete (Cur = 0, 0x10000, Flag = 0x10000)
   [     Startup:fe0d4054 ] (00:03) [SEQ] NotifyComplete (Cur = 1, 0x20000002, Flag = 0x20000000)
   [    PowerMgr:fe0d4054 ] (00:03) [SEQ] NotifyComplete (Cur = 1, 0x2, Flag = 0x2)
   [     FileMgr:fe0d4054 ] (00:03) [SEQ] NotifyComplete (Cur = 2, 0x10, Flag = 0x10)
   [     Startup:fe0d4054 ] (00:03) [SEQ] NotifyComplete (Cur = 3, 0xe0110, Flag = 0x40000)
   [     Startup:fe0d4054 ] (00:03) [SEQ] NotifyComplete (Cur = 3, 0xa0110, Flag = 0x80000)
   [     Startup:fe0d4054 ] (00:03) [SEQ] NotifyComplete (Cur = 3, 0x20110, Flag = 0x100)
   [     FileMgr:fe0d4054 ] (00:03) [SEQ] NotifyComplete (Cur = 3, 0x20010, Flag = 0x10)

It got stuck because somebody has yet to call ``NotifyComplete`` with ``Flag = 0x20000``.

Who's supposed to call that? Either look in the disassembly to find who calls ``NotifyComplete`` with the right argument,
or — if not obvious — look in the startup logs of other camera models from the same generation, where the flag is likely the same.

Why it didn't get called? Most of the time:
  
- some task is waiting at some semaphore / message queue / event flag
- it may expect some interrupt to be triggered (to complete the initialization of some peripheral)
- it may expect some message from the MPU
- other (some task stuck in a loop, some prerequisite code did not run etc)

How to solve? There's no fixed recipe; generally, try to steer the code towards calling ``NotifyComplete`` with the missing flag.
You'll need to figure out where it gets stuck and how to fix it. Some things to try:

- check whether the task supposed to call the troublesome ``NotifyComplete`` is waiting
  (not advancing past a ``take_semaphore`` / ``msg_queue_receive`` / ``wait_for_event_flag``; 
  the ``extask`` command in `Dry-shell`__ may help)

__ `Serial console`_

- check who calls the corresponding give_semaphore / msg_queue_send etc and why it doesn't run
  (it may be some callback, it may be expected to run from an interrupt, it may wait for some peripheral and so on)

In our case, after cross-checking the same sequence on a 60D (another DIGIC 4 camera) and figuring out a hackish way to patch it
(enough to bring the GUI, but unreliable, with some mysterious bugs), noticed that... we were looking in the wrong place!

The DryOS timer interrupt (heartbeat) was different from *all other* DIGIC 4 and 5 models, and we've never expected
the emulation to go **that** far without a valid heartbeat (that way, we've lost many hours of debugging).
Now scroll up and read that section again ;)

Fixing that and a few other things (`commit 7f1a436 <https://bitbucket.org/hudson/magic-lantern/commits/7f1a436>`_)
were enough to bring the GUI on 1300D.

PowerShot firmware startup sequence
'''''''''''''''''''''''''''''''''''

TODO (see CHDK). Startup code is generally simpler and single-threaded, but less verbose.

Assertions
''''''''''

These are triggered by Canon code when something goes wrong. On the UI, these will show ERR70 —
if the rest of the system is able to change the GUI mode and show things of the screen.

When running Magic Lantern, it will attempt to save a crash log for each ERR70.

There are usually over 1000 different conditions that can trigger an assertion (ERR70).
**The only way to tell** which one it was is to read the assert message and locate it in the disassembly.
The `ERR70 description from Canon <http://cpn.canon-europe.com/content/education/infobank/camera_settings/eos_error_codes_and_messages.do>`_
("A malfunction with the images has been detected.") is overly simplistic.

-------------

**Do not attempt to fix a camera with ERR70 yourself!** Please contact us instead,
providing any relevant details (crash logs, what you did before the error and so on).
This section is for fixing assertions **in the emulation** (on a virtual machine), not on real cameras!

-------------

What we can do about them?

- figure out why they happen and fix the emulation
- as a workaround, patch the affected function (see `Patching code`_)

Tip: find the assert stub, add assert_log to your debugmsg.gdb
and run the firmware under GDB with ``-d callstack``.
You'll get a stack trace to see what code called that assertion - example below.

Patching code
'''''''''''''

Emulation is not perfect, and neither our skills. If we can figure out how to emulate cleanly
all the code, that's great. If not, there will be some code bits that will not be emulated well.
For example, an unhandled microsecond timer (USleep in DIGIC 6 models) will cause the emulation to halt
when the firmware only wants to wait for a few microseconds.

When you don't know how to solve it, you may get away with patching the troublesome routine.
This shouldn't be regarded as a fix — it's just a workaround that will hopefully help advancing the emulation.

That's why we prefer to patch the firmware from GDB scripts. These can be edited easily to experiment with,
and there is some additional burden for running a patched firmware (longer commands to type),
as a reminder that a proper fix is still wanted.

Patching things may very well break other stuff down the road — use with care.

-------------

**Be very careful patching the assertions when running on a physical camera.
If an assert was reached, that usually means something already went terribly wrong —
hiding the error message from the user is *not* the way to solve it!**

-------------

Examples:

**Patching the UTimer waiting routine on 80D** (``80D/patches.gdb``, commit `7ea57e7 <https://bitbucket.org/hudson/magic-lantern/commits/7ea57e73c09#chg-contrib/qemu/scripts/80D/patches.gdb>`_):

.. code::

 source patch-header.gdb
 
 # UTimer (fixme)
 set *(int*)0xFE5998C6 = 0x4770
 
 source patch-footer.gdb

Note: ``0x4770`` is ``BX LR`` on Thumb code; on ARM, that would be ``0xe12fff1e``.
See arm-mcr.h for a few useful instructions encodings, use an assembler or read the ARM docs
(in particular, `ARM Architecture Reference Manual <http://www.scss.tcd.ie/~waldroj/3d1/arm_arm.pdf>`_ 
and `Thumb-2 Supplement Reference Manual <http://read.pudn.com/downloads159/doc/709030/Thumb-2SupplementReferenceManual.pdf>`_).

**Patching the EstimatedSize assertion on 80D** (``80D/patches.gdb``, commit `b6c5710 <https://bitbucket.org/hudson/magic-lantern/commits/b6c5710afebbffbb194f9102fbfa9798b99fde1b?at=qemu#chg-contrib/qemu/scripts/80D/patches.gdb>`_)

After enabling the above UTimer patch, with the generic MPU messages you may get this error::

  ASSERT : Resource/./EstimatedSize.c, Task = RscMgr, Line 1484

To find where it was triggered from, make sure you have the assert stub enabled in ``80D/debugmsg.gdb``::

  b *0xFE547CD4
  assert_log

then run the firmware under GDB, with ``-d callstack``:

.. code:: shell

  (./run_canon_fw.sh 80D,firmware="boot=0" -d debugmsg,callstack -s -S & arm-none-eabi-gdb -x 80D/debugmsg.gdb) |& grep --text -C 5 ASSERT
  ...
          0xFE19B1A9(0, 1, 51, 8000003b)                                           at [RscMgr:fe19b287:2f4330] (pc:sp)
           0xFE19B03B(2f4320, 1, 51, 8000003b)                                     at [RscMgr:fe19b1af:2f4310] (pc:sp)
            0xFE547CD5(fe19b104 "FALSE", fe19b0d0 "Resource/./EstimatedSize.c", 5cc, 8000003b)
                                                                                   at [RscMgr:fe19b14b:2f42f0] (pc:sp)
  [      RscMgr:fe19b14b ] [ASSERT] FALSE at Resource/./EstimatedSize.c:1484, fe19b14f
  ...

The function you are looking for is ``0xFE19B03B`` (could have been any of the callers) and the assertion was triggered at ``0xfe19b14b``.
`Our patch <https://bitbucket.org/hudson/magic-lantern/commits/b6c5710afebbffbb194f9102fbfa9798b99fde1b?at=qemu#chg-contrib/qemu/scripts/80D/patches.gdb>`_
is at ``0xFE19B06A``, in the function identified with this method.

Incorrect firmware version?
'''''''''''''''''''''''''''

If you have to use ``patches.gdb`` for your camera, you need to be careful:
these patching scripts may perform temporary changes to the ROM. However,
at startup, ML computes a simple signature of the firmware,
to make sure it is started on the correct camera model and firmware version
(and print an error message otherwise, with portable display routines).
These patches will change the firmware signature — so you'll get an error message
telling you the firmware version is incorrect (even though it is the right one).

To avoid this issue, please consider one of the following:

- fix the emulation to avoid unnecessary patches (preferred)

- implement the patches as GDB breakpoints, rather than changing ROM contents
  (that way, the patches will not interfere with ML's firmware signature checking.)

Note: at the time of writing, firmware signature only covers the first 0x40000 bytes
from main firmware start address; ROM patches after this offset should be fine.
If in doubt, just make sure the same ML binary loads on both the patched and unpatched ROMs.


MPU communication
'''''''''''''''''

On EOS firmware, buttons, some properties (camera settings) and a few others are handled on a different CPU,
called MPU in Canon code (not sure what it stands for). On PowerShot firmware you don't need to worry about it — buttons are handled on the main CPU (PhySw).

Communication is done on a serial interface with some GPIO handshaking (look up SIO3 and MREQ in the firmware).
It can be initiated from the main CPU (mpu_send, which toggles a GPIO to get MPU's attention) or from the MPU (by triggering a MREQ interrupt); 
the transfer is then continued in SIO3 interrupts. Each interrupt transfers two bytes of data.

Message format is: ``[message_size] [payload_size] <payload>`` (where ``[x]`` is 1 byte and ``<x>`` is variable-sized).

Payload format is: ``[class] [id] <data> [ack_requested]``.

The first two bytes can be used to identify the message
(and for messages that refer to a property, to identify the property).
Property events are in `known_spells.h <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/known_spells.h>`_;
GUI events (button codes) have ``class = 06``.

To log the MPU communication:

- `dm-spy-experiments <http://www.magiclantern.fm/forum/index.php?topic=2388.0>`_ branch, ``CONFIG_DEBUG_INTERCEPT_STARTUP=y`` (``mpu_send`` and ``mpu_recv`` stubs are enabled by default)
- `startup log <http://builds.magiclantern.fm/jenkins/view/Experiments/job/startup-log/>`_ builds (compiled with the above configuration)
- in QEMU, enable ``mpu_send`` and ``mpu_recv`` in ``debugmsg.gdb`` and run the firmware under GDB
- low-level: ``-d io,mpu``.

The first message is sent from the main CPU; upon receiving it, the MPU replies back:

.. code::

  ./run_canon_fw.sh 60D -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb
  ...
  [     Startup:ff1bf228 ] register_interrupt(MREQ_ISR, 0x50, 0xff1bf06c, 0x0)
  [     Startup:ff1bf23c ] register_interrupt(SIO3_ISR, 0x36, 0xff1bf0fc, 0x0)
  [     Startup:ff1dcc18 ] task_create(PropMgr, prio=14, stack=0, entry=ff1dcb24, arg=807b1c)
  [     Startup:ff05e1b8 ] mpu_send( 06 04 02 00 00 )
  [MPU] Received: 06 04 02 00 00 00  (Init - spell #1)
  [MPU] Sending : 08 07 01 33 09 00 00 00  (unnamed)
  [     INT-36h:ff1bf420 ] mpu_recv( 08 07 01 33 09 00 00 00 )
  [MPU] Sending : 06 05 01 20 00 00  (PROP_CARD1_EXISTS)
  [     INT-36h:ff1bf420 ] mpu_recv( 06 05 01 20 00 00 )
  [MPU] Sending : 06 05 01 21 01 00  (PROP_CARD2_EXISTS)
  [     INT-36h:ff1bf420 ] mpu_recv( 06 05 01 21 01 00 )
  ...

The message sent by the main CPU is::

  06 04 02 00 00 00

- ``06`` is message size (always even)
- ``04`` is payload size (always ``message_size - 1`` or ``message_size - 2``)
- ``02 00 00 00`` is the payload:

  - ``02 00`` identifies the message (look it up in `known_spells.h <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/known_spells.h>`_)
  - the last ``00`` means no special confirmation was requested (``Complete WaitID`` string)
  - the remaining ``00`` may contain property data or other information (nothing interesting here)

The first message sent back by the MPU is::

  08 07 01 33 09 00 00 00

- ``08 07``: message size and payload size
- ``01 33`` identifies the message (maps to property 0x80000029, unknown meaning)
- ``09 00 00`` is the property data (note: its size is 3 on the MPU, but 4 on the main CPU)
- ``00`` means no special confirmation was requested
  (``01`` would print ``Complete WaitID = 0x80000029``)

The second and third messages are easier to grasp::

  06 05 01 20 00 00
  06 05 01 21 01 00

- ``06 05``: message size and payload size
- ``01 20`` and ``01 21`` identify the messages (``0x8000001D/1E PROP_CARD1/CARD2_EXISTS``)
- ``00`` and ``01``: property data, meaning CF absent and SD present (size 1 on MPU, 4 on main CPU)
- ``00`` (last one) means no special confirmation was requested.


How do you get these messages?

From a `startup log <http://builds.magiclantern.fm/jenkins/view/Experiments/job/startup-log/>`_ (`dm-spy-experiments <http://www.magiclantern.fm/forum/index.php?topic=2388.0>`_), use 
`extract_init_spells.py <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/extract_init_spells.py>`_
to parse the MPU communication into C code (see `make_spells.sh <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/make_spells.sh>`_).

There are also generic spells in `generic.h <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/generic.h>`_
that are recognized by most EOS models and are good enough to enable navigation on Canon menus.

Things to check:

- mpu_send: the message format should make sense (consistent sizes etc)
- our emulated MPU should receive the message correctly: ``[MPU] Received:`` should match the previous mpu_send line
- it should reply back with something: ``[MPU] Sending :``
- mpu_recv should be called, with the same message as argument
- to see what the firmware does with these messages, look in mpu_send and track the messages from there.

Serial flash
''''''''''''

To enable serial flash emulation (if your camera needs it, you'll see some relevant startup messages),
define ``.serial_flash_size`` in ``model_list.c`` and a few other parameters:

- chip select signal (CS): some GPIO register toggled before and after serial flash access
- SIO channel (used for SPI transfers)
- check SFIO and SFDMA in ``eos_handlers`` (for DMA transfers — Canon reused the same kind of DMA used for SD card).

Dumper: `sf_dump module <https://bitbucket.org/hudson/magic-lantern/src/unified/modules/sf_dump>`_.

For early ports, you might (or might not) get away with serial flash contents from another model.

`Patching <https://bitbucket.org/hudson/magic-lantern/commits/652133663c39>`_ might help.
When editing SFDATA.BIN files manually, watch out — some data blocks are shifted by 4 bits for some reason.

WriteProtect switch
'''''''''''''''''''

This is easy: run with ``-d debugmsg,io`` and look for a GPIO read right before this message::

  [STARTUP] WriteProtect (%#x)

Example::

  ./run_canon_fw.sh 6D,firmware="boot=0" -d debugmsg,io |& ansi2txt | grep WriteProtect -C 5
  ...
  [GPIO]   at Startup:FF14A330:FF0C4490 [0xC02200D0] -> 0x1       : GPIO_52
  [     Startup:ff0c44a8 ] (00:05) [STARTUP] WriteProtect (0x1)
  ...

That means, register 0xC02200D0 shows the WriteProtect switch state; you may want to change it to emulate a SD card without write protection.

If you don't see the WriteProtect message, this register is probably OK. To test the above, comment out the WriteProtect register handling code for your camera (usually in eos_handle_gpio).

HotPlug events
''''''''''''''

There is a task polling for hardware events, such as plugging a microphone, an external monitor,
an USB cable and maybe a few others. Generally, you want to emulate without these things,
so you'll need to look in the disassembly of HotPlug and see what it expects for each peripheral;
most of the time, it checks some GPIO registers — you may have to adjust them (usually in ``eos_handle_gpio``).

Since all of these registers are checked in a loop, you may want to silence them (``IGNORE_CONNECT_POLL``).

Adding support for a new Canon firmware version
```````````````````````````````````````````````

You will have to update:

- GDB scripts (easy — copy/paste from ML stubs or `look them up <http://www.magiclantern.fm/forum/index.php?topic=12177.0>`_)
- expected test results (time-consuming, see the `Test suite`_)
- any hardcoded stubs that might be around (e.g. in ``dbi/memcheck.c``)

Most other emulation bits usually do not depend on the firmware version
(5D3 1.2.3 was an exception).

`Updating Magic Lantern to run on a new Canon firmware version <http://www.magiclantern.fm/forum/index.php?topic=19417.0>`_
is a bit more time-consuming, but it's not difficult.

Are there any good docs on QEMU internals?
``````````````````````````````````````````

- http://nairobi-embedded.org/category/qemu.html
- http://blog.vmsplice.net
- QEMU mailing list (huge!)
- Xilinx QEMU

DryOS internals?
````````````````

This is the perfect tool for studying them. Start at:

- `Working out all the way to Canon GUI`_ for an overview
- DryOS shell (View -> Serial in menu, then type ``akashimorino``, then ``drysh``)
- task_create (from GDB scripts)
- semaphores (some GDB scripts have them)
- message queues (some GDB scripts have them)
- heartbeat timer (dryos_timer_id/interrupt in `model_list.c <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/model_list.c>`_)
- interrupt handler (follow the code at 0x18)
- to debug: ``-d io,int`` is very helpful (although a bit too verbose)

|

Serial console
``````````````

.. image:: doc/img/drysh.png
   :scale: 50 %
   :align: center

QEMU menu: ``View -> Serial``.

Hardware connections: possibly in the `battery grip pins <http://www.magiclantern.fm/forum/index.php?topic=7531>`_; 
see also `JTAG on PowerShot <https://nada-labs.net/2014/finding-jtag-on-a-canon-elph100hs-ixus115/>`_ 
and `UART pins on EOS M3 <http://chdk.setepontos.com/index.php?topic=12542.msg129346#msg129346>`_.

Some of these functions **can damage your camera!**

EOS menus
'''''''''

- FROMUTILITY menu

  - delete ``AUTOEXEC.BIN`` from the virtual card, but leave it bootable (and start with ``firmware="boot=1"``).
  - this is what happens when your camera locks up (see the warnings in `ML install guide <http://wiki.magiclantern.fm/install>`_).
  - interesting items:

    - boot flags
    - SROM menu on models with serial flash
    - Bufcon (GPIO names, `hidden menu <https://bitbucket.org/hudson/magic-lantern/commits/5d1f223994c4b437bfaae51b22e0fb216e73a4b7#chg-contrib/qemu/eos/eos_bufcon_100D.h>`_)

- FACTADJ menu

  - exit from FROMUTILITY menu to find it.

- Event shell

  - start main firmware (e.g. ``firmware="boot=0"``)
  - type ``akashimorino``
  - type ``?`` to see functions registered by name (aka `eventprocs <http://chdk.wikia.com/wiki/Event_Procedure>`_)
  - interesting items:

    - ``drysh`` to open the DryOS shell console
    - ``smemShowFix`` for the `RscMgr memory map <http://www.magiclantern.fm/forum/index.php?topic=5071.0>`_
    - ``dumpf`` to save a debug log (not all messages are saved; use `dm-spy-experiments <http://www.magiclantern.fm/forum/index.php?topic=2388.0>`_ to capture all of them)
    - ``dispcheck`` to save a screenshot of the BMP overlay
    - there are more functions than you can count, feel free to experiment and report back ;)
    - some of these functions **can damage your camera!** (but you can safely try them in QEMU)

- Dry-shell console (DryOS shell, DrySh)

  - type ``drysh`` at the event shell
  - type ``help`` for the available functions
  - interesting items:

    - ``extask`` to display DryOS tasks and their status, memory usage etc
    - ``meminfo`` and ``memmap`` to display DryOS memory map (ML is loaded in the *malloc* memory pool on many models)
    - network functions on recent models

PowerShot menus
'''''''''''''''

The PowerShot firmware expects some sort of `loopback <http://chdk.setepontos.com/index.php?topic=13278.0>`_ —
it prints a ``#`` and expects it to be echoed back, then waits for this switch to be turned off.

On EOS M3/M10, you can enter this menu by adding this to eos_handle_uart, under ``Write char``:

.. code:: C

    if (value == '#')
    {
        s->uart.reg_rx = value;
        s->uart.reg_st |= ST_RX_RDY;
    }

This will enable a debug shell; type ``?`` for the available commands.

Cross-checking the emulation with actual hardware
`````````````````````````````````````````````````

- dm-spy-experiments branch
- CONFIG_DEBUG_INTERCEPT_STARTUP=y
- run the same build on both camera and QEMU
- compare the logs (sorry, no good tool for this)
- add extra hooks as desired (dm-spy-extra.c)
- caveat: the order of execution is not deterministic.

Checking MMIO values from actual hardware
'''''''''''''''''''''''''''''''''''''''''

See `this commit <https://bitbucket.org/hudson/magic-lantern/commits/726806f3bc352c41bbd72bf40fdbab3c7245039d>`_:

- ``./run_canon_fw.sh 5D3 [...] -d io_log``
- copy/paste some entries into ``dm-spy-extra.c`` (grep for ``mmio_log`` to find them)
- get logs from both camera and QEMU (dm-spy-experiments branch, ``CONFIG_DEBUG_INTERCEPT_STARTUP=y``, maybe also `CONFIG_QEMU=y`)
- adjust the emulation until the logs match.

Checking interrupts from actual hardware
''''''''''''''''''''''''''''''''''''''''

LOG_INTERRUPTS in dm-spy-experiments.

Misc notes
``````````

Model-specific parameters: ``eos/model_list.c`` (todo: move all hardcoded stuff there).

MMIO handlers: ``eos_handlers`` -> ``eos_handle_whatever`` (with ``io_log`` for debug messages).

Useful: ``eos_get_current_task_name/id/stack``, ``eos_mem_read/write``.

To extract MPU messages from a `startup log <http://builds.magiclantern.fm/jenkins/view/Experiments/job/startup-log/>`_,
use `extract_init_spells.py <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/extract_init_spells.py>`_ (see `MPU communication`_).

To customize keys or add support for new buttons or GUI events,
edit `mpu.c <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu.c>`_,
`button_codes.h <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/button_codes.h>`_
and `extract_button_codes.py <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/extract_button_codes.py>`_.

Known MPU messages and properties are exported to `known_spells.h <https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/eos/mpu_spells/known_spells.h>`_.

Committing your changes
```````````````````````

After editing the sources outside the magic-lantern directory, 
first make sure you are on the ``qemu`` branch:

.. code:: shell

  # from the magic-lantern directory
  hg up qemu -C

Then copy your changes back into the ML tree:

.. code:: shell

  # from the qemu directory
  ./copy_back_to_contrib.sh

Then commit as usual, from the ``contrib/qemu`` directory.

Reverting your changes
``````````````````````

If you want to go back to an older changeset, or just undo any changes you
made outside the magic-lantern directory, you may run the install script
again. It will not re-download QEMU, but unfortunately you will have to
recompile QEMU from scratch (which is very slow).

If you have changed only the ``eos`` files, to avoid a full recompilation
you may try a script similar to the following:

.. code:: shell

    #!/bin/bash
    
    QEMU_PATH=${QEMU_PATH:=qemu-2.5.0}
    ML_PATH=${ML_PATH:=../magic-lantern}

    cp -v $ML_PATH/contrib/qemu/eos/* $QEMU_PATH/hw/eos/
    cp -v $ML_PATH/contrib/qemu/eos/mpu_spells/* $QEMU_PATH/hw/eos/mpu_spells/
    cp -v $ML_PATH/contrib/qemu/eos/dbi/* $QEMU_PATH/hw/eos/dbi/
    cp -v $ML_PATH/src/backtrace.[ch] $QEMU_PATH/hw/eos/dbi/
    cp -vr $ML_PATH/contrib/qemu/tests/* tests/
    cp -vr $ML_PATH/contrib/qemu/scripts/* .


Test suite
``````````

Most Canon cameras are very similar inside — which is why one is able to run the same codebase
from DIGIC 2 (original 5D) all the way to DIGIC 5 (and soon 6). Yet, every camera model has its own quirks
(not only on the firmware, but also on the hardware side). Therefore, it's hard to predict whether a tiny change in the emulation, to fix a quirk for camera model X,
will have a positive or negative or neutral impact on camera model Y. The test suite tries to answer this,
and covers the following:

- Bootloader code (to make sure AUTOEXEC.BIN is loaded from the card)
- Portable display test (all EOS models)
- Portable ROM dumper (EOS models with bootloader file write routines)
- Menu navigation (on supported models) — depends on user settings from the ROM
- Card formatting (and restoring ML)
- Call/return trace until booting the GUI (a rigid test that may have to be updated frequently)
- Call/return trace on bootloader (likely independent of firmware version and user settings)
- Callstack consistency with call/return trace (at every DebugMsg call)
- File I/O (whether the firmware creates a DCIM directory on startup)
- FA_CaptureTestImage (basic image capture process, without compression or CR2 output)
- HPTimer (difficult to get right)
- DryOS task information (current_task, current_interrupt)
- GDB scripts (just a few basics)
- DryOS shell (UART)
- PowerShot models (limited tests)
- Secondary DryOS cores (limited tests)

Limitations:

- The tests are tied to our copies of the ROMs (which also store various user settings);
  unfortunately, these ROMs are not public (see `ML FAQ <http://wiki.magiclantern.fm/faq>`_).
  
  Workarounds:
  
  - run the test suite for your camera model(s) only, e.g. ``./run_tests.sh 5D3 60D 70D``
  - inspect the test results (e.g. screenshots) manually, and compare them to
    `our results from Jenkins <http://builds.magiclantern.fm/jenkins/view/QEMU/job/QEMU-tests/>`_
    to decide whether they are correct or not
  - if you have made changes to the emulation, just ask us to test them.

  Saving the ROM right after clearing camera settings may or may not give repeatable results (not tested).

- The test suite is very slow (30-60 minutes, even on decent hardware)

  Workarounds:

  - run the test suite for a small number of camera model(s): ``./run_tests.sh 5D3 60D 70D``
  - run only the test(s) you are interested in: ``./run_tests.sh 5D3 80D menu calls-main drysh``

  If you have any ideas on how to improve the tests, we are listening.

To avoid committing (large) reference screenshots or log files,
a lot of expected test results are stored as MD5 sums. That's a bit rigid,
but it does the job for now. Where appropriate, we also have grep-based
tests or custom logic on log files.

The expected test results ("`needles <http://open.qa/docs/#_needles>`_") are updated manually
(e.g. ``md5sum disp.ppm > disp.md5``). Suggestions welcome.

Code coverage?
``````````````

`Yes <http://builds.magiclantern.fm/jenkins/view/QEMU/job/QEMU-coverage/>`_.


----------

..

----------

`Back to README.rst <README.rst#rst-header-hacking>`_
