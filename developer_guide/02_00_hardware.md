
## Hardware

### Overview

Each camera can be considered to be a distributed system of processors and peripherals, able to communicate via interrupts, shared memory, or message passing.  Not all devices are connected to all others.  Not all communication options are applicable to all devices; you must use what works.

Our knowledge of what devices exist is incomplete.  Our knowledge of what devices can do is limited.  This section will try not to mislead you, but it is bound to contain incomplete information, and likely to contain incorrect information.

#### Digic

Digic (officially: DiG!C) is the main ASIC for Canon cameras.  This contains one or more ARM cores, which run the primary OS, and many peripherals, including DSPs, high precision timers, etc.

Digic exists in many generations, with ML having good support for Digic 4 and 5.  There is no generation 9, this was skipped in preference for the term Digic X.  ML support for generations 6 and 7 has had some success, gens 8 and X have limited support with undiagnosed bugs making stable builds not yet possible.

This document may use D4 to indicate Digic 4, or D678 to indicate any generation from 6 to 8, inclusive - or other similar constructions.

Also see:
<a>https://en.wikipedia.org/wiki/DIGIC</a>
<a>https://wiki.magiclantern.fm/digic</a>

#### Main CPU(s)

The main CPU, from an ML perspective, is termed the ICU.  Our code runs here.  A lot of DryOS code runs here, also.  An important secondary CPU is termed the MPU.  An approximate split of their duties is that the ICU handles UI and CPU intensive tasks, while the MPU handles low-level tasks such as battery management, switch and button handling.  In some senses, the MPU is the primary CPU: it is responsible for detecting power on events and bringing the ICU up.  The MPU is also responsible for turning the cam off should e.g., the battery door be opened.

There are three main families of ARM used across Digic gens that we care about.  D45 uses ARMv5TE.  D6 uses ARMv7-R.  This has a Memory Protection Unit (MPU!  But not the same MPU as above).  D78X use ARMv7-A and have a full MMU.

D45 are single core parts.

D6 are less well understood.  There's a Master and Slave CPU, both ARMv7-R.  These might be on the same chip.

D78X parts are true dual-core, with shared RAM.  They have ARM GIC and can communicate in a standard manner via interrupts.  In addition, DryOS provides at least two RPC mechanisms.

Some cams are described by Canon as Dual Digic.  E.g. the 7D Mark II, which has two independent Digic 6 ASICs.  This probably makes it somewhat equivalent to a dual-socket, dual-core system.

Some cams have a secondary Digic, of a lower generation, used for a subset of tasks.  For example, the 1DX is D5, but uses a D4 for auto-exposure.

Lastly, and least important to ML, some Digic gens come in a normal and "+" variant.  E.g. the 5D Mark III is Digic 5+.  These plus variants don't seem to have extra capabilities, and are likely the same fundamental design, perhaps die-shrunk, and clocked higher.  From a processing power perspective this is important - Canon claims a 3x improvement between some plus and non-plus parts.  From a reverse engineering perspective the differences are minimal.

Points of note:

- All cams can run Thumb instructions
- D45 can use cache locking to "edit" instructions in code ROM
- D78X have MMU so can "edit" large regions of code ROM
- D6 doesn't have a proven way to "edit" ROM

#### Secondary processors

There are further processors for specialised tasks.  Since these are utilised via APIs and / or message passing, their internals are poorly understood.

Tasks that are performance critical (either throughput or latency) may run on some accelerator.  E.g., JPEG compression, video compression.  These are likely ASICs, though there are signs that FPGAs are used in some generations.  Possibly, early implementations use FPGA, which are later converted to ASICs.

Cams with integrated networking typically delegate this to another part.  On D78X this is Xtensa ISA and internally named "Lime".  The ICU can command Lime, which, presumably, directly controls some WiFi SoC.

#### Other peripherals

Not all devices are processors.  There are many MMIO devices, critical to controlling the camera.  Some are digital, e.g. the DMA controller.  Some border on analog, e.g. there's a unit named ADTG which is believed to be a timing generator, used for controlling how the sensor is read out.  Digital configuration, most likely controlling analog hardware.

MMIO addresses are used by the ICU (and potentially other processors) to configure, control, or read information from, these peripherals.

<div style="page-break-after: always; visibility: hidden"></div>
