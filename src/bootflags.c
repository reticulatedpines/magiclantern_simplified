/** \file
 * Autoboot flag control
 */
#include "dryos.h"
#include "bmp.h"

/* CF/SD device structure. we have two types which have different parameter order and little differences in behavior */
#if !defined(CONFIG_500D) && !defined(CONFIG_50D) && !defined(CONFIG_5D2) && !defined(CONFIG_40D)
struct cf_device
{
    /* type b always reads from raw sectors */
    int (*read_block)(
        struct cf_device * dev,
        void * buf,
        uintptr_t block,
        size_t num_blocks
    );

    int (*write_block)(
        struct cf_device * dev,
        const void * buf,
        uintptr_t block,
        size_t num_blocks
    );
    
    /* is 7D the only one with two null pointers between? */
#if defined(CONFIG_7D)
    void *null_1;
    void *null_2;
#endif

    void * io_control;
    void * soft_reset;
};
#else
struct cf_device
{
    /* If block has the top bit set the physical blocks will be read instead of from the first partition.  Cool. */
    int (*read_block)(
        struct cf_device * dev,
        uintptr_t block,
        size_t num_blocks,
        void * buf
    );

    int (*write_block)(
        struct cf_device * dev,
        uintptr_t block,
        size_t num_blocks,
        const void * buf
    );
    
    void * io_control;
    void * soft_reset;
};

#endif

/** Shadow copy of the NVRAM boot flags stored at 0xF8000000 */
#define NVRAM_BOOTFLAGS     ((void*) 0xF8000000)
struct boot_flags
{
    uint32_t        firmware;   // 0x00
    uint32_t        bootdisk;   // 0x04
    uint32_t        ram_exe;    // 0x08
    uint32_t        update;     // 0x0c
    uint32_t        flag_0x10;
    uint32_t        flag_0x14;
    uint32_t        flag_0x18;
    uint32_t        flag_0x1c;
};

static struct boot_flags * const    boot_flags = NVRAM_BOOTFLAGS;;


extern struct cf_device * const cf_device[];
extern struct cf_device * const sd_device[];

struct partition_table 
{
    uint8_t state; // 0x80 = bootable
    uint8_t start_head;
    uint16_t start_cylinder_sector;
    uint8_t type;
    uint8_t end_head;
    uint16_t end_cylinder_sector;
    uint32_t sectors_before_partition;
    uint32_t sectors_in_partition;
}__attribute__((aligned,packed));


/*
 * recompute a exFAT VBR checksum in sector 12
 */

static uint32_t VBRChecksum( unsigned char octets[], int NumberOfBytes) {
   uint32_t Checksum = 0;
   int Index;
   for (Index = 0; Index < NumberOfBytes; Index++) {
     if (Index != 106 && Index != 107 && Index != 112)  // skip 'volume flags' and 'percent in use'
	 Checksum = ((Checksum <<31) | (Checksum>> 1)) + (uint32_t) octets[Index];
   }
   return Checksum;
}

static void exfat_sum(uint32_t* buffer) // size: 12 sectors (0-11)
{
    int i=0;
    uint32_t sum;

    sum = VBRChecksum((unsigned char*)buffer, (512*11));

    //~ NotifyBox(2000, "before: %x %x\nafter: %x ", buffer[(512*11)/4], buffer[(512*11)/4 + 1], sum); msleep(2000);
    
    // fill sector 11 with the checksum, repeated
    for(i=0; i<512; i+=4)
        buffer[(512*11+i)/4] = sum;
}


// http://www.datarescue.com/laboratory/partition.htm
// http://magiclantern.wikia.com/wiki/Bootdisk
#if !defined(CONFIG_500D) && !defined(CONFIG_50D) && !defined(CONFIG_5D2) && !defined(CONFIG_40D)
int
bootflag_write_bootblock( void )
{
#if defined(CONFIG_7D)
    struct cf_device * const dev = (struct cf_device *) cf_device[6];
#elif defined(CONFIG_5D3)
    /* dual card slot */
    int ml_on_cf = (get_ml_card()->drive_letter[0] == 'A');
    extern struct cf_device ** cf_device_ptr[];
    struct cf_device * const dev = (struct cf_device *) (ml_on_cf ? cf_device_ptr[0][4] : sd_device[1]);
#else
    struct cf_device * const dev = (struct cf_device *) sd_device[1];
#endif

    if (!dev)
    {
        return 0;
    }

    uint8_t *block = fio_malloc( 512 );
    int i;
    for(i=0 ; i<0x200 ; i++) block[i] = 0xAA;
    
    dev->read_block( dev, block, 0, 1 );

    struct partition_table p;
    extern void fsuDecodePartitionTable(void* partition_address_on_card, struct partition_table * ptable);
    fsuDecodePartitionTable(block + 446, &p);

    //~ NotifyBox(1000, "decoded => %x,%x,%x", p.type, p.sectors_before_partition, p.sectors_in_partition);

    if (p.type == 6 || p.type == 0xb || p.type == 0xc) // FAT16 or FAT32
    {
        int rc = dev->read_block( dev, block, p.sectors_before_partition, 1 );
        int off1 = p.type == 6 ? 0x2b : 0x47;
        int off2 = p.type == 6 ? 0x40 : 0x5c;
        memcpy( block + off1, (uint8_t*) "EOS_DEVELOP", 0xB );
        memcpy( block + off2, (uint8_t*) "BOOTDISK", 0x8 );
        //~ NotifyBox(1000, "writing");
        rc = dev->write_block( dev, block, p.sectors_before_partition, 1 );
        if (rc != 1)
        {
            NotifyBox(2000, "Bootflag write failed\np.type=%d, p.sec = %d, rc=%d", p.type, p.sectors_before_partition, rc); 
            msleep(2000);
            return 0;
        }
    }
    else if (p.type == 7) // ExFAT
    {
        uint8_t* buffer = fio_malloc(512*24);
        dev->read_block( dev, buffer, p.sectors_before_partition, 24 );

        int off1 = 130;
        int off2 = 122;
        memcpy( buffer + off1, (uint8_t*) "EOS_DEVELOP", 0xB );
        memcpy( buffer + off2, (uint8_t*) "BOOTDISK", 0x8 );
        memcpy( buffer + 512*12 + off1, (uint8_t*) "EOS_DEVELOP", 0xB );
        memcpy( buffer + 512*12 + off2, (uint8_t*) "BOOTDISK", 0x8 );
        exfat_sum((uint32_t*)(buffer));
        exfat_sum((uint32_t*)(buffer+512*12));

        dev->write_block( dev, buffer, p.sectors_before_partition, 24 );
        fio_free( buffer );
    }
    else
    {
        fio_free( block );
        NotifyBox(2000, "Unknown partition: %d", p.type); msleep(2000);
        return 0;
    }
    fio_free( block );
    return 1; // success!
}

#else
// 500D doesn't use partition table like the 550d and other cameras.
// This method is a mix of Trammell's old code and a lot of testing / verifying by me.
// -Coutts
int
bootflag_write_bootblock( void )
{
#ifdef CONFIG_500D
    struct cf_device * const dev = sd_device[1];
#elif defined(CONFIG_50D) || defined(CONFIG_5D2) || defined(CONFIG_40D) // not good for 40D, need checking
    struct cf_device * const dev = cf_device[5];
#endif
    
    uint8_t *block = fio_malloc( 512 );
    
    int i;
    for(i=0; i<512; i++) block[i] = 0xAA;

    dev->read_block( dev, 0, 1, block ); //overwrite our AAAs in our buffer with the MBR partition of the SD card.
    
    // figure out if we are a FAT32 partitioned drive. this spells out FAT32 in chars.
    // FAT16 not supported yet - I don't have a small enough card to test with.
    //if( block[0x52] == 0x46 && block[0x53] == 0x41 && block[0x54] == 0x54 && block[0x55] == 0x33 && block[0x56] == 0x32 )
    if( strncmp((const char*) block + 0x52, "FAT32", 5) == 0 ) //check if this card is FAT32
    {
        dev->read_block( dev, 0, 1, block );
        memcpy( block + 0x47, (uint8_t*) "EOS_DEVELOP", 0xB );
        memcpy( block + 0x5C, (uint8_t*) "BOOTDISK", 0xB );
        dev->write_block( dev, 0, 1, block );
    }
    else if( strncmp((const char*) block + 0x36, "FAT16", 5) == 0 ) //check if this card is FAT16
    {
        dev->read_block( dev, 0, 1, block );
        memcpy( block + 0x2B, (uint8_t*) "EOS_DEVELOP", 0xB );
        memcpy( block + 0x40, (uint8_t*) "BOOTDISK", 0xB );
        dev->write_block( dev, 0, 1, block );
    }
    else if( strncmp((const char*) block + 0x3, "EXFAT", 5) == 0 ) //check if this card is EXFAT
    {
        uint8_t* buffer = fio_malloc(512*24);
        dev->read_block( dev, 0, 24, buffer );
        int off1 = 130;
        int off2 = 122;
        memcpy( buffer + off1, (uint8_t*) "EOS_DEVELOP", 0xB );
        memcpy( buffer + off2, (uint8_t*) "BOOTDISK", 0x8 );
        memcpy( buffer + 512*12 + off1, (uint8_t*) "EOS_DEVELOP", 0xB );
        memcpy( buffer + 512*12 + off2, (uint8_t*) "BOOTDISK", 0x8 );
        exfat_sum((uint32_t*)(buffer));
        exfat_sum((uint32_t*)(buffer+512*12));
        dev->write_block( dev, 0, 24, buffer );
        fio_free( buffer );
    }
    else // if it's not FAT16 neither FAT32, don't do anything.
    {
        NotifyBox(2000, "Unknown partition :("); msleep(2000);
        return 0;
    }
    
    fio_free( block );
    return 1;
}
#endif
