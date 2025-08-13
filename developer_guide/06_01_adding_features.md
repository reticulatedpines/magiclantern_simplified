## Using EDMAC

_The following is rough notes only and needs re-working_

"Channel": DryOS holds a global array, each element holding an EDMAC register and associated ModeInfo, e.g.:
uint D0004100h channel\_addr
uint 2C05h     ModeInfo

Each element is a "channel" (Digic >= 8 calls these "ports").  The channel\_addr values are often
laid out in groups, evenly separated by 0x100.  They can jump by large amounts between groups.
Presumably the EDMAC unit itself has more registers than channels assigned to DryOS for EDMAC.

On 200D, the channel array starts at 0x66264 and holds 0x35 elements.  I call this DmacInfo\_global.
Many EDmac functions ref this array, including ConnectReadEDmac().  Functions often do a size check
early on to ensure they're not going out of bounds.  Max channel index on 200D is 0x34.

"Resources": these are in essence devices that can be utilised via EDMAC.  
Code often calls these Engine Resources.  "Engine" seems to mean something
like "hardware devices accessed via EDMAC".

The top-level struct for Resources I call ResourceInfo\_global.  This is indexed by blockNum,
on 200D, max of 0x37.  Size is 0x1b8 total, 0x37 size 8 elements; two pointers each.
On 200D, 0x10998 is a pointer to this global.  This represents "blocks" of "entries";
each block is some distinct category or purpose.  Some purposes are known:
ResourceInfo\_global[0] holds EDMAC resources designated for writes, 1 is for reads,
2 is read-write, and so is 3.  Possibly 2 and 3 are different directions, RW vs WR,
which could explain why there are separate groups.

ML *inconsistently* refers to groups 2 and 3 as "connections".  So, when old code notes or docs
mentions an EDMAC connection, this *may* be referring to getting a RW channel via this mechanism.
Sometimes, 2 is called "write connections", 3 "read connections".

As noted, ResourceInfo\_global is an array of pointer pairs.  Each array element represents
a block, but a block contains nothing but entries.  Each pointer has type ResInfo\_entry \*,
the first is entry\_prev, the second entry\_next.  Canon code has a slightly odd trick,
where the prev pointer of the first element in the list, and the next pointer of the last element,
point to the pointer-pair in ResourceInfo\_global.  This isn't a valid ResInfo\_entry, so when
walking the list you must check the pointer address before accessing ResInfo\_entry fields.
This also means that a block containing no entries will consist of two pointers both pointing
to the address of the first pointer.

Before doing anything with EDMAC, you should create a lock for the set of resources / devices you
wish to use, then later take the lock.  You pass a packed array of {u16 entryIndex, u16 blockNum},
each a Resource, and a count, to CreateResLockEntry().  This inserts each Resource into the requested
block (your lock can simultaneously target multiple blocks).  The returned lock can later be
used to either obtain all resources, or non-fatally fail.

ML code doesn't have a struct for Resources, it hard-codes them into a uint32\_t, e.g.:

    resource = 0x00010000 + i; /* read edmac channel */

That's a reference to block 1 (EDmac reads), entry i.


read\_edmacs[] and write\_edmacs[] in edmac.c are found by dumping the contents of blocks 0 and 1.
Are they?  Docs + forum doesn't really specify.  I think this might be blocks 0 + 2, and 1 + 3.

<div style="page-break-after: always; visibility: hidden"></div>
