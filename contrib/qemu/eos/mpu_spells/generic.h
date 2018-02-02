 /**
 * Minimal generic spell set based on 60D (deleted everything that did not prevent booting)
 * 
 * Without card support (comment out PROP_CARD2_EXISTS), it boots most EOS models (menu navigation works)
 * Tested on: 5D2 5D3 6D 50D 60D 70D 450D 500D 550D 600D 650D 700D 100D 1000D 1100D 1200D 1300D EOSM2.
 * Not working: EOSM.
 * Does something: 80D, 750D, 760D.
 *
 * With SD card support (unmodified - PROP_CARD2_EXISTS enabled), it initializes SD
 * on all the above EOS models, but may crash at EstimatedSize on the most recent ones.
 * Even if it crashes, the SD card is initialized and the firmware can save files to the virtual card.
 * Working (menu navigation, card format): 60D 500D 550D 600D 650D 700D 1100D 1200D 1300D.
 * Working (menu navigation, card recognized): 450D 1000D.
 * Assert at EstimatedSize, dumpf works: 5D3 6D 70D 80D 100D EOSM2.
 * Not working EOSM.
 * 
 * With CF card support (use PROP_CARD1_EXISTS, 0x21 -> 0x20):
 * Working: 50D 5D2.
 * Not working: 5D3.
 * 
 */


static struct mpu_init_spell mpu_init_spells_generic[] = {
    { { 0x06, 0x04, 0x02, 0x00, 0x00 }, .description = "Init", .out_spells = { /* spell #1 */
        { 0x2c, 0x2a, 0x02, 0x00, 0x03, 0x03, 0x03, 0x04, 0x03, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x14, 0x50, 0x00, 0x00, 0x00, 0x00, 0x81, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x4b, 0x01 },/* reply #1.17, Init */
        { 0x06, 0x05, 0x01, 0x21, 0x01, 0x00 },                 /* reply #1.3, PROP_CARD2_EXISTS */

        { 0 } } },

    #include "NotifyGUIEvent.h"
    #include "UILock.h"
    #include "CardFormat.h"
    #include "GPS.h"
    #include "Shutdown.h"
};
