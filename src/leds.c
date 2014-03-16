#include "dryos.h"

#if defined(CONFIG_7D) // pel: Checked. That's how it works in the 7D firmware
static void _card_led_on()  //See sub_FF32B410 -> sub_FF0800A4
{
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = 0x800c00;
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDON); //0x138000
}

static void _card_led_off()  //See sub_FF32B424 -> sub_FF0800B8
{
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = 0x800c00;
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDOFF); //0x38400
}

#elif defined(CARD_LED_ADDRESS) && defined(LEDON) && defined(LEDOFF)
static void _card_led_on()  { *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDON); }
static void _card_led_off() { *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDOFF); }
#else
#warn Make sure you define CARD_LED_ADDRESS, LEDON and LEDOFF to have working LEDS
static void _card_led_on()  { return; }
static void _card_led_off() { return; }
#endif

void info_led_on()
{
    #ifdef CONFIG_VXWORKS
    LEDBLUE = LEDON;
    #elif defined(CONFIG_BLUE_LED)
    call("EdLedOn");
    #else
    _card_led_on();
    #endif
}
void info_led_off()
{
    #ifdef CONFIG_VXWORKS
    LEDBLUE = LEDOFF;
    #elif defined(CONFIG_BLUE_LED)
    call("EdLedOff");
    #else
    _card_led_off();
    #endif
}

void info_led_blink(int times, int delay_on, int delay_off)
{
    for (int i = 0; i < times; i++)
    {
        info_led_on();
        msleep(delay_on);
        info_led_off();
        msleep(delay_off);
    }
}
