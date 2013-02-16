#include "dryos.h"
#include "menu.h"
#include "bmp.h"
#include "math.h"
#include "config.h"
#define _beep_c_
#include "property.h"

extern int gui_state;
extern int file_number;

int beep_playing = 0;

#ifdef CONFIG_BEEP

#define BEEP_LONG -1
#define BEEP_SHORT 0
#define BEEP_WAV -2
// positive values: X beeps
static int beep_type = 0;
static int record_flag = 0;

CONFIG_INT("beep.enabled", beep_enabled, 1);
CONFIG_INT("beep.volume", beep_volume, 3);
CONFIG_INT("beep.freq.idx", beep_freq_idx, 11); // 1 KHz
CONFIG_INT("beep.wavetype", beep_wavetype, 0); // square, sine, white noise

static int beep_freq_values[] = {55, 110, 220, 262, 294, 330, 349, 392, 440, 494, 880, 1000, 1760, 2000, 3520, 5000, 12000};

void generate_beep_tone(int16_t* buf, int N);

static void asif_stopped_cbr()
{
    beep_playing = 0;
    audio_configure(1);
}
static void asif_stop_cbr()
{
    StopASIFDMADAC(asif_stopped_cbr, 0);
    audio_configure(1);
}
void play_beep(int16_t* buf, int N)
{
    beep_playing = 1;
    SetSamplingRate(48000, 1);
    MEM(0xC0920210) = 4; // SetASIFDACModeSingleINT16
    PowerAudioOutput();
    audio_configure(1);
    SetAudioVolumeOut(COERCE(beep_volume, 1, 5));
    StartASIFDMADAC(buf, N, 0, 0, asif_stop_cbr, 0);
}

void play_beep_ex(int16_t* buf, int N, int sample_rate)
{
    beep_playing = 1;
    SetSamplingRate(sample_rate, 1);
    MEM(0xC0920210) = 4; // SetASIFDACModeSingleINT16
    PowerAudioOutput();
    audio_configure(1);
    SetAudioVolumeOut(COERCE(beep_volume, 1, 5));
    StartASIFDMADAC(buf, N, 0, 0, asif_stop_cbr, 0);
}

void normalize_audio(int16_t* buf, int N)
{
    int m = 0;
    for (int i = 0; i < N/2; i++)
        m = MAX(m, ABS(buf[i]));
    
    for (int i = 0; i < N/2; i++)
        buf[i] = (int)buf[i] * 32767 / m;
}

#ifdef FEATURE_WAV_RECORDING
    #ifndef FEATURE_BEEP
    #error This requires FEATURE_BEEP.
    #endif

// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
// http://www.sonicspot.com/guide/wavefiles.html
static uint8_t wav_header[44] = {
    0x52, 0x49, 0x46, 0x46, // RIFF
    0xff, 0xff, 0xff, 0xff, // chunk size: (file size) - 8
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
    0xff, 0xff, 0xff, 0xff, // data size (bytes)
};


static void wav_set_size(uint8_t* header, int size)
{
    uint32_t* data_size = (uint32_t*)&header[40];
    uint32_t* main_chunk_size = (uint32_t*)&header[4];
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

void WAV_PlaySmall(char* filename)
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
    
    int data_size = *(int*)(buf + data_offset + 4);
    int16_t* data = (int16_t*)(buf + data_offset + 8);
    if (data_size > size - data_offset - 8) { NotifyBox(5000, "WAV: data size wrong"); goto end; }
    
    info_led_on();
    normalize_audio(data, data_size);
    play_beep_ex(data, data_size, sample_rate);
    while (beep_playing) msleep(100);
    info_led_off();
    
end:
    free_dma_memory(buf);
}


static FILE* file = INVALID_PTR;
#define WAV_BUF_SIZE 8192
static int16_t* wav_buf[2] = {0,0};
static int wav_ibuf = 0;

static void asif_continue_cbr()
{
    if (file == INVALID_PTR) return;

    void* buf = wav_buf[wav_ibuf];
    int s = 0;
    if (beep_playing != 2) s = FIO_ReadFile( file, buf, WAV_BUF_SIZE );
    if (!s) 
    { 
        FIO_CloseFile(file);
        file = INVALID_PTR;
        asif_stop_cbr(); 
        info_led_off();
        return; 
    }
    SetNextASIFDACBuffer(buf, s);
    wav_ibuf = !wav_ibuf;
}

void WAV_Play(char* filename)
{
    uint8_t* buf1 = (uint8_t*)(wav_buf[0]);
    uint8_t* buf2 = (uint8_t*)(wav_buf[1]);
    if (!buf1) return;
    if (!buf2) return;

    unsigned size;
    if( FIO_GetFileSize( filename, &size ) != 0 ) return;

    if( file != INVALID_PTR ) return;
    file = FIO_Open( filename, O_RDONLY | O_SYNC );
    if( file == INVALID_PTR ) return;

    int s1 = FIO_ReadFile( file, buf1, WAV_BUF_SIZE );
    int s2 = FIO_ReadFile( file, buf2, WAV_BUF_SIZE );

    // find the "fmt " subchunk
    int fmt_offset = wav_find_chunk(buf1, s1, 0x20746d66);
    if (MEM(buf1+fmt_offset) != 0x20746d66) goto wav_cleanup;
    int sample_rate = *(uint32_t*)(buf1 + fmt_offset + 12);
    int channels = *(uint16_t*)(buf1 + fmt_offset + 10);
    int bitspersample = *(uint16_t*)(buf1 + fmt_offset + 22);
    
    // find the "data" subchunk
    int data_offset = wav_find_chunk(buf1, s1, 0x61746164);
    if (MEM(buf1+data_offset) != 0x61746164) goto wav_cleanup;
    
    uint8_t* data = buf1 + data_offset + 8;
    int N1 = s1 - data_offset - 8;
    int N2 = s2;

    beep_playing = 1;
    SetSamplingRate(sample_rate, 1);
    // 1 = mono uint8
    // 3 = stereo uint8
    // 4 = mono int16
    // 6 = stereo int16
    // => bit 2 = 16bit, bit 1 = stereo, bit 0 = 8bit
    MEM(0xC0920210) = (channels == 2 ? 2 : 0) | (bitspersample == 16 ? 4 : 1); // SetASIFDACMode*
    wav_ibuf = 0;
#ifdef CONFIG_600D
    PowerAudioOutput();
    StartASIFDMADAC(data, N1, buf2, N2, asif_continue_cbr, 0);
    audio_configure(1);
#else
    PowerAudioOutput();
    audio_configure(1);
    SetAudioVolumeOut(COERCE(beep_volume, 1, 5));
    
    StartASIFDMADAC(data, N1, buf2, N2, asif_continue_cbr, 0);
#endif
    return;
    
wav_cleanup:
    FIO_CloseFile(file);
    file = INVALID_PTR;
}

static int audio_recording = 0;
static int audio_recording_start_time = 0;
static void asif_rec_stop_cbr()
{
    audio_recording = 0;
}

static void record_show_progress()
{
    int s = get_seconds_clock() - audio_recording_start_time;
    bmp_printf(
        FONT_LARGE,
        50, 50,
        "Recording... %02d:%02d", s/60, s%60
    );
}

void WAV_RecordSmall(char* filename, int duration, int show_progress)
{
    int N = 48000 * 2 * duration;
    uint8_t* wav_buf = alloc_dma_memory(sizeof(wav_header) + N);
    if (!wav_buf) 
    {
        NotifyBox(2000, "WAV: not enough memory");
        return;
    }

    int16_t* buf = (int16_t*)(wav_buf + sizeof(wav_header));
    
    my_memcpy(wav_buf, wav_header, sizeof(wav_header));
    wav_set_size(wav_buf, N);

    info_led_on();
    audio_recording = 1;
    audio_recording_start_time = get_seconds_clock();

    SetSamplingRate(48000, 1);
    MEM(0xC092011C) = 4; // SetASIFADCModeSingleINT16

    StartASIFDMAADC(buf, N, 0, 0, asif_rec_stop_cbr, N);
    while (audio_recording) 
    {
        msleep(100);
        if (show_progress) record_show_progress();
    }
    info_led_off();
    msleep(1000);
       
    FILE* f = FIO_CreateFileEx(filename);
    FIO_WriteFile(f, UNCACHEABLE(wav_buf), sizeof(wav_header) + N);
    FIO_CloseFile(f);
    free_dma_memory(wav_buf);
}

#endif

static void audio_stop_playback()
{
#ifdef FEATURE_WAV_RECORDING
    if (beep_playing && file != INVALID_PTR) 
    {
        info_led_on();
        beep_playing = 2; // the CBR will stop the playback and close the file properly
        while (beep_playing) msleep(100);
        ASSERT(file == INVALID_PTR);
    }
    else // simple beep, just stop it 
#endif
        asif_stop_cbr();
}

#ifdef FEATURE_WAV_RECORDING

static void audio_stop_recording()
{
    info_led_on();
    audio_recording = 2; // the CBR will stop recording and close the file properly
    while (audio_recording) msleep(100);
    ASSERT(file == INVALID_PTR);
}
#endif

int audio_stop_rec_or_play() // true if it stopped anything
{
    #ifndef FEATURE_WAV_RECORDING
    int audio_recording = 0;
    #endif
    int ans = beep_playing || audio_recording;
    if (beep_playing) audio_stop_playback();
    if (audio_recording) audio_stop_recording();
    return ans;
}

#ifdef FEATURE_WAV_RECORDING
typedef struct _write_q {
    int multiplex;
    void *buf;
    struct _write_q *next;
}WRITE_Q;

#define QBUF_SIZE 4
#define QBUF_MAX 20
WRITE_Q *rootq;

static void add_write_q(void *buf){
    WRITE_Q *tmpq = rootq;
    WRITE_Q *newq;

    int i=0;
    while(tmpq->next){
        tmpq = tmpq->next;
        i++;
    }
    if(i > QBUF_MAX){
        NotifyBox(2000,"Lost WAV data\nUse more faster card");
        return;
    }

    if(tmpq->multiplex < QBUF_SIZE){
        if(!tmpq->buf){
            tmpq->buf = alloc_dma_memory(WAV_BUF_SIZE*QBUF_SIZE);
        }
        int offset = WAV_BUF_SIZE * tmpq->multiplex;
        memcpy(tmpq->buf + offset,buf,WAV_BUF_SIZE);
        tmpq->multiplex++;
    }else{
        newq = SmallAlloc(sizeof(WRITE_Q));
        memset(newq,0,sizeof(WRITE_Q));
        newq->buf = alloc_dma_memory(WAV_BUF_SIZE*QBUF_SIZE);
        memcpy(newq->buf ,buf,WAV_BUF_SIZE);
        newq->multiplex++;
        tmpq->next = newq;
    }
}

static void write_q_dump(){
    WRITE_Q *tmpq = rootq;
    WRITE_Q *prevq;

    while(tmpq->next){
        prevq = tmpq;
        tmpq = tmpq->next;
        FIO_WriteFile(file, UNCACHEABLE(tmpq->buf), WAV_BUF_SIZE * tmpq->multiplex);
        free_dma_memory(tmpq->buf);
        prevq->next = tmpq->next;
        SmallFree(tmpq);
        tmpq = prevq;
    }
}



static void asif_rec_continue_cbr()
{
    if (file == INVALID_PTR) return;

    void* buf = wav_buf[wav_ibuf];
    add_write_q(buf);

    if (audio_recording == 2)
    {
        FIO_CloseFile(file);
        file = INVALID_PTR;
        audio_recording = 0;
        info_led_off();
        return;
    }
    SetNextASIFADCBuffer(buf, WAV_BUF_SIZE);
    wav_ibuf = !wav_ibuf;
}

void WAV_Record(char* filename, int show_progress)
{
    uint8_t* buf1 = (uint8_t*)wav_buf[0];
    uint8_t* buf2 = (uint8_t*)wav_buf[1];
    if (!buf1) return;
    if (!buf2) return;

    if( file != INVALID_PTR ) return;
    file = FIO_CreateFileEx(filename);
    if( file == INVALID_PTR ) return;
    FIO_WriteFile(file, UNCACHEABLE(wav_header), sizeof(wav_header));
    
    audio_recording = 1;
    audio_recording_start_time = get_seconds_clock();

    SetSamplingRate(48000, 1);
    MEM(0xC092011C) = 4; // SetASIFADCModeSingleINT16

    wav_ibuf = 0;        
    
#if defined(CONFIG_7D)
    /* experimental for 7D now, has to be made generic */
    void SoundDevActiveIn (uint32_t);
    SoundDevActiveIn(0);
#endif

    StartASIFDMAADC(buf1, WAV_BUF_SIZE, buf2, WAV_BUF_SIZE, asif_rec_continue_cbr, 0);
    while (audio_recording) 
    {
        msleep(100);
        if (show_progress) record_show_progress();
    }
#if defined(CONFIG_7D)
    /* experimental for 7D now, has to be made generic */
    void SoundDevShutDownIn();
    SoundDevShutDownIn();
#endif
    
    info_led_off();
}

static MENU_UPDATE_FUNC(record_display)
{
    if (audio_recording)
    {
        int s = get_seconds_clock() - audio_recording_start_time;
        MENU_SET_NAME("Recording...");
        MENU_SET_VALUE("%02d:%02d", s/60, s%60);
        MENU_SET_ICON(MNI_NAMED_COLOR, (intptr_t)"Red");
    }
    else
    {
        MENU_SET_NAME("Record new audio clip");
        MENU_SET_ICON(MNI_ACTION, 0);
    }
}

#endif

 void generate_beep_tone(int16_t* buf, int N)
{
    int beep_freq = beep_freq_values[beep_freq_idx];
    
    // for sine wave: 1 hz => t = i * 2*pi*MUL / 48000
    int twopi = 102944;
    float factor = (int)roundf((float)twopi / 48000.0f * beep_freq);
    
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
                int ang = t % twopi;
                #define MUL 16384
                int s = sinf((float)ang / MUL) * MUL;

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
    if (audio_stop_rec_or_play()) return;
#ifdef CONFIG_600D
    if (AUDIO_MONITORING_HEADPHONES_CONNECTED){
        NotifyBox(2000,"600D does not support\nPlay and monitoring together");
        return;
    }
#endif
    beep_type = BEEP_LONG;
    give_semaphore(beep_sem);
}

void unsafe_beep()
{
    if (!beep_enabled)
    {
        info_led_blink(1,50,50); // silent warning
        return;
    }

    if (audio_stop_rec_or_play()) return;
#ifdef CONFIG_600D
    if (AUDIO_MONITORING_HEADPHONES_CONNECTED){
        NotifyBox(2000,"600D does not support\nPlay and monitoring together");
        return;
    }
#endif
    beep_type = BEEP_SHORT;
    give_semaphore(beep_sem);
}

void beep_times(int times)
{
    times = COERCE(times, 1, 100);
    
    if (!beep_enabled || recording)
    {
        info_led_blink(times,50,50); // silent warning
        return;
    }

    if (audio_stop_rec_or_play()) return;

    beep_type = times;
    give_semaphore(beep_sem);
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

#ifdef FEATURE_WAV_RECORDING

static void wav_playback_do();
static void wav_record_do();


static void write_q_task()
{
    TASK_LOOP
    {
        if (audio_recording==1 || rootq->next)
        {
            write_q_dump();
        }
        msleep(500);
    }
}

TASK_CREATE( "write_q_task", write_q_task, 0, 0x16, 0x1000 );
#endif

static void beep_task()
{
    TASK_LOOP
    {
        take_semaphore( beep_sem, 0 );
        
        if (record_flag)
        {
            #ifdef FEATURE_WAV_RECORDING
            wav_record_do();
            #endif
            record_flag = 0;
            continue;
        }
        
        if (beep_type == BEEP_WAV)
        {
            #ifdef FEATURE_WAV_RECORDING
            wav_playback_do();
            #endif
        }
        else if (beep_type == BEEP_LONG)
        {
            info_led_on();
            int N = 48000*5;
            int16_t* beep_buf = SmallAlloc(N*2);
            if (!beep_buf) { N = 48000; beep_buf = SmallAlloc(N*2); } // not enough RAM, try a shorter tone
            if (!beep_buf) { N = 10000; beep_buf = SmallAlloc(N*2); } // even shorter
            if (!beep_buf) continue; // give up
            generate_beep_tone(beep_buf, N);
            play_beep(beep_buf, N);
            while (beep_playing) msleep(100);
            SmallFree(beep_buf);
            info_led_off();
        }
        else if (beep_type == BEEP_SHORT)
        {
            int N = 5000;
            int16_t* beep_buf = SmallAlloc(N*2);
            if (!beep_buf) continue; // give up
            generate_beep_tone(beep_buf, 5000);
            play_beep(beep_buf, 5000);
            while (beep_playing) msleep(20);
            SmallFree(beep_buf);
        }
        else if (beep_type > 0) // N beeps
        {
            int times = beep_type;
            int N = 10000;
            int16_t* beep_buf = SmallAlloc(N*2);
            if (!beep_buf) continue;
            generate_beep_tone(beep_buf, N);
            
            for (int i = 0; i < times/10; i++)
            {
                play_beep(beep_buf, 10000);
                while (beep_playing) msleep(100);
                msleep(500);
            }
            for (int i = 0; i < times%10; i++)
            {
                play_beep(beep_buf, 3000);
                while (beep_playing) msleep(20);
                msleep(120);
            }
            
            SmallFree(beep_buf);
        }
        msleep(100);
        audio_configure(1);
    }
}

TASK_CREATE( "beep_task", beep_task, 0, 0x18, 0x1000 );

#ifdef FEATURE_WAV_RECORDING

// that's extremely inefficient
static int find_wav(int * index, char* fn)
{
    struct fio_file file;
    struct fio_dirent * dirent = 0;
    int N = 0;
    
    dirent = FIO_FindFirstEx( get_dcim_dir(), &file );
    if( IS_ERROR(dirent) )
    {
        return 0; // can be safely ignored
    }

    do {
        if (file.mode & 0x10) continue; // is a directory
        int n = strlen(file.name);
        if ((n > 4) && (streq(file.name + n - 4, ".WAV")))
            N++;
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);

    static int old_N = 0;
    if (N != old_N) // number of files was changed, select the last one
    {
        old_N = N;
        *index = N-1;
    }
    
    *index = mod(*index, N);

    dirent = FIO_FindFirstEx( get_dcim_dir(), &file );
    if( IS_ERROR(dirent) )
    {
        bmp_printf( FONT_LARGE, 40, 40, "find_wav: dir err" );
        return 0;
    }

    int k = 0;
    int found = 0;
    do {
        if (file.mode & 0x10) continue; // is a directory
        int n = strlen(file.name);
        if ((n > 4) && (streq(file.name + n - 4, ".WAV")))
        {
            if (k == *index)
            {
                snprintf(fn, 100, "%s/%s", get_dcim_dir(), file.name);
                found = 1;
            }
            k++;
        }
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
    return found;
}

static char current_wav_filename[100];

void find_next_wav(void* priv, int dir)
{
    static int index = -1;

    index += dir;
    
    if (find_wav(&index, current_wav_filename))
    {
        // OK, nothing to do
    }
    else
    {
        snprintf(current_wav_filename, sizeof(current_wav_filename), "(no WAV files)");
    }
}

static MENU_UPDATE_FUNC(filename_display)
{
    // display only the filename, without the path
    char* fn = current_wav_filename + strlen(current_wav_filename) - 1;
    while (fn > current_wav_filename && *fn != '/') fn--; if (*fn == '/') fn++;
    
    MENU_SET_VALUE(
        fn
    );
}

static void wav_playback_do()
{
    if (beep_playing) return;
    if (audio_recording) return;
    WAV_Play(current_wav_filename);
}
#endif

static void playback_start(void* priv, int delta)
{
    if (audio_stop_rec_or_play()) return;
#ifdef CONFIG_600D
    if (AUDIO_MONITORING_HEADPHONES_CONNECTED){
        NotifyBox(2000,"600D does not support\nPlay and monitoring together");
        return;
    }
#endif
    beep_type = BEEP_WAV;
    give_semaphore(beep_sem);
}

static char* wav_get_new_filename()
{
    static char imgname[100];
    int wav_number = 1;
    
    if (QR_MODE)
    {
        snprintf(imgname, sizeof(imgname), "%s/IMG_%04d.WAV", get_dcim_dir(), file_number);
        return imgname;
    }
    
    else if (recording)
    {
        snprintf(imgname, sizeof(imgname), "%s/MVI_%04d.WAV", get_dcim_dir(), file_number);
        return imgname;
    }
    
    for ( ; wav_number < 10000; wav_number++)
    {
        snprintf(imgname, sizeof(imgname), "%s/SND_%04d.WAV", get_dcim_dir(), wav_number);
        unsigned size;
        if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
        if (size == 0) break;
    }
    return imgname;
}

#ifdef FEATURE_WAV_RECORDING

static void wav_notify_filename()
{
    // display only the filename, without the path
    char* fn = current_wav_filename + strlen(current_wav_filename) - 1;
    while (fn > current_wav_filename && *fn != '/') fn--; fn++;
    NotifyBox(1000, "Sound: %s", fn);
}

static void wav_record_do()
{
    if (beep_playing) return;
    if (audio_recording) return;
    int q = QR_MODE;
    char* fn = wav_get_new_filename();
    snprintf(current_wav_filename, sizeof(current_wav_filename), fn);
    if (recording) wav_notify_filename();
    else msleep(100); // to avoid the noise from shortcut key
    WAV_Record(fn, q);
    if (q)
    {
        redraw();
        wav_playback_do();
    }
}

static void record_start(void* priv, int delta)
{
    if (audio_stop_rec_or_play()) return;

    if (recording && sound_recording_mode != 1)
    {
        NotifyBox(2000, 
            "Cannot record WAV sound \n"
            "with Canon audio enabled"
        );
        return ;
    }

    record_flag = 1;
    give_semaphore(beep_sem);
}

static void delete_file(void* priv, int delta)
{
    if (beep_playing || audio_recording) return;
    
    FIO_RemoveFile(current_wav_filename);
    find_next_wav(0,1);
}
#endif

static CONFIG_INT("voice.tags", voice_tags, 0);

#ifdef FEATURE_VOICE_TAGS
    #ifndef FEATURE_WAV_RECORDING
    #error This requires FEATURE_WAV_RECORDING.
    #endif

int handle_voice_tags(struct event * event)
{
    if (!voice_tags) return 1;
    if (event->param == BGMT_PRESS_SET)
    {
        if (audio_recording)
        {
            audio_stop_recording();
            return 0;
        }
        if (QR_MODE)
        {
            record_start(0,0);
            return 0;
        }
    }
    return 1;
}
#endif

#ifdef FEATURE_WAV_RECORDING
PROP_HANDLER( PROP_MVR_REC_START )
{
    if (!fps_should_record_wav() && !hibr_should_record_wav()) return;
    int rec = buf[0];
    if (rec == 1) record_start(0,0);
    else if (rec == 0) audio_stop_recording();
}
#endif

MENU_UPDATE_FUNC(beep_update)
{
    MENU_SET_ENABLED(beep_enabled);
}

static struct menu_entry beep_menus[] = {
    #ifdef FEATURE_BEEP
    {
        .name = "Beep, test tones",
        .select = menu_open_submenu,
        .update = beep_update,
        .submenu_width = 680,
        .help = "Configure ML beeps and play test tones (440Hz, 1kHz...)",
        .children =  (struct menu_entry[]) {
            {
                .name = "Enable Beeps",
                .priv       = &beep_enabled,
                .max = 1,
                .help = "Enable beep signal for misc events in ML.",
            },
#if !defined(CONFIG_7D)
            {
                .name = "Beep Volume",
                .priv       = &beep_volume,
                .min = 1,
                .max = 5,
                .icon_type = IT_PERCENT,
                //~ .select = beep_volume_toggle,
                .help = "Volume for ML beep and test tones (1-5).",
            },
#endif
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
                //~ .update = play_test_tone_print,
                .icon_type = IT_ACTION,
                .select = play_test_tone,
                .help = "Play a 5-second test tone with current settings.",
            },
            {
                .name = "Test beep sound",
                //~ .update = play_test_tone_print,
                .icon_type = IT_ACTION,
                .select = unsafe_beep,
                .help = "Play a short beep which will be used for ML events.",
            },
            MENU_EOL,
        }
    },
    #endif
    #ifdef FEATURE_WAV_RECORDING
    {
        .name = "Sound recorder",
        .select = menu_open_submenu,
        .help = "Record short audio clips, add voice tags to pictures...",
        .children =  (struct menu_entry[]) {
            {
                .name = "File name",
                .update = filename_display,
                .select = find_next_wav,
                .help = "Select a file name for playback.",
            },
            {
                .name = "Record",
                .update = record_display,
                .select = record_start,
                .help = "Press SET to start or stop recording.",
            },
            {
                .name = "Playback selected file",
                .select = playback_start,
                .help = "Play back a WAV file, in built-in speaker or headphones.",
            },
            {
                .name = "Delete selected file",
                .select = delete_file,
                .help = "Be careful :)",
            },
            MENU_EOL,
        }
    },
    #endif
    #ifdef FEATURE_VOICE_TAGS
    {
        .name = "Voice Tags", 
        .priv = &voice_tags, 
        .max = 1,
        .help = "After you take a picture, press SET to add a voice tag.",
        .works_best_in = DEP_PHOTO_MODE,
    },
    #endif
};

#if 0 // wtf is that?! start recording at startup?!
#ifdef CONFIG_600D
void Load_ASIFDMAADC(){
    uint8_t* buf1 = (uint8_t*)wav_buf[0];
    uint8_t* buf2 = (uint8_t*)wav_buf[1];
    if (!buf1) return;
    if (!buf2) return;

    audio_recording = 0;
    SetSamplingRate(48000, 1);
    MEM(0xC092011C) = 4; // SetASIFADCModeSingleINT16

    wav_ibuf = 0;
    StartASIFDMAADC(buf1, WAV_BUF_SIZE, buf2, WAV_BUF_SIZE, asif_rec_continue_cbr, 0);
}
#endif
#endif

static void beep_init()
{
#ifdef FEATURE_WAV_RECORDING
    wav_buf[0] = alloc_dma_memory(WAV_BUF_SIZE);
    wav_buf[1] = alloc_dma_memory(WAV_BUF_SIZE);
    
    rootq = SmallAlloc(sizeof(WRITE_Q));
    memset(rootq,0,sizeof(WRITE_Q));
    rootq->multiplex=100;
#endif

    beep_sem = create_named_semaphore( "beep_sem", 0 );
    menu_add( "Audio", beep_menus, COUNT(beep_menus) );

#ifdef FEATURE_WAV_RECORDING
    find_next_wav(0,1);
#endif

//~ #ifdef CONFIG_600D
    //~ Load_ASIFDMAADC();
//~ #endif
}

INIT_FUNC("beep.init", beep_init);

#else // beep not working, keep dummy stubs

    void unsafe_beep(){}
    void beep(){}
    void Beep(){}
    void beep_times(int times){};
    int beep_enabled = 0;
#endif

