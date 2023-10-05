
# Magic Lantern

## Architecture

### Running Magic Lantern code

How do we convince cameras to run our code?

DryOS has functionality, disabled by default, to load and run code from a file named autoexec.bin.
This is presumed to be an engineering or testing function.

#### Enabling the camera boot flag

Before DryOS will attempt to run autoexec.bin, a flag must be set in the flash mem / ROM.  There are multiple routes to achieve this, the most common is using a FIR file created by us for this purpose.  When the user triggers the standard Canon firmware update routine, instead of running update code, our FIR file toggles the boot flag.

This is the only direct persistent change we make.  Other persistent changes are made indirectly, since DryOS saves various settings ("properties"), and we may alter their content.  Saving incorrect properties can make a cam unresponsive until they are reset, which may not be practical for the average user - try to never have this occur!

#### Making a card bootable

Boot disks have a few magic bytes set, see contrib/make-bootable for details.

#### autoexec.bin contains Magic Lantern

With the camera boot flag set, DryOS will look for a boot disk. Behaviour is as follows:

- If a boot disk is not found, a normal boot occurs.  Therefore, if you use a card without Magic Lantern installed, the camera will behave as normal.

- If a boot disk is found, but there is no autoexec.bin, the camera will hang.  This is deliberate error handling.  The cam must be powered down and battery removed.  This means if you simply delete ML content from a card, a boot flag enabled camera can appear broken, when it isn't.  Formatting the card removes the boot disk magic.

- If a boot disk is found, and autoexec.bin exists on the root of the filesystem, the contents are copied to address 0x800\_0000 and control is transferred to that address.

The final executable output of the build system for ML is autoexec.bin.  However, our code runs before the OS has started, which includes hardware initialisation.  We want DryOS to do the hard work of bringing the camera fully up.  We inject enough code so that later on we maintain visibility and control.

#### Boot process under ML in detail

It's important to understand how the boot process works if you're interested in porting ML to a new cam, since debugging these early stages is hard without a good mental model.  If you're working on a well supported cam, this is less relevant, since dev work is much closer to "normal": you're writing code that will run at the application layer, on top of a running OS.

DryOS always starts execution at offset 0 of autoexec.bin, and this always occurs at 0x800\_0000 in ram.  This code is in reboot.c, located via symbol name "\_start".  The early asm code does some safety checks, and if they pass transfers control to cstart(), in the same file.

cstart() checks that we're running on the expected cam, erroring if not.  Assuming success, we copy a large portion of autoexec.bin to another location, because the 0x800\_0000 area is used for multiple purposes - later on, DryOS will re-use this region and delete our initial code.  That copy happens here:

```
     blob_memcpy(
         (void*) RESTARTSTART,
         &blob_start,
         &blob_end
     );
```

blob\_start and blob\_end are addresses that cover a region in autoexec.bin.  The build system places "magiclantern.bin" in this region, which is a stripped version of "magiclantern" - which is a normal ELF binary.  This allows us to have a compact representation in the limited camera memory, while having a relatively normal build process, with usable debug symbols.

We then do a small amount of hardware specific initialisation, found via reverse engineering normal DryOS startup.  Finally, autoexec.bin transfers control to RESTARTSTART; the address in mem of our copied magiclantern.bin.  RESTARTSTART is defined in platform/XXXD.YYY/Makefile.platform.default.  It's used in src/magiclantern.lds.S when linking magiclantern.

Which code is at the start of magiclantern.bin and therefore called as RESTARTSTART is defined by entry.S - a function called copy\_and\_restart().  We're now running from the new location and the copy of autoexec.bin at 0x800\_0000 is no longer needed.

There are multiple source files containing a copy\_and\_restart() function, but only one is used per cam.  Selection occurs as part of the per platform config, e.g.:

```
    platform/60D.111/Makefile.platform.default:ML_BOOT_OBJ   = boot-d45-ch.o
```

Therefore for 60D, boot-d45-ch.c will be used.  The code is split like this as there are multiple different ways to reserve space for ML, depending on memory layout and physical capabilities of a cam.

copy\_and\_restart() has the job of making a copy of Canon's early init code in ram, then modifying it for our purposes.  This is where we inject ML functionality.  We must also fixup code broken by the relocation (e.g. PC relative addressing).  We then transfer to this code, allowing DryOS to initialise but with our modifications.

The precise modifications vary a little per cam, but the common idea is that DryOS initialisation uses pointers to various functions and data structures; we have substituted our own.  Part of this ensures that the memory region we copied magiclantern.bin to is outside of the memory DryOS will use - we tell the OS it has a smaller region to work with, and ML fits inside the "missing" memory.

If you look at this code you may see the name ROMBASEADDR.  It's worth noting this is a misleading name.  It's nothing to do with the base address of the rom, it is in fact the start address of the main firmware; the code after the bootloader, responsible for bringing up the OS.

We've got out of the bootloader!  Things are simpler from now on.

To summarise:

- autoexec.bin loads at 0x800\_0000, running code from reboot.c
- magiclantern.bin, contained in autoexec.bin, copied to a safe location in ram
- RESTARTSTART() called, which is an alias for copy\_and\_restart(), from boot-XX.c
- Canon init code relocated to a safe writable location, modified to inject ML
- Modified Canon init code called: DryOS initialises with ML injected

More information about how and where ML injects into DryOS are in the next section.

### ML injection points

## Build guide

<div style="page-break-after: always; visibility: hidden"/>
<div style="visibility: visible"/>
