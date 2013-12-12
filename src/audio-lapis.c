/** \file
 * Onscreen audio meters
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "dryos.h"
#include "bmp.h"
#include "config.h"
#include "property.h"
#include "menu.h"
#include "gui.h"
#include "audio-common.c"

// Set defaults
CONFIG_INT( "audio.override_audio", cfg_override_audio,   0 );
CONFIG_INT( "audio.analog_gain",    cfg_analog_gain,      0 );
CONFIG_INT( "audio.analog_boost",   cfg_analog_boost,     0 ); //test
CONFIG_INT( "audio.enable_dc",      cfg_filter_dc,        1 );
CONFIG_INT( "audio.enable_hpf2",    cfg_filter_hpf2,      0 );
CONFIG_INT( "audio.hpf2config",     cfg_filter_hpf2config,7 );

CONFIG_INT( "audio.dgain",          cfg_recdgain,         0 );
CONFIG_INT( "audio.dgain.l",        dgain_l,              8 );
CONFIG_INT( "audio.dgain.r",        dgain_r,              8 );
CONFIG_INT( "audio.effect.mode",    cfg_effect_mode,      0 );


int audio_meters_are_drawn()
{
    if(!cfg_override_audio){
        return 0;
    }
    return audio_meters_are_drawn_common();
}

static void audio_monitoring_update()
{
    if (cfg_override_audio == 0) return;
    
    // kill video connect/disconnect event... or not
    *(int*)HOTPLUG_VIDEO_OUT_STATUS_ADDR = audio_monitoring ? 2 : 0;
        
    if (audio_monitoring && rca_monitor)
        {
            audio_monitoring_force_display(0);
            msleep(1000);
            audio_monitoring_display_headphones_connected_or_not();
        }
    else{
        audio_ic_set_lineout_onoff(OP_STANDALONE);
    }
}


static void
audio_ic_set_mute_on(unsigned int wait){
    if(audio_monitoring){
        audio_ic_write(ML_HP_AMP_VOL | 0x0E); // headphone mute
    }
    masked_audio_ic_write(ML_AMP_VOLFUNC_ENA,0x02,0x02); //mic in vol to -12
    msleep(wait);
}
static void
audio_ic_set_mute_off(unsigned int wait){
    msleep(wait);
    masked_audio_ic_write(ML_AMP_VOLFUNC_ENA,0x02,0x00);
    if(audio_monitoring){
        audio_ic_set_lineout_vol();
    }
}

static void
audio_ic_set_micboost(){ //600D func lv is 0-8
    if(cfg_override_audio == 0) return;

    //    if(lv > 7 ) lv = 6;
    if(cfg_analog_boost > 6 ) cfg_analog_boost = 6;

    switch(cfg_analog_boost){
    case 0:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_OFF);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF);
        break;
    case 1:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_OFF);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_ON);
        break;
    case 2:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_1);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF);
        break;
    case 3:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_1);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_ON);
        break;
    case 4:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_2);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF);
        break;
    case 5:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_2);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_ON);
        break;
    case 6:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_3);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF); //to be sure
        break;
    }
}

static void
audio_ic_set_effect_mode(){
    if(cfg_override_audio == 0) return;
    short mode[] = {  ML_SND_EFFECT_MODE_NOTCH, 
                      ML_SND_EFFECT_MODE_EQ,
                      ML_SND_EFFECT_MODE_NOTCHEQ,
                      ML_SND_EFFECT_MODE_ENHANCE_REC,
                      ML_SND_EFFECT_MODE_ENHANCE_RECPLAY,
                      ML_SND_EFFECT_MODE_LOUD};

    audio_ic_write(ML_SND_EFFECT_MODE | mode[cfg_effect_mode]);

}
static void
audio_ic_set_analog_gain(){
    if(cfg_override_audio == 0) return;

	int volumes[] = { 0x10, 0x18, 0x24, 0x30, 0x3c, 0x3f};
    //mic in vol 0-7 0b1-0b111111
    audio_ic_write(ML_MIC_IN_VOL   | volumes[cfg_analog_gain]);   //override mic in volume
}

static void
audio_ic_set_input(int op_mode){
    if(cfg_override_audio == 0) return;

    if(op_mode) audio_ic_set_mute_on(30);
    if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_STOP); //descrived in pdf p71
    
    switch (get_input_source())
        {
        case 0: //LR internal mic
            audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_COLD); // 
            audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_COLD); // 
            audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICR2LCH_MICR2RCH); // 
            audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            break;
        case 1:// L internal R extrenal
            audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_HOT); //
            audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_COLD); // 
            audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICL2LCH_MICR2RCH); //
            audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            break;
        case 2:// LR external
            audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_HOT); //
            audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_HOT); //
            audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICL2LCH_MICR2RCH); // 
            audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            break;
        case 3://L internal R balranced (used for test)
            audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_HOT); //1
            audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_DIFFER);       //2: 1and2 are combination value
            audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_COLD); // 
            audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICL2LCH_MICR2RCH); //
            break;
        case 4: //int out auto
            if(mic_inserted){
                audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_HOT); //
                audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_HOT); //
                audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICL2LCH_MICR2RCH); // 
                audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            }else{
                audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_COLD); // 
                audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_COLD); // 
                audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICR2LCH_MICR2RCH); // 
                audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            }
            break;
        }
    
    if(audio_monitoring){
        if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_MON); // monitor mode
    }else{
        if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_AUTO_ON | ML_RECPLAY_STATE_REC);
    }
    if(op_mode) audio_ic_set_mute_off(80);

}

static void
audio_ic_set_RecLRbalance(){
    if(cfg_override_audio == 0) return;

    int val = dgain_l<<4;
    val = val | dgain_r;
    audio_ic_write( ML_REC_LR_BAL_VOL | val);
}

static void
audio_ic_set_filters(int op_mode){
    if(cfg_override_audio == 0) return;

    if(op_mode) audio_ic_set_mute_on(30);
    int val = 0;
    if(cfg_filter_dc) val = 0x1;
    if(cfg_filter_hpf2) val = val | 0x2;
    audio_ic_write(ML_FILTER_EN | val);
    if(val){
        audio_ic_write(ML_HPF2_CUTOFF | cfg_filter_hpf2config);
    }else{
        audio_ic_write(ML_FILTER_EN | 0x3);
    }
    if(op_mode) audio_ic_set_mute_off(80);
}

static void
audio_ic_set_agc(){
    if(cfg_override_audio == 0) return;

    if(alc_enable){
        masked_audio_ic_write(ML_DVOL_CTL_FUNC_EN, 0x03, 0x03);
    }else{
        masked_audio_ic_write(ML_DVOL_CTL_FUNC_EN, 0x03, 0x00);
    }
}

static void
audio_ic_off(){

    audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_OFF);
    audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF);
    audio_ic_write(ML_MIC_IN_VOL | ML_MIC_IN_VOL_2);
    audio_ic_write(ML_PW_ZCCMP_PW_MNG | 0x00); //power off
    audio_ic_write(ML_PW_REF_PW_MNG | ML_PW_REF_PW_MNG_ALL_OFF);

    audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_STOP);
    audio_ic_write(ML_HPF2_CUTOFF | ML_HPF2_CUTOFF_FREQ200);
    audio_ic_write(ML_FILTER_EN | ML_FILTER_DIS_ALL);
    audio_ic_write(ML_REC_LR_BAL_VOL | 0x00);
    audio_ic_set_lineout_onoff(OP_MULTIPLE);
}

static void
audio_ic_on(){
    if(cfg_override_audio == 0) return;
    audio_ic_write(ML_PW_ZCCMP_PW_MNG | ML_PW_ZCCMP_PW_MNG_ON); //power on
    audio_ic_write(ML_PW_IN_PW_MNG | ML_PW_IN_PW_MNG_BOTH);   //DAC(0010) and PGA(1000) power on
    masked_audio_ic_write(ML_PW_REF_PW_MNG,0x7,ML_PW_REF_PW_MICBEN_ON | ML_PW_REF_PW_HISPEED);
    //    audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_REC);
    audio_ic_write(ML_AMP_VOLFUNC_ENA | ML_AMP_VOLFUNC_ENA_FADE_ON);
    audio_ic_write(ML_MIXER_VOL_CTL | 0x10);
}

static void
audio_ic_set_lineout_vol(){
    int vol = MIN(lovl*2, 11) + 0x0E + 38; // lovl=0 => default to 0 dB; max 11 dB
    audio_ic_write(ML_HP_AMP_VOL | vol);

    //This can be more boost headphone monitoring volume.Need good menu interface
    audio_ic_write(ML_PLYBAK_BOST_VOL | ML_PLYBAK_BOST_VOL_DEF );

}


static void
audio_ic_set_lineout_onoff(int op_mode){
    //PDF p38
    if(audio_monitoring &&
       AUDIO_MONITORING_HEADPHONES_CONNECTED &&
       cfg_override_audio==1){
        if(op_mode) audio_ic_set_mute_on(30);
        
        if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_STOP); //directly change prohibited p55

        audio_ic_write(ML_PW_REF_PW_MNG | ML_PW_REF_PW_HP_STANDARD | ML_PW_REF_PW_MICBEN_ON | ML_PW_REF_PW_HISPEED); //HeadPhone amp-std voltage(HPCAP pin voltage) gen circuit power on.
        audio_ic_write(ML_PW_IN_PW_MNG | ML_PW_IN_PW_MNG_BOTH ); //adc pga on
        audio_ic_write(ML_PW_DAC_PW_MNG | ML_PW_DAC_PW_MNG_PWRON); //DAC power on
        audio_ic_write(ML_PW_SPAMP_PW_MNG | (0xFF & ~ML_PW_SPAMP_PW_MNG_ON));
        audio_ic_write(ML_HP_AMP_OUT_CTL | ML_HP_AMP_OUT_CTL_ALL_ON);
        audio_ic_write(ML_AMP_VOLFUNC_ENA | ML_AMP_VOLFUNC_ENA_FADE_ON );
        audio_ic_write(ML_DVOL_CTL_FUNC_EN | ML_DVOL_CTL_FUNC_EN_ALC_FADE );
        audio_ic_set_lineout_vol();

        if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_MON); // monitor mode

        if(op_mode) audio_ic_set_mute_off(80);

    }else{
        if(cfg_override_audio==1){
            if(op_mode) audio_ic_set_mute_on(30);
        
            if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_STOP); //directory change prohibited p55
            audio_ic_write(ML_PW_DAC_PW_MNG | ML_PW_DAC_PW_MNG_PWROFF); //DAC power on
            audio_ic_write(ML_HP_AMP_OUT_CTL | 0x0);
            audio_ic_write(ML_PW_SPAMP_PW_MNG | ML_PW_SPAMP_PW_MNG_OFF);
            
            if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_AUTO_ON | ML_RECPLAY_STATE_REC);
            if(op_mode) audio_ic_set_mute_off(80);
        }
    }
}

static void
audio_ic_set_recdgain(){
    if(cfg_override_audio == 0) return;

    int vol = 0xff - cfg_recdgain;
    masked_audio_ic_write(ML_REC_DIGI_VOL, 0x7f, vol);
}


static void
audio_ic_set_recplay_state(){

    if(audio_monitoring &&
       AUDIO_MONITORING_HEADPHONES_CONNECTED){
        audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_MON); // monitor mode
    }else{
        audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_AUTO_ON | ML_RECPLAY_STATE_REC);
    }

}


void
audio_configure( int force )
{
    extern int beep_playing;
    if (beep_playing && !(audio_monitoring && AUDIO_MONITORING_HEADPHONES_CONNECTED))
        return; // don't redirect wav playing to headphones if they are not connected

#ifdef CONFIG_AUDIO_REG_LOG
    audio_reg_dump( force );
    return;
#endif
#ifdef CONFIG_AUDIO_REG_BMP
    audio_reg_dump_screen();
    return;
#endif

    if(cfg_override_audio == 0){
        //        audio_ic_off();
        return;
    }else{
        audio_ic_set_mute_on(30);
        audio_ic_on();
    }

    audio_set_meterlabel();
    audio_ic_set_input(OP_MULTIPLE);
    audio_ic_set_analog_gain();
    audio_ic_set_micboost();
    audio_ic_set_recdgain();
    audio_ic_set_RecLRbalance();
    audio_ic_set_filters(OP_MULTIPLE);
    audio_ic_set_effect_mode();
    audio_ic_set_agc();
    audio_ic_set_lineout_onoff(OP_MULTIPLE);
    audio_monitoring_update();
    audio_ic_set_recplay_state();

    audio_ic_set_mute_off(80);

}

static void
audio_lovl_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, delta, 0, 6);
    audio_ic_set_lineout_vol();
}

static void
audio_dgain_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, delta, 0, 15);
    audio_ic_set_RecLRbalance();
}


static int
get_dgain_val(int isL){

    float val;
    if(isL){ 
        val = dgain_l;
    }else{   
        val = dgain_r; 
    }
    
    if(val == 8){
        return 0;
    }else if(val < 8){
        return -(8 - val);
    }else if(val > 8){
        return (val - 8);
    }        
    return 0;
}

static MENU_UPDATE_FUNC(audio_dgain_display)
{
    int dgainval = get_dgain_val(entry->priv == &dgain_l ? 1 : 0);
    MENU_SET_VALUE("%d dB", dgainval);
    if (alc_enable)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "AGC is enabled.");
}

static MENU_UPDATE_FUNC(audio_lovl_display)
{
    if (!audio_monitoring)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Headphone monitoring is disabled");
}

static void
audio_alc_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 1);
    audio_ic_set_agc();
}

static void
audio_input_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, delta, 0, 4);
    if(*(unsigned*)priv == 3) *(unsigned*)priv += delta; //tamporaly disabled Ext:balanced. We can't find it.
    audio_ic_set_input(OP_STANDALONE);
}

static void override_audio_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 1);
    audio_configure(2);
}

static void analog_gain_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, delta, 0, 5);
    audio_ic_set_analog_gain();
}

static void analog_boost_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, delta, 0, 6);
    audio_ic_set_micboost();
}

static void audio_effect_mode_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, delta, 0, 5);
    audio_ic_set_effect_mode();
}

/** DSP Filter Function Enable Register p77 HPF1 HPF2 */
static void audio_filter_dc_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, -1, 0, 1);
    audio_ic_set_filters(OP_STANDALONE);
}

static MENU_UPDATE_FUNC(audio_filter_dc_display)
{
    if(cfg_filter_dc == 0){
        // what does this mean?!
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Now you got under 10Hz, but meter is wrong value signed");
    }

}
static void audio_filter_hpf2_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 1);
    audio_ic_set_filters(OP_STANDALONE);
}

static void audio_hpf2config_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, delta, 0, 7);
    audio_ic_set_filters(OP_STANDALONE);
}

static void
audio_filters_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 1);
    audio_ic_set_filters(OP_STANDALONE);
}

static void
audio_filters_toggle_reverse( void * priv, int delta )
{
    menu_numeric_toggle(priv, -1, 0, 1);
    audio_ic_set_filters(OP_STANDALONE);
}

static void
audio_recdgain_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, -2*delta, 0, 126); 
    audio_ic_set_recdgain();
}

static int get_cfg_recdgain(){
    //cfg_recdgain value correcting
    if(cfg_recdgain%2){
        cfg_recdgain = (cfg_recdgain/2)*2;
    }

    if(cfg_recdgain == 0){
        return 0;
    }else{
        return -(cfg_recdgain/2);
    }
}
static MENU_UPDATE_FUNC(audio_recdgain_display)
{
    MENU_SET_VALUE(
        "%d dB",
        get_cfg_recdgain()
    );
    
    if (alc_enable)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "AGC is enabled");

}

static struct menu_entry audio_menus[] = {
    {
        .name        = "Override audio settings",
        .priv           = &cfg_override_audio,
        .max            = 1,
        .select         = override_audio_toggle,
        .depends_on     = DEP_SOUND_RECORDING,
        .help = "Override audio setting by ML",
    },
    {
        .name        = "Analog gain",
        .priv           = &cfg_analog_gain,
        .select         = analog_gain_toggle,
        .max            = 5,
        .icon_type      = IT_PERCENT,
        .choices        = CHOICES("0 dB", "+6 dB", "+15 dB", "+24 dB", "+33 dB", "+35 dB"),
        .depends_on     = DEP_SOUND_RECORDING,
        .help = "Gain applied to both inputs in analog domain.",
    },
    {
        .name        = "Mic Boost",
        .priv           = &cfg_analog_boost,
        .select         = analog_boost_toggle,
        .max            = 6,
        .icon_type      = IT_PERCENT,
        .choices        = CHOICES("0 dB","+5 dB","+10 dB","+15 dB","+20 dB","+25 dB","+30 dB"),
        .depends_on     = DEP_SOUND_RECORDING,
        .help = "Analog mic boost.",
    },
    {
        .name = "Digital Gain", 
        .select = menu_open_submenu, 
        .help = "Digital Volume and R-L gain",
        .depends_on = DEP_SOUND_RECORDING,
        .children =  (struct menu_entry[]) {
            {
                .name = "Record Effect Mode",
                .priv           = &cfg_effect_mode,
                .select         = audio_effect_mode_toggle,
                .max            = 5,
                .icon_type      = IT_DICE,
                .choices        = CHOICES("Notch Filter", "EQ", "Notch EQ", "Enhnc REC", "Enhnc RECPLAY", "Loud"),
                .help           = "Choose mode :Notch,EQ,Notch/EQ,Enhance [12],Loudness.",
            },
            {
                .name = "Record Digital Volume ",
                .priv           = &cfg_recdgain,
                .max            = 126,
                .icon_type      = IT_PERCENT,
                .select         = audio_recdgain_toggle,
                .help = "Record Digital Volume. ",
                .edit_mode = EM_MANY_VALUES,
            },
            {
                .name = "Left Digital Gain ",
                .priv           = &dgain_l,
                .max            = 15,
                .icon_type      = IT_PERCENT,
                .select         = audio_dgain_toggle,
                .update         = audio_dgain_display,
                .help = "Digital gain (LEFT). Any nonzero value reduces quality.",
                .edit_mode = EM_MANY_VALUES,
            },
            {
                .name = "Right Digital Gain",
                .priv           = &dgain_r,
                .max            = 15,
                .icon_type      = IT_PERCENT,
                .select         = audio_dgain_toggle,
                .update         = audio_dgain_display,
                .help = "Digital gain (RIGHT). Any nonzero value reduces quality.",
                .edit_mode = EM_MANY_VALUES,
            },
            {
                .name = "AGC",
                .priv           = &alc_enable,
                .select         = audio_alc_toggle,
                .max            = 1,
                .help = "Automatic Gain Control - turn it off :)",
                //~ .icon_type = IT_DISABLE_SOME_FEATURE_NEG,
                //.essential = FOR_MOVIE, // nobody needs to toggle this, but newbies want to see "AGC:OFF", manual controls are not enough...
            },
            MENU_EOL,
        },
    },
    {
        .name = "Input source",
        .priv           = &input_choice,
        .select         = audio_input_toggle,
        .max            = 4,
        .icon_type      = IT_ALWAYS_ON,
        .choices = (const char *[]) {"Internal mic", "L:int R:ext", "External stereo", "Balanced (N/A)", "Auto int/ext"},
        .help = "Audio input: internal / external / both / auto.",
        .depends_on        = DEP_SOUND_RECORDING,
    },
    {
        .name = "Wind Filter",
        .help = "High pass filter for wind noise reduction. ML26121A.pdf p77",
        .select            =  menu_open_submenu,
        .depends_on        = DEP_SOUND_RECORDING,
        .submenu_width = 650,
        .children =  (struct menu_entry[]) {
            {
                .name = "DC filter",
                .priv              = &cfg_filter_dc,
                .select            = audio_filter_dc_toggle,
                .max               = 1,
                .update            = audio_filter_dc_display,
                .help = "first-order high pass filter for DC cut",
            },
            {
                .name = "High Pass filter",
                .priv              = &cfg_filter_hpf2,
                .select            = audio_filter_hpf2_toggle,
                .max                = 1,
                .help = "second-order high pass filter for noise cut",
            },
            {
                .name = "HPF2 Cutoff Hz",
                .priv           = &cfg_filter_hpf2config,
                .select         = audio_hpf2config_toggle,
                .max            = 7,
                .icon_type      = IT_PERCENT,
                .choices        = CHOICES("80Hz", "100Hz", "130Hz", "160Hz", "200Hz", "260Hz", "320Hz", "400Hz"),
                .help = "Set the cut off frequency for noise reduction",
            },
            MENU_EOL
        }
    },
#ifdef CONFIG_AUDIO_REG_LOG
    {
        .priv           = "Close register log",
        .select         = audio_reg_close,
        .update        = menu_print,
    },
#endif
    /*{
      .priv              = &loopback,
      .select            = audio_binary_toggle,
      .update   = audio_loopback_display,
      },*/
    {
        .name = "Headphone Mon.",
        .priv = &audio_monitoring,
        .max  = 1,
        .select         = audio_monitoring_toggle,
        .help = "Monitoring via A-V jack. Disable if you use a SD display.",
        .depends_on     = DEP_SOUND_RECORDING,
    },
    {
        .name = "Headphone Volume",
        .priv           = &lovl,
        .select         = audio_lovl_toggle,
        .max            = 6,
        .icon_type      = IT_PERCENT,
        .update         = audio_lovl_display,
        .choices        = CHOICES("0 dB","+2 dB","+4 dB","+6 dB","+8 dB","+10 dB","+11 dB"),
        .help = "Output volume for audio monitoring (headphones only).",
        .depends_on     = DEP_SOUND_RECORDING,
    },
/* any reason to turn these off?
    {
        .name = "Audio Meters",
        .priv           = &cfg_draw_meters,
        .max            = 1,
        .help = "Bar peak decay, -40...0 dB, yellow at -12 dB, red at -3 dB.",
        .depends_on     = DEP_GLOBAL_DRAW | DEP_SOUND_RECORDING,
    },
    */
};

static void volume_display()
{
    char dbval[14][4] = {"  0", " +6", "+15", "+24", "+33", "+35"};
    NotifyBox(2000, "Volume: %s + (%d,%d) dB", dbval[cfg_analog_gain], get_dgain_val(1),get_dgain_val(0));
}

void volume_up()
{
    analog_gain_toggle(&cfg_analog_gain,1);
    volume_display();
}

void volume_down()
{
    analog_gain_toggle(&cfg_analog_gain,-1);
    volume_display();
}

static void out_volume_display()
{
    NotifyBox(2000, "Out Volume: %d dB", MIN(lovl*2, 11));
}
void out_volume_up()
{
    audio_lovl_toggle(&lovl,1);
    out_volume_display();
}
void out_volume_down()
{
    audio_lovl_toggle(&lovl,-1);
    out_volume_display();
}

PROP_HANDLER( PROP_AUDIO_VOL_CHANGE_600D )
{
    /* Cannot overwrite audio config direct here!
       Cannon firmware is overwrite after finishing here.So you need to set value with delay
    */
    audio_configure(1);

}

PROP_HANDLER( PROP_PLAYMODE_LAUNCH_600D )
{
    audio_monitoring_update();
}
PROP_HANDLER( PROP_PLAYMODE_VOL_CHANGE_600D )
{
}


static void audio_menus_init()
{
    menu_add( "Audio", audio_menus, COUNT(audio_menus) );
}


INIT_FUNC("audio.init", audio_menus_init);

// for PicoC
#ifdef FEATURE_MIC_POWER
void mic_out(int val)
{
    console_printf("Fixme: not implemented\n");
}
#endif
