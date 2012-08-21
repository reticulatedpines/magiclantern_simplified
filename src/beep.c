#include "dryos.h"
#include "menu.h"
#include "config.h"
#include "cordic-16bit.h"

int beep_playing = 0;

#if defined(CONFIG_50D) || defined(CONFIG_5DC) // beep not working, keep dummy stubs
    void unsafe_beep(){}
    void beep(){}
    void Beep(){}
    void beep_times(int times){};
    int beep_enabled = 0;
#else // beep working

extern int recording; // don't beep while recording, it may break audio

#define BEEP_LONG -1
#define BEEP_SHORT 0
// positive values: X beeps
static int beep_type = 0;

CONFIG_INT("beep.enabled", beep_enabled, 1);
CONFIG_INT("beep.volume", beep_volume, 3);
CONFIG_INT("beep.freq.idx", beep_freq_idx, 11); // 1 KHz
CONFIG_INT("beep.wavetype", beep_wavetype, 0); // square, sine, white noise

static int beep_freq_values[] = {55, 110, 220, 262, 294, 330, 349, 392, 440, 494, 880, 1000, 1760, 2000, 3520, 5000, 12000};

void generate_beep_tone(int16_t* buf, int N);

static int16_t beep_buf[5000];

static void asif_stopped_cbr()
{
    beep_playing = 0;
    audio_force_reconfigure();
}
static void asif_stop_cbr()
{
    StopASIFDMADAC(asif_stopped_cbr, 0);
    audio_force_reconfigure();
}
void play_beep(int16_t* buf, int N)
{
    beep_playing = 1;
    SetSamplingRate(48000, 1);
    MEM(0xC0920210) = 4; // SetASIFDACModeSingleINT16
    PowerAudioOutput();
    audio_force_reconfigure();
    SetAudioVolumeOut(COERCE(beep_volume, 1, 5));
    StartASIFDMADAC(buf, N, buf, N, asif_stop_cbr, N);
}

void play_beep_ex(int16_t* buf, int N, int sample_rate)
{
    beep_playing = 1;
    audio_force_reconfigure();
    SetSamplingRate(sample_rate, 1);
    MEM(0xC0920210) = 4; // SetASIFDACModeSingleINT16
    PowerAudioOutput();
    audio_force_reconfigure();
    SetAudioVolumeOut(COERCE(beep_volume, 1, 5));
    StartASIFDMADAC(buf, N, buf, N, asif_stop_cbr, N);
}

static void asif_continue_cbr()
{
    int16_t* buf = beep_buf;
    int N = 5000;
    SetNextASIFADCBuffer(buf, N);
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


void normalize_audio(int16_t* buf, int N)
{
    int m = 0;
    for (int i = 0; i < N/2; i++)
        m = MAX(m, ABS(buf[i]));
    
    for (int i = 0; i < N/2; i++)
        buf[i] = (int)buf[i] * 32767 / m;
}

// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
// http://www.sonicspot.com/guide/wavefiles.html
static uint8_t wav_header[44] = {
    0x52, 0x49, 0x46, 0x46, // RIFF
    0x00, 0x00, 0x00, 0x00, // chunk size: (file size) - 8
    0x57, 0x41, 0x56, 0x45, // WAVE
    0x66, 0x6d, 0x74, 0x20, // fmt 
    0x10, 0x00, 0x00, 0x00, // subchunk size = 16
    0x01, 0x00,             // PCM uncompressed
    0x01, 0x00,             // mono
    0x80, 0xbb, 0x00, 0x00, // 48000 Hz
    0x00, 0x77, 0x01, 0x00, // 96000 bytes / second
    0x02, 0x00,             // 2 bytes / sample
    0x10, 0x00,             // 16 bits / sample
    0x64, 0x61, 0x74, 0x61, // data
    0x00, 0x00, 0x00, 0x00, // data size (bytes)
};


static void wav_set_size(uint8_t* header, int size)
{
    uint32_t* data_size = &header[40];
    uint32_t* main_chunk_size = &header[4];
    *data_size = size;
    *main_chunk_size = size + 36;
}

static int wav_find_chunk(uint8_t* buf, int size, uint32_t chunk_code)
{
    int offset = 12; // start after RIFFnnnnWAVE
    while (offset < size && *(uint32_t*)(buf + offset) != chunk_code)
        offset += *(uint32_t*)(buf + offset + 4) + 8;
    if (*(uint32_t*)(buf + offset) != chunk_code) 
    { 
        NotifyBox(5000, "WAV: subchunk not found");
        return 0;
    }
    return offset;
}

void WAV_Play(char* filename)
{
    int size = 0;
    uint8_t* buf = (uint8_t*)read_entire_file(filename, &size);
    if (!size) return;
    if (!buf) return;
    extern int beep_playing;

    // find the "fmt " subchunk
    int fmt_offset = wav_find_chunk(buf, size, 0x20746d66);
    if (!fmt_offset) goto end;
    int sample_rate = *(uint32_t*)(buf + fmt_offset + 12);
    
    // find the "data" subchunk
    int data_offset = wav_find_chunk(buf, size, 0x61746164);
    if (!data_offset) goto end;
    
    uint32_t data_size = *(uint32_t*)(buf + data_offset + 4);
    uint8_t* data = buf + data_offset + 8;
    if (data_size > size - data_offset - 8) { NotifyBox(5000, "WAV: data size wrong"); goto end; }
    
    normalize_audio(data, data_size);
    play_beep_ex(data, data_size, sample_rate);
    while (beep_playing) msleep(100);

end:
    free_dma_memory(buf);
}

static int rec_done = 0;
static void asif_rec_stop_cbr()
{
    rec_done = 1;
}

void WAV_Record(char* filename, int duration)
{
    int N = 48000 * 2 * duration;
    uint8_t* wav_buf = alloc_dma_memory(sizeof(wav_header) + N);
    int16_t* buf = (int16_t*)(wav_buf + sizeof(wav_header));
    if (!buf) return;
    
    my_memcpy(wav_buf, wav_header, sizeof(wav_header));
    wav_set_size(wav_buf, N);

    info_led_on();
    rec_done = 0;
    SetSamplingRate(48000, 1);
    MEM(0xC092011C) = 4; // SetASIFDACModeSingleINT16
    StartASIFDMAADC(buf, N, 0, 0, asif_rec_stop_cbr, N);
    while (!rec_done) msleep(100);
    info_led_off();
    msleep(1000);
       
    FILE* f = FIO_CreateFileEx(filename);
    FIO_WriteFile(f, UNCACHEABLE(wav_buf), sizeof(wav_header) + N);
    FIO_CloseFile(f);
    free_dma_memory(wav_buf);
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

 void generate_beep_tone(int16_t* buf, int N)
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

void play_test_tone()
{
    if (beep_playing)
    {
        asif_stop_cbr();
    }
    else
    {
        beep_type = BEEP_LONG;
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
        beep_type = BEEP_SHORT;
        give_semaphore(beep_sem);
    }
}

void beep_times(int times)
{
    times = COERCE(times, 1, 10);
    
    if (!beep_enabled || recording)
    {
        info_led_blink(times,50,50); // silent warning
        return;
    }

    if (beep_playing)
    {
        asif_stop_cbr();
    }
    else
    {
        beep_type = times;
        give_semaphore(beep_sem);
    }
}

void beep()
{
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
        
        if (beep_type == BEEP_LONG)
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
        else if (beep_type == BEEP_SHORT)
        {
            generate_beep_tone(beep_buf, 5000);
            play_beep(beep_buf, 5000);
            while (beep_playing) msleep(20);
        }
        else if (beep_type > 0) // N beeps
        {
            int N = beep_type;
            generate_beep_tone(beep_buf, 5000);
            for (int i = 0; i < N; i++)
            {
                play_beep(beep_buf, 3000);
                while (beep_playing) msleep(20);
                msleep(70);
            }
        }
        msleep(100);
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

#endif
