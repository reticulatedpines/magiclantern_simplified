 /*#################################################################################
 #                                                                                 #
 #                          _____     _       _                                    #
 #                         |  ___|   | |     | |                                   #
 #                         |___ \  __| |_ __ | |_   _ ___                          #
 #                             \ \/ _` | '_ \| | | | / __|                         #
 #                         /\__/ / (_| | |_) | | |_| \__ \                         #
 #                         \____/ \__,_| .__/|_|\__,_|___/                         #
 #                                     | |                                         #
 #                                     |_|                                         #
 #                                                                                 #
 ###################################################################################
 #                                                                                 #
 #  USE ONLY WITH v1.1.1 FIRMWARE. I AM NOT RESPONSIBLE FOR ANYTHING THAT HAPPENS  #
 #  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  #
 #                                                                                 #
 #  This will toggle the bootdisk flag for autoexec.bin booting. It calls the boot #
 #  flag functions in the bootloader to set / clear the flags.                     #
 #                                                                                 #
 #################################################################################*/

#define LEDBLUE         *(int*)0xC02200F0
#define LEDRED          *(int*)0xC02200A0
#define LEDON           0x46
#define LEDOFF          0x44
#define BOOT_FLAG       0xF8000000
#define FLAG_BUF_SIZE   (0x80 / sizeof(int))

typedef void (*ft_read_bootflag)(void* buf1, void* buf2);
typedef void (*ft_write_bootflag)(void* buf1, void* buf2);
typedef void (*ft_write_card_bootflag)(int arg0);


/* Wait function. */
void sleep(int n)
{
    int i,j;
    static volatile int k = 0;
    for (i = 0; i < n; i++)
        for (j = 0; j < 300000; j++)
            k++;
}



void ledon()
{
    LEDBLUE = LEDON;
}


void blink()
{
    while(1)
    {
        LEDBLUE = LEDON;
        LEDRED = LEDON;
        sleep(100);
        LEDRED = LEDOFF;
        LEDBLUE = LEDOFF;
        sleep(100);
    }
}


/* Set the bootflags. This will always set the flags to the camera default, and invert the
 * bootdisk flag. This way we have an easy way to recover from something like the main
 * firmware flag being disabled (where you need a FIR file to boot the camera). This is a
 * mix of the 350d bootflag code and my own to make it work for the 5dc. Any red led means
 * that something went wrong. A solid blue led means that the bootdisk is now enabled, and
 * a blinking blue led means that the bootdisk is now disabled.
 */
void toggle_bootdisk()
{
    //~ Declare our pointer functions, buffer, and boot_flag location variable.
    ft_read_bootflag read_bootflag;
    ft_write_bootflag write_bootflag;
    ft_write_card_bootflag write_card_bootflag;
    int buf1[FLAG_BUF_SIZE];
    int *boot_flag = (int*)BOOT_FLAG;
    
    
    //~ Verify that the boot_flag functions exist / are in the correct spots in the
    //~ bootloader. Calling bootloader functions is risky, especially blind! Blink the
    //~ red led and stop execution if this test fails.
    if (*(int*)0xFFFF8AE0 != 0xE52DE004 || *(int*)0xFFFF89F0 != 0xE92D41F0)
    {
        while(1)
        {
            LEDRED = LEDON;
            sleep(3);
            LEDRED = LEDOFF;
            sleep(3);
        }
    }
    
    
    /* I located these functions by hand using the 400d bootloader as a reference. I had
     * to write code to search the bootloader region (0xFFFF0000-0xFFFFFFFF) for signatures
     * of the read_bootflag and write_bootflag functions. It was a very long/tedious process
     * checking each address one at a time - blinking everything through the LEDs. These
     * routines are safe to run to the best of my knowledge, I have not had any issues yet.
     *
     * Note: in the 350d, they copy a small section of the bootloader into memory and run
     * their bootflag functions from their RAM copy. I could not make this work as I do not
     * have a copy of the 5dc bootloader. Calling the bootloader functions directly works.
     *
     * Here are the start/end addressses of the bootflag functions in the 5dc bootloader.
     * I used the led blinking / memory scanning method to locate these. They are identical
     * to the 400d's bootloader functions except for write_bootflag which appears to be
     * 0x4 bytes longer than the 400d's function (so it has an extra instruction in there
     * somewhere I guess).
     *
     *
     * 0xFFFF89F0 | start of write_bootflag in 5dc BL.
     * 0xFFFF8A94 | end of write_bootflag in 5dc BL.
     * 0xFFFF8AE0 | start of read_bootflag in 5dc BL.
     * 0xFFFF8B20 | end of read_bootflag in 5dc BL.
     *
     */
    read_bootflag = (ft_read_bootflag)0xFFFF8AE0;
    write_bootflag = (ft_write_bootflag)0xFFFF89F0;
    write_card_bootflag = (ft_write_card_bootflag)0xFFFF4074;
    
    
    //~ Read the current flags into our buffer so we can modify them.
    read_bootflag(0, buf1);
    
    
    //~ Check if read operation properly copied the flags to buf1. If something went wrong
    //~ during the read operation, blink the red led and loop so that we don't run any
    //~ more code.
    if (buf1[0] != boot_flag[0] || buf1[1] != boot_flag[1] || buf1[2] != boot_flag[2])
    {
        while(1)
        {
            LEDRED = LEDON;
            sleep(3);
            LEDRED = LEDOFF;
            sleep(3);
        }
    }
    
    
    //~ Toggle the bootdisk flag, set the others to their default values.
    buf1[0] = 0;
    buf1[1] = (buf1[1] == -1) ? 0 : -1;
    buf1[2] = -1;
    
    
    //~ Write our modified flags.
    write_bootflag(0, buf1);
    
    //~ Make CF card bootable.
    write_card_bootflag(0);
    
    
    
    //~ Final check to see how the write operation went. Red led means something went wrong.
    //~ If all went well, you should see the blue led now. A blinking blue led means the
    //~ bootdisk is now disabled (camera default), and a solid blue led means the bootdisk
    //~ is now enabled (to boot autoexec.bin files).
    if (buf1[0] == boot_flag[0] && buf1[1] == boot_flag[1] && buf1[2] == boot_flag[2])
    {
        if (boot_flag[1] == -1)
        {
            LEDBLUE = LEDON;
            while(1);
        }
        else
        {
            while(1)
            {
                LEDBLUE = LEDON;
                sleep(3);
                LEDBLUE = LEDOFF;
                sleep(3);
            }
        }
    }
}
