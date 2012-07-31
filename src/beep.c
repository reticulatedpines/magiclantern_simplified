#include "dryos.h"
#include "menu.h"
#include "config.h"
#include "cordic-16bit.h"

CONFIG_INT("beep.enabled", beep_enabled, 1);
CONFIG_INT("beep.volume", beep_volume, 3);
CONFIG_INT("beep.freq.idx", beep_freq_idx, 11); // 1 KHz
CONFIG_INT("beep.wavetype", beep_wavetype, 0); // square, sine, white noise

static int beep_freq_values[] = {55, 110, 220, 262, 294, 330, 349, 392, 440, 494, 880, 1000, 1760, 2000, 3520, 5000, 12000};

static void generate_beep_tone(int16_t* buf, int N);

static int16_t beep_buf[5000];

int beep_playing = 0;
static void asif_stopped_cbr()
{
    beep_playing = 0;
}
static void asif_stop_cbr()
{
    StopASIFDMADAC(asif_stopped_cbr, 0);
}
static void play_beep(int16_t* buf, int N)
{
    beep_playing = 1;
    SetSamplingRate(48000, 1);
    MEM(0xC0920210) = 4; // SetASIFDACModeSingleINT16
    PowerAudioOutput();
    SetAudioVolumeOut(COERCE(beep_volume, 1, 5));
    StartASIFDMADAC(buf, N, buf, N, asif_stop_cbr, N);
}

static void asif_continue_cbr()
{
    int16_t* buf = beep_buf;
    int N = 5000;
    StartASIFDMADAC(buf, N, buf, N, asif_continue_cbr, N);
}
void play_continuous_test() // doesn't work well, it pauses
{
    int16_t* buf = beep_buf;
    int N = 5000;
    generate_beep_tone(buf, N);
    beep_playing = 1;
    SetSamplingRate(48000, 1);
    MEM(0xC0920210) = 4; // SetASIFDACModeSingleINT16
    PowerAudioOutput();
    SetAudioVolumeOut(COERCE(beep_volume, 1, 5));
    StartASIFDMADAC(buf, N, buf, N, asif_continue_cbr, N);
}


static void cordic_ex(int theta, int* s, int* c, int n)
{
    theta = mod(theta + 2*half_pi, 4*half_pi) - 2*half_pi; // range: -pi...pi
    if (theta < -half_pi || theta > half_pi)
    {
        if (theta < 0)
            cordic(theta + 2*half_pi, s, c, n);
        else
            cordic(theta - 2*half_pi, s, c, n);
        *s = -(*s);
        *c = -(*c);
    }
    else
    {
        cordic(theta, s, c, n);
    }
}

static void generate_beep_tone(int16_t* buf, int N)
{
    int beep_freq = beep_freq_values[beep_freq_idx];
    
    // for sine wave: 1 hz => t = i * 2*pi*MUL / 48000
    int twopi = 102944;
    float factor = (int)roundf((float)twopi / 48000.0 * beep_freq);
    
    for (int i = 0; i < N; i++)
    {
        switch(beep_wavetype)
        {
            case 0: // square
            {
                int factor = 48000 / beep_freq / 2;
                buf[i] = ((i/factor) % 2) ? 32767 : -32767;
                break;
            }
            case 1: // sine
            {
                int t = (int)(factor * i);
                int s, c;
                cordic_ex(t % twopi, &s, &c, 16);
                buf[i] = COERCE(s*2, -32767, 32767);
                break;
            }
            case 2: // white noise
            {
                buf[i] = rand();
                break;
            }
        }
    }
}

static struct semaphore * beep_sem;

int long_beep = 0;
void play_test_tone()
{
    if (beep_playing)
    {
        asif_stop_cbr();
    }
    else
    {
        long_beep = 1;
        give_semaphore(beep_sem);
    }
}

void unsafe_beep()
{
    if (!beep_enabled)
    {
        info_led_blink(1,50,50); // silent warning
        return;
    }

    if (beep_playing)
    {
        asif_stop_cbr();
    }
    else
    {
        long_beep = 0;
        give_semaphore(beep_sem);
    }
}

void beep()
{
    extern int recording;
    if (!recording) // breaks audio
        unsafe_beep();
}

void Beep()
{
    beep();
}

static void beep_task()
{
    TASK_LOOP
    {
        take_semaphore( beep_sem, 0 );
        
        if (long_beep)
        {
            info_led_on();
            int N = 48000*5;
            int16_t* long_buf = AllocateMemory(N*2);
            if (!long_buf) { N = 48000; long_buf = AllocateMemory(N*2); } // not enough RAM, try a shorter tone
            if (!long_buf)
            {
                generate_beep_tone(beep_buf, 5000);  // really low RAM, do a really short tone
                play_beep(beep_buf, 5000);
                continue;
            }
            generate_beep_tone(long_buf, N);
            play_beep(long_buf, N);
            while (beep_playing) msleep(100);
            FreeMemory(long_buf);
            info_led_off();
        }
        else
        {
            generate_beep_tone(beep_buf, 5000);
            play_beep(beep_buf, 5000);
        }
        msleep(500);
    }
}

TASK_CREATE( "beep_task", beep_task, 0, 0x18, 0x1000 );

static struct menu_entry beep_menus[] = {
    {
        .name = "Beep and test tones...",
        .select = menu_open_submenu,
        .help = "Configure ML beeps and play test tones (440Hz, 1kHz...)",
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "Enable Beeps",
                .priv       = &beep_enabled,
                .max = 1,
                .help = "Enable beep signal for misc events in ML.",
            },
            {
                .name = "Beep Volume",
                .priv       = &beep_volume,
                .min = 1,
                .max = 5,
                .icon_type = IT_PERCENT,
                //~ .select = beep_volume_toggle,
                .help = "Volume for ML beep and test tones (1-5).",
            },
            {
                .name = "Tone Waveform", 
                .priv = &beep_wavetype,
                .max = 2,
                .choices = (const char *[]) {"Square", "Sine", "White Noise"},
                .help = "Type of waveform to be generated: square, sine, white noise.",
            },
            {
                .name = "Tone Frequency",
                .priv       = &beep_freq_idx,
                .max = 16,
                .icon_type = IT_PERCENT,
                //  {55, 110, 220, 262, 294, 330, 349, 392, 440, 494, 880, 1000, 1760, 2000, 3520, 5000, 12000};
                .choices = (const char *[]) {"55 Hz (A1)", "110 Hz (A2)", "220 Hz (A3)", "262 Hz (Do)", "294 Hz (Re)", "330 Hz (Mi)", "349 Hz (Fa)", "392 Hz (Sol)", "440 Hz (La-A4)", "494 Hz (Si)", "880 Hz (A5)", "1 kHz", "1760 Hz (A6)", "2 kHz", "3520 Hz (A7)", "5 kHz", "12 kHz"},
                .help = "Frequency for ML beep and test tones (Hz).",
            },
            {
                .name = "Play test tone",
                //~ .display = play_test_tone_print,
                .icon_type = IT_ACTION,
                .select = play_test_tone,
                .help = "Play a 5-second test tone with current settings.",
            },
            {
                .name = "Test beep sound",
                //~ .display = play_test_tone_print,
                .icon_type = IT_ACTION,
                .select = unsafe_beep,
                .help = "Play a short beep which will be used for ML events.",
            },
            MENU_EOL,
        }
    },
};

static void beep_init()
{
    beep_sem = create_named_semaphore( "beep_sem", 0 );
    menu_add( "Audio", beep_menus, COUNT(beep_menus) );
}

INIT_FUNC("beep.init", beep_init);
