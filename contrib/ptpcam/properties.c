
/* properties.c
 *
 * Copyright (C) 2005 Mariusz Woloszyn <emsi@ipartners.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"

#include "ptp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#define SVALLEN		256
#define SVALRET(s) { \
			if (n>=SVALLEN) s[SVALLEN]='\0'; \
			return s;\
}

int
ptp_property_issupported(PTPParams* params, uint16_t property)
{
	int i=0;

	for (;i<params->deviceinfo.DevicePropertiesSupported_len;i++) {
		if (params->deviceinfo.DevicePropertiesSupported[i]==property)
			return 1;
	}
	return 0;
}

static struct {
	uint16_t dpc;
	const char *txt;
} ptp_device_properties[] = {
	{PTP_DPC_Undefined,		N_("PTP Undefined Property")},
	{PTP_DPC_BatteryLevel,		N_("Battery Level")},
	{PTP_DPC_FunctionalMode,	N_("Functional Mode")},
	{PTP_DPC_ImageSize,		N_("Image Size")},
	{PTP_DPC_CompressionSetting,	N_("Compression Setting")},
	{PTP_DPC_WhiteBalance,		N_("White Balance")},
	{PTP_DPC_RGBGain,		N_("RGB Gain")},
	{PTP_DPC_FNumber,		N_("F-Number")},
	{PTP_DPC_FocalLength,		N_("Focal Length")},
	{PTP_DPC_FocusDistance,		N_("Focus Distance")},
	{PTP_DPC_FocusMode,		N_("Focus Mode")},
	{PTP_DPC_ExposureMeteringMode,	N_("Exposure Metering Mode")},
	{PTP_DPC_FlashMode,		N_("Flash Mode")},
	{PTP_DPC_ExposureTime,		N_("Exposure Time")},
	{PTP_DPC_ExposureProgramMode,	N_("Exposure Program Mode")},
	{PTP_DPC_ExposureIndex,		N_("Exposure Index (film speed ISO)")},
	{PTP_DPC_ExposureBiasCompensation, N_("Exposure Bias Compensation")},
	{PTP_DPC_DateTime,		N_("Date Time")},
	{PTP_DPC_CaptureDelay,		N_("Pre-Capture Delay")},
	{PTP_DPC_StillCaptureMode,	N_("Still Capture Mode")},
	{PTP_DPC_Contrast,		N_("Contrast")},
	{PTP_DPC_Sharpness,		N_("Sharpness")},
	{PTP_DPC_DigitalZoom,		N_("Digital Zoom")},
	{PTP_DPC_EffectMode,		N_("Effect Mode")},
	{PTP_DPC_BurstNumber,		N_("Burst Number")},
	{PTP_DPC_BurstInterval,		N_("Burst Interval")},
	{PTP_DPC_TimelapseNumber,	N_("Timelapse Number")},
	{PTP_DPC_TimelapseInterval,	N_("Timelapse Interval")},
	{PTP_DPC_FocusMeteringMode,	N_("Focus Metering Mode")},
	{PTP_DPC_UploadURL,		N_("Upload URL")},
	{PTP_DPC_Artist,		N_("Artist")},
	{PTP_DPC_CopyrightInfo,		N_("Copyright Info")},
	{0,NULL}
};
static struct {
	uint16_t dpc;
	const char *txt;
} ptp_device_properties_EK[] = {
	{PTP_DPC_EK_ColorTemperature,	N_("EK Color Temperature")},
	{PTP_DPC_EK_DateTimeStampFormat,N_("EK Date Time Stamp Format")},
	{PTP_DPC_EK_BeepMode,		N_("EK Beep Mode")},
	{PTP_DPC_EK_VideoOut,		N_("EK Video Out")},
	{PTP_DPC_EK_PowerSaving,	N_("EK Power Saving")},
	{PTP_DPC_EK_UI_Language,	N_("EK UI Language")},
	{0,NULL}
};

static struct {
	uint16_t dpc;
	const char *txt;
} ptp_device_properties_CANON[] = {
	{PTP_DPC_CANON_BeepMode,	N_("CANON Beep Mode")},
	{PTP_DPC_CANON_UnixTime,	N_("CANON Time measured in"
					" secondssince 01-01-1970")},
	{PTP_DPC_CANON_FlashMemory,	N_("CANON Flash Card Capacity")},
	{PTP_DPC_CANON_CameraModel,	N_("CANON Camera Model")},
	{0,NULL}
};
/**
 * Properties reported by Corey Manders and Mehreen Chaudary, revised for
 * D70 by Mariusz Woloszyn
 **/
static struct {
	uint16_t dpc;
	const char *txt;
} ptp_device_properties_NIKON[] = {
	{PTP_DPC_NIKON_ShootingBank,	N_("NIKON Shooting Bank")},
	{PTP_DPC_NIKON_ShootingBankNameA, N_("NIKON Shooting Bank Name A")},
	{PTP_DPC_NIKON_ShootingBankNameB, N_("NIKON Shooting Bank Name B")},
	{PTP_DPC_NIKON_ShootingBankNameC, N_("NIKON Shooting Bank Name C")},
	{PTP_DPC_NIKON_ShootingBankNameD, N_("NIKON Shooting Bank Name D")},
	{PTP_DPC_NIKON_RawCompression,	N_("NIKON Raw Compression")},
	{PTP_DPC_NIKON_WhiteBalanceAutoBias, N_("NIKON White Balance Auto Bias")},
	{PTP_DPC_NIKON_WhiteBalanceTungstenBias, N_("NIKON White Balance Tungsten Bias")},
	{PTP_DPC_NIKON_WhiteBalanceFlourescentBias, N_("NIKON White Balance Flourescent Bias")},
	{PTP_DPC_NIKON_WhiteBalanceDaylightBias, N_("NIKON White Balance Daylight Bias")},
	{PTP_DPC_NIKON_WhiteBalanceFlashBias, N_("NIKON White Balance Flash Bias")},
	{PTP_DPC_NIKON_WhiteBalanceCloudyBias, N_("NIKON White Balance Cloudy Bias")},
	{PTP_DPC_NIKON_WhiteBalanceShadeBias, N_("NIKON White Balance Shade Bias")},
	{PTP_DPC_NIKON_WhiteBalanceColorTemperature, N_("NIKON White Balance Color Temperature")},
	{PTP_DPC_NIKON_ImageSharpening, N_("NIKON Image Sharpening")},
	{PTP_DPC_NIKON_ToneCompensation, N_("NIKON Tone Compensation")},
	{PTP_DPC_NIKON_ColorMode,	N_("NIKON Color Mode")},
	{PTP_DPC_NIKON_HueAdjustment,	N_("NIKON Hue Adjustment")},
	{PTP_DPC_NIKON_NonCPULensDataFocalLength, N_("NIKON Non CPU Lens Data Focal Length")},
	{PTP_DPC_NIKON_NonCPULensDataMaximumAperture, N_("NIKON Non CPU Lens Data Maximum Aperture")},
	{PTP_DPC_NIKON_CSMMenuBankSelect, N_("NIKON CSM Menu Bank Select")},
	{PTP_DPC_NIKON_MenuBankNameA,	N_("NIKON Menu Bank Name A")},
	{PTP_DPC_NIKON_MenuBankNameB,	N_("NIKON Menu Bank Name B")},	
	{PTP_DPC_NIKON_MenuBankNameC,	N_("NIKON Menu Bank Name C")},
	{PTP_DPC_NIKON_MenuBankNameD,	N_("NIKON Menu Bank Name D")},
	{PTP_DPC_NIKON_A1AFCModePriority, N_("NIKON (A1) AFC Mode Priority")},
	{PTP_DPC_NIKON_A2AFSModePriority, N_("NIKON (A2) AFS Mode Priority")},
	{PTP_DPC_NIKON_A3GroupDynamicAF, N_("NIKON (A3) Group Dynamic AF")},
	{PTP_DPC_NIKON_A4AFActivation,	N_("NIKON (A4) AF Activation")},	
	{PTP_DPC_NIKON_A5FocusAreaIllumManualFocus, N_("NIKON (A5) Focus Area Illum Manual Focus")},
	{PTP_DPC_NIKON_FocusAreaIllumContinuous, N_("NIKON Focus Area Illum Continuous")},
	{PTP_DPC_NIKON_FocusAreaIllumWhenSelected, N_("NIKON Focus Area Illum When Selected")},
	{PTP_DPC_NIKON_FocusAreaWrap,	N_("NIKON Focus Area Wrap")},
	{PTP_DPC_NIKON_A7VerticalAFON, N_("NIKON (A7) Vertical AF ON")},
	{PTP_DPC_NIKON_ISOAuto,		N_("NIKON ISO Auto")},
	{PTP_DPC_NIKON_B2ISOStep,	N_("NIKON (B2) ISO Step")},
	{PTP_DPC_NIKON_EVStep,	N_("NIKON EV Step")},
	{PTP_DPC_NIKON_B4ExposureCompEv, N_("NIKON (B4) Exposure Comp Ev")},
	{PTP_DPC_NIKON_ExposureCompensation, N_("NIKON Exposure Compensation by Command Dial only")},
	{PTP_DPC_NIKON_CenterWeightArea, N_("NIKON Center Weighted Area")},
	{PTP_DPC_NIKON_AELockMode,	N_("NIKON AE Lock Mode")},
	{PTP_DPC_NIKON_AELAFLMode,	N_("NIKON AE-L/AF-L Mode")},
	{PTP_DPC_NIKON_MeterOff, N_("NIKON Meter-Off")},
	{PTP_DPC_NIKON_SelfTimer,	N_("NIKON Self Timer")},	
	{PTP_DPC_NIKON_MonitorOff,	N_("NIKON Monitor Off")},
	{PTP_DPC_NIKON_D1ShootingSpeed, N_("NIKON (D1) Shooting Speed")},
	{PTP_DPC_NIKON_ExposureTime, N_("NIKON Exposure Time")},
	{PTP_DPC_NIKON_ACPower, N_("NIKON AC Power")},
	{PTP_DPC_NIKON_D2MaximumShots, N_("NIKON (D2) Maximum Shots")},
	{PTP_DPC_NIKON_D3ExpDelayMode,	N_("NIKON (D3) ExpDelayMode")},	
	{PTP_DPC_NIKON_LongExposureNoiseReduction, N_("NIKON Long Exposure Noise Reduction")},
	{PTP_DPC_NIKON_FileNumberSequence, N_("NIKON File Number Sequence")},
	{PTP_DPC_NIKON_D6ControlPanelFinderRearControl, N_("NIKON (D6) Control Panel Finder Rear Control")},
	{PTP_DPC_NIKON_ControlPanelFinderViewfinder, N_("NIKON Control Panel Finder Viewfinder")},
	{PTP_DPC_NIKON_D7Illumination,	N_("NIKON (D7) Illumination")},
	{PTP_DPC_NIKON_E1FlashSyncSpeed, N_("NIKON (E1) Flash Sync Speed")},
	{PTP_DPC_NIKON_FlashShutterSpeed, N_("NIKON Slowest Flash Shutter Speed")},
	{PTP_DPC_NIKON_E3AAFlashMode, N_("NIKON (E3) AA Flash Mode")},
	{PTP_DPC_NIKON_E4ModelingFlash,	N_("NIKON (E4) Modeling Flash")},
	{PTP_DPC_NIKON_BracketSet, N_("NIKON Bracket Set")},
	{PTP_DPC_NIKON_E6ManualModeBracketing, N_("NIKON (E6) Manual Mode Bracketing")},
	{PTP_DPC_NIKON_BracketOrder, N_("NIKON Bracket Order")},
	{PTP_DPC_NIKON_E8AutoBracketSelection, N_("NIKON (E8) Auto Bracket Selection")},
	{PTP_DPC_NIKON_BracketingSet, N_("NIKON Auto Bracketing Set")},
	{PTP_DPC_NIKON_F1CenterButtonShootingMode, N_("NIKON (F1) Center Button Shooting Mode")},
	{PTP_DPC_NIKON_CenterButtonPlaybackMode, N_("NIKON Center Button Playback Mode")},
	{PTP_DPC_NIKON_F2Multiselector, N_("NIKON (F2) Multiselector")},
	{PTP_DPC_NIKON_F3PhotoInfoPlayback, N_("NIKON (F3) PhotoInfoPlayback")},	
	{PTP_DPC_NIKON_F4AssignFuncButton, N_("NIKON (F4) Assign Function Button")},
	{PTP_DPC_NIKON_F5CustomizeCommDials, N_("NIKON (F5) Customize Comm Dials")},
	{PTP_DPC_NIKON_ReverseCommandDial, N_("NIKON Reverse Command Dials")},
	{PTP_DPC_NIKON_ApertureSetting, N_("NIKON Aperture Setting")},
	{PTP_DPC_NIKON_MenusAndPlayback, N_("NIKON Menus and Playback")},
	{PTP_DPC_NIKON_F6ButtonsAndDials, N_("NIKON (F6) Buttons and Dials")},
	{PTP_DPC_NIKON_NoCFCard,	N_("NIKON No CF Card")},
	{PTP_DPC_NIKON_ImageRotation, N_("NIKON Image Rotation")},
	{PTP_DPC_NIKON_Bracketing, N_("NIKON Bracketing")},
	{PTP_DPC_NIKON_ExposureBracketingIntervalDist, N_("NIKON Exposure Bracketing Interval Distance")},
	{PTP_DPC_NIKON_BracketingProgram, N_("NIKON Bracketing Program")},
	{PTP_DPC_NIKON_WhiteBalanceBracketStep, N_("NIKON White Balance Bracket Step")},
	{PTP_DPC_NIKON_AutofocusLCDTopMode2, N_("NIKON Autofocus LCD Top Mode 2")},
	{PTP_DPC_NIKON_AutofocusArea, N_("NIKON Autofocus Area selector")},
	{PTP_DPC_NIKON_LightMeter,	N_("NIKON Light Meter")},
	{PTP_DPC_NIKON_ExposureApertureLock, N_("NIKON Exposure Aperture Lock")},
	{PTP_DPC_NIKON_MaximumShots,	N_("NIKON Maximum Shots")},
	{PTP_DPC_NIKON_AFLLock, N_("NIKON AF-L Locked")},
        {PTP_DPC_NIKON_BeepOff, N_("NIKON Beep")},
        {PTP_DPC_NIKON_AutofocusMode, N_("NIKON Autofocus Mode")},
        {PTP_DPC_NIKON_AFAssist, N_("NIKON AF Assist Lamp")},
        {PTP_DPC_NIKON_PADVPMode, N_("NIKON Auto ISO shutter limit for P A and DVP Mode")},
        {PTP_DPC_NIKON_ImageReview, N_("NIKON Image Review")},
        {PTP_DPC_NIKON_GridDisplay, N_("NIKON Viewfinder Grid Display")},
        {PTP_DPC_NIKON_AFAreaIllumination, N_("NIKON AF Area Illumination")},
        {PTP_DPC_NIKON_FlashMode, N_("NIKON Flash Mode")},
        {PTP_DPC_NIKON_FlashCommanderMode, N_("NIKON Flash Commander Mode")},
        {PTP_DPC_NIKON_FlashSign, N_("NIKON Flash Signal Indicator")},
        {PTP_DPC_NIKON_GridDisplay, N_("NIKON Grid Display")},
        {PTP_DPC_NIKON_FlashModeManualPower, N_("NIKON Flash Manual Mode Power")},
        {PTP_DPC_NIKON_FlashModeCommanderPower, N_("NIKON Flash Commander Mode Power")},
        {PTP_DPC_NIKON_FlashExposureCompensation, N_("NIKON Flash Exposure Compensation")},
        {PTP_DPC_NIKON_RemoteTimeout, N_("NIKON Remote Timeout")},
        {PTP_DPC_NIKON_ImageCommentString, N_("NIKON Image Comment String")},
        {PTP_DPC_NIKON_ImageCommentAttach, N_("NIKON Image Comment Attach")},
        {PTP_DPC_NIKON_FlashOpen, N_("NIKON Flash Open")},
        {PTP_DPC_NIKON_FlashCharged, N_("NIKON Flash Charged")},
        {PTP_DPC_NIKON_LensID, N_("NIKON Lens ID")},
        {PTP_DPC_NIKON_FocalLengthMin, N_("NIKON Min. Focal Length")},
        {PTP_DPC_NIKON_FocalLengthMax, N_("NIKON Max. Focal Length")},
        {PTP_DPC_NIKON_MaxApAtMinFocalLength, N_("NIKON Max. Aperture at Min. Focal Length")},
        {PTP_DPC_NIKON_MaxApAtMaxFocalLength, N_("NIKON Max. Aperture at Max. Focal Length")},
        {PTP_DPC_NIKON_LowLight, N_("NIKON Low Light Indicator")},
        {PTP_DPC_NIKON_CSMMenu, N_("NIKON CSM Menu")},
        {PTP_DPC_NIKON_OptimizeImage, N_("NIKON Optimize Image")},
        {PTP_DPC_NIKON_AutoExposureLock, N_("NIKON AE Lock")},
        {PTP_DPC_NIKON_AutoFocusLock, N_("NIKON AF Lock")},
        {PTP_DPC_NIKON_CameraOrientation, N_("NIKON Camera orientation")},
        {PTP_DPC_NIKON_BracketingIncrement, N_("NIKON Bracketing Increment")},
        {PTP_DPC_NIKON_Saturation, N_("NIKON Saturation")},

	{0,NULL}
};


/* return ptp property name */

const char*
ptp_prop_getname(PTPParams* params, uint16_t dpc)
{
	int i;
	/* Device Property descriptions */
	for (i=0; ptp_device_properties[i].txt!=NULL; i++)
		if (ptp_device_properties[i].dpc==dpc)
			return (ptp_device_properties[i].txt);

	/*if (dpc|PTP_DPC_EXTENSION_MASK==PTP_DPC_EXTENSION)*/
	switch (params->deviceinfo.VendorExtensionID) {
		case PTP_VENDOR_EASTMAN_KODAK:
			for (i=0; ptp_device_properties_EK[i].txt!=NULL; i++)
				if (ptp_device_properties_EK[i].dpc==dpc)
					return (ptp_device_properties_EK[i].txt);
			break;

		case PTP_VENDOR_CANON:
			for (i=0; ptp_device_properties_CANON[i].txt!=NULL; i++)
				if (ptp_device_properties_CANON[i].dpc==dpc)
					return (ptp_device_properties_CANON[i].txt);
			break;
		case PTP_VENDOR_NIKON:
			for (i=0; ptp_device_properties_NIKON[i].txt!=NULL; i++)
				if (ptp_device_properties_NIKON[i].dpc==dpc)
					return (ptp_device_properties_NIKON[i].txt);
			break;
	

		}
	return NULL;
}

uint16_t
ptp_prop_getcodebyname(PTPParams* params, char* name)
{
	int i;
	for (i=0; ptp_device_properties[i].txt!=NULL; i++)
		if (!strncasecmp(ptp_device_properties[i].txt,name,
			strlen(name)))
			return ptp_device_properties[i].dpc;

	/* XXX*/
	for (i=0; ptp_device_properties_NIKON[i].txt!=NULL; i++)
		if (!strncasecmp(ptp_device_properties_NIKON[i].txt,name,
			strlen(name)))
			return ptp_device_properties_NIKON[i].dpc;

	return 0;
}

/* properties interpretation */

static const char*
ptp_prop_NIKON_d100(PTPParams* params, PTPDevicePropDesc *dpd, char* strval)
{
	static char strvalret[SVALLEN];
	uint32_t val= (uint32_t) strtol (strval, NULL, 10);
	uint16_t numerator=(uint16_t) (val >> 16);
	uint16_t denominator=(uint16_t) val;
	int n;

	n=snprintf(strvalret,SVALLEN,"%i/%i",numerator, denominator);
	SVALRET(strvalret);
}

static const char*
ptp_prop_getdescscale10000(PTPParams* params, PTPDevicePropDesc *dpd, char* strval)
{
	long long int value=strtoll(strval, NULL, 10);
	double floatvalue=(double) value/(double )10000.0;
	static char strvalret[SVALLEN];
	int i,n;
	static struct {
		uint16_t dpc;
		const char *units;
	} prop_units[] = {
		{PTP_DPC_ExposureTime, N_("s")},
		{0, NULL}
	};

	switch (params->deviceinfo.VendorExtensionID) {
		case PTP_VENDOR_NIKON:
			//RETPROPDESC(pd_NIKONN);
			break;
	}

	//RETPROPDESC(pd);
	for (i=0; prop_units[i].dpc!=0; i++)	{ 
		if (prop_units[i].dpc==dpd->DevicePropertyCode) {
			n=snprintf(strvalret,SVALLEN,"%.4f%s",floatvalue,prop_units[i].units);
			SVALRET(strvalret);
		}
	}
	return NULL;
}

static const char*
ptp_prop_getdescscale1000(PTPParams* params, PTPDevicePropDesc *dpd, char* strval)
{
	long long int value=strtoll(strval, NULL, 10);
	double floatvalue=(double) value/(double )1000.0;
	static char strvalret[SVALLEN];
	int i,n;
	static struct {
		uint16_t dpc;
		const char *units;
		int prec;
	} prop_units[] = {
		{PTP_DPC_ExposureBiasCompensation, N_(""),1},
		{0, NULL}
	};
	static struct {
		uint16_t dpc;
		const char *units;
		int prec;
	} prop_units_NIKON[] = {
		{0, NULL}
	};

	switch (params->deviceinfo.VendorExtensionID) {
	case PTP_VENDOR_NIKON:
		for (i=0; prop_units_NIKON[i].dpc!=0; i++)	{ 
			if (prop_units_NIKON[i].dpc==dpd->DevicePropertyCode){
				n=snprintf(strvalret,SVALLEN,"%.*f%s",
					prop_units_NIKON[i].prec,floatvalue,
					prop_units_NIKON[i].units);
				SVALRET(strvalret);
			}
		}
			break;

	}

	for (i=0; prop_units[i].dpc!=0; i++)	{ 
		if (prop_units[i].dpc==dpd->DevicePropertyCode) {
			n=snprintf(strvalret,SVALLEN,"%.*f%s",
				prop_units[i].prec,floatvalue,
				prop_units[i].units);
			SVALRET(strvalret);
		}
	}
	return NULL;
}

static const char*
ptp_prop_getdescscale100(PTPParams* params, PTPDevicePropDesc *dpd, char* strval)
{
	long long int value=strtoll(strval, NULL, 10);
	double floatvalue=(double) value/(double )100.0;
	static char strvalret[SVALLEN];
	int i,n;
	static struct {
		uint16_t dpc;
		const char *units;
		int prec;
	} prop_units[] = {
		{PTP_DPC_FNumber, N_(""),1},
		{PTP_DPC_FocalLength, N_("mm"),0},
		{0, NULL}
	};
	static struct {
		uint16_t dpc;
		const char *units;
		int prec;
	} prop_units_NIKON[] = {
		{PTP_DPC_NIKON_FocalLengthMin, N_(""),0},
		{PTP_DPC_NIKON_FocalLengthMax, N_(""),0},
		{PTP_DPC_NIKON_MaxApAtMinFocalLength, N_(""),1},
		{PTP_DPC_NIKON_MaxApAtMaxFocalLength, N_(""),1},
		{0, NULL}
	};

	switch (params->deviceinfo.VendorExtensionID) {
	case PTP_VENDOR_NIKON:
		for (i=0; prop_units_NIKON[i].dpc!=0; i++)	{ 
			if (prop_units_NIKON[i].dpc==dpd->DevicePropertyCode){
				n=snprintf(strvalret,SVALLEN,"%.*f%s",
					prop_units_NIKON[i].prec,floatvalue,
					prop_units_NIKON[i].units);
				SVALRET(strvalret);
			}
		}
			break;
	}

	for (i=0; prop_units[i].dpc!=0; i++)	{ 
		if (prop_units[i].dpc==dpd->DevicePropertyCode) {
			n=snprintf(strvalret,SVALLEN,"%.*f%s",
				prop_units[i].prec,floatvalue,
				prop_units[i].units);
			SVALRET(strvalret);
		}
	}
	return NULL;
}




static struct {
	uint16_t dpc;
	char *val;
	const char *txt;
} ptp_property_meaning[] = {
	{PTP_DPC_WhiteBalance, "1", N_("Manual")},
	{PTP_DPC_WhiteBalance, "2", N_("Automatic")},
	{PTP_DPC_WhiteBalance, "3", N_("One-push Automatic")},
	{PTP_DPC_WhiteBalance, "4", N_("Daylight")},
	{PTP_DPC_WhiteBalance, "5", N_("Fluorescent")},
	{PTP_DPC_WhiteBalance, "6", N_("Tungsten")},
	{PTP_DPC_WhiteBalance, "7", N_("Flash")},
	{PTP_DPC_FocusMode, "1", N_("Manual")},
	{PTP_DPC_FocusMode, "2", N_("Automatic")},
	{PTP_DPC_FocusMode, "3", N_("Automatic Macro")},
	{PTP_DPC_ExposureMeteringMode, "1", N_("Manual")},
	{PTP_DPC_ExposureMeteringMode, "2", N_("Center-weighted")},
	{PTP_DPC_ExposureMeteringMode, "3", N_("Multi-spot")},
	{PTP_DPC_ExposureMeteringMode, "4", N_("Center-spot")},
	{PTP_DPC_FlashMode, "1", N_("Auto flash")},
	{PTP_DPC_FlashMode, "2", N_("Flash off")},
	{PTP_DPC_FlashMode, "3", N_("Fill flash")},
	{PTP_DPC_FlashMode, "4", N_("Red eye auto")},
	{PTP_DPC_FlashMode, "5", N_("Red eye fill")},
	{PTP_DPC_FlashMode, "6", N_("External flash")},
	{PTP_DPC_ExposureProgramMode, "1", N_("Manual")},
	{PTP_DPC_ExposureProgramMode, "2", N_("Automatic (P)")},
	{PTP_DPC_ExposureProgramMode, "3", N_("Aperture Priority")},
	{PTP_DPC_ExposureProgramMode, "4", N_("Shutter Priority")},
	{PTP_DPC_ExposureProgramMode, "5", N_("Program Creative")},
	{PTP_DPC_ExposureProgramMode, "6", N_("Program Action")},
	{PTP_DPC_ExposureProgramMode, "7", N_("Portrait")},
	{PTP_DPC_StillCaptureMode, "1", N_("Normal")},
	{PTP_DPC_StillCaptureMode, "2", N_("Burst")},
	{PTP_DPC_StillCaptureMode, "3", N_("Timelapse")},
	{PTP_DPC_FocusMeteringMode, "1", N_("Center-spot")},
	{PTP_DPC_FocusMeteringMode, "2", N_("Multi-spot")},

	/* returned by function call */
	{PTP_DPC_FNumber, (char*) ptp_prop_getdescscale100, NULL},
	{PTP_DPC_FocalLength, (char*) ptp_prop_getdescscale100, NULL},
	{PTP_DPC_ExposureTime, (char*) ptp_prop_getdescscale10000, NULL},
	{PTP_DPC_ExposureBiasCompensation,(char*) ptp_prop_getdescscale1000,NULL},
	{0, NULL, NULL}
};

static struct {
	uint16_t dpc;
	char *val;
	const char *txt;
} ptp_property_meaning_NIKON[] = {
	{PTP_DPC_CompressionSetting, "0", N_("JPEG Basic")},
	{PTP_DPC_CompressionSetting, "1", N_("JPEG Normal")},
	{PTP_DPC_CompressionSetting, "2", N_("JPEG Fine")},
	{PTP_DPC_CompressionSetting, "4", N_("NEF RAW")},
	{PTP_DPC_CompressionSetting, "5", N_("NEF+JPEG Basic")},
	{PTP_DPC_WhiteBalance, "2", N_("Automatic")},
	{PTP_DPC_WhiteBalance, "4", N_("Direct Sunlight")},
	{PTP_DPC_WhiteBalance, "5", N_("Fluorescent")},
	{PTP_DPC_WhiteBalance, "6", N_("Incadescent")},
	{PTP_DPC_WhiteBalance, "7", N_("Flash")},
	{PTP_DPC_WhiteBalance, "32784", N_("Cloudy")},
	{PTP_DPC_WhiteBalance, "32785", N_("Shade")},
	{PTP_DPC_WhiteBalance, "32786", N_("Color Temperature")},
	{PTP_DPC_WhiteBalance, "32787", N_("Preset")},
	{PTP_DPC_FocusMode, "32784", N_("AF-S (single-servo)")},
	{PTP_DPC_FocusMode, "32785", N_("AF-C (continuous-servo)")},
	{PTP_DPC_FlashMode, "4", N_("Red-eye reduction")},
	{PTP_DPC_FlashMode, "32784", N_("Front-courtain")},
	{PTP_DPC_FlashMode, "32785", N_("Slow Sync")},
	{PTP_DPC_FlashMode, "32786", N_("(Slow) Rear-curtain")},
	{PTP_DPC_FlashMode, "32787", N_("Slow Sync with Red-eye")},
	{PTP_DPC_ExposureProgramMode, "32784", N_("Camera Auto")},
	{PTP_DPC_ExposureProgramMode, "32785", N_("Portrait")},
	{PTP_DPC_ExposureProgramMode, "32786", N_("Landscape")},
	{PTP_DPC_ExposureProgramMode, "32787", N_("Close Up")},
	{PTP_DPC_ExposureProgramMode, "32788", N_("Sports")},
	{PTP_DPC_ExposureProgramMode, "32789", N_("Night Portrait")},
	{PTP_DPC_ExposureProgramMode, "32790", N_("Night Landscape")},
	{PTP_DPC_StillCaptureMode, "1", N_("Single Frame")},
	{PTP_DPC_StillCaptureMode, "2", N_("Continuous")},
	{PTP_DPC_StillCaptureMode, "32784", N_("Continuous Low Speed")},
	{PTP_DPC_StillCaptureMode, "32785", N_("Self-timer")},
	{PTP_DPC_StillCaptureMode, "32787", N_("Remote")},
	{PTP_DPC_StillCaptureMode, "32786", N_("Mirror Up")},
	{PTP_DPC_StillCaptureMode, "32788", N_("Delayed Remote")},
	{PTP_DPC_FocusMeteringMode, "2", N_("Dynamic Area")},
	{PTP_DPC_FocusMeteringMode, "32784", N_("Single Area")},
	{PTP_DPC_FocusMeteringMode, "32785", N_("Closest Subject")},
	{PTP_DPC_FocusMeteringMode, "32786", N_("Group Dynamic")},
	{PTP_DPC_NIKON_ImageSharpening, "0", N_("Auto")},
	{PTP_DPC_NIKON_ImageSharpening, "1", N_("Normal")},
	{PTP_DPC_NIKON_ImageSharpening, "2", N_("-2 Low")},
	{PTP_DPC_NIKON_ImageSharpening, "3", N_("-1 Medium Low")},
	{PTP_DPC_NIKON_ImageSharpening, "4", N_("+1 Medium High")},
	{PTP_DPC_NIKON_ImageSharpening, "5", N_("+2 High")},
	{PTP_DPC_NIKON_ImageSharpening, "6", N_("None")},
	{PTP_DPC_NIKON_ToneCompensation, "0", N_("Auto")},
	{PTP_DPC_NIKON_ToneCompensation, "1", N_("Normal")},
	{PTP_DPC_NIKON_ToneCompensation, "2", N_("-2 Low Contrast")},
	{PTP_DPC_NIKON_ToneCompensation, "3", N_("-1 Medium Low")},
	{PTP_DPC_NIKON_ToneCompensation, "4", N_("+1 Medium High")},
	{PTP_DPC_NIKON_ToneCompensation, "5", N_("+2 High Contrast")},
	{PTP_DPC_NIKON_ToneCompensation, "6", N_("Custom")},
	{PTP_DPC_NIKON_ColorMode, "0", N_("Ia (sRGB)")},
	{PTP_DPC_NIKON_ColorMode, "1", N_("II (Adobe RGB)")},
	{PTP_DPC_NIKON_ColorMode, "2", N_("IIIa (sRGB)")},
	{PTP_DPC_NIKON_Saturation, "0", N_("Normal")},
	{PTP_DPC_NIKON_Saturation, "1", N_("Moderate")},
	{PTP_DPC_NIKON_Saturation, "2", N_("Enhanced")},
	{PTP_DPC_NIKON_OptimizeImage, "0", N_("Normal")},
	{PTP_DPC_NIKON_OptimizeImage, "1", N_("Vivid")},
	{PTP_DPC_NIKON_OptimizeImage, "2", N_("Sharp")},
	{PTP_DPC_NIKON_OptimizeImage, "3", N_("Soft")},
	{PTP_DPC_NIKON_OptimizeImage, "4", N_("Direct Print")},
	{PTP_DPC_NIKON_OptimizeImage, "5", N_("Portrait")},
	{PTP_DPC_NIKON_OptimizeImage, "6", N_("Landscape")},
	{PTP_DPC_NIKON_OptimizeImage, "7", N_("Custom")},
	{PTP_DPC_NIKON_FocusAreaWrap, "1", N_("Wrap")},
	{PTP_DPC_NIKON_FocusAreaWrap, "0", N_("No Wrap")},
	{PTP_DPC_NIKON_ISOAuto, "0", N_("Off")},
	{PTP_DPC_NIKON_ISOAuto, "1", N_("On")},
	{PTP_DPC_NIKON_ExposureCompensation, "0", N_("Off")},
	{PTP_DPC_NIKON_ExposureCompensation, "1", N_("On")},
	{PTP_DPC_NIKON_CenterWeightArea, "0", N_("6mm")},
	{PTP_DPC_NIKON_CenterWeightArea, "1", N_("8mm")},
	{PTP_DPC_NIKON_CenterWeightArea, "2", N_("10mm")},
	{PTP_DPC_NIKON_CenterWeightArea, "3", N_("12mm")},
	{PTP_DPC_NIKON_AELockMode, "0", N_("AE-L Button")},
	{PTP_DPC_NIKON_AELockMode, "1", N_("Release Button")},
	{PTP_DPC_NIKON_AELAFLMode, "0", N_("Exposure and Focus Lock")},
	{PTP_DPC_NIKON_AELAFLMode, "1", N_("Exposure Lock Only")},
	{PTP_DPC_NIKON_AELAFLMode, "2", N_("Focus Lock Only")},
	{PTP_DPC_NIKON_AELAFLMode, "3", N_("Exposure Lock Hold")},
	{PTP_DPC_NIKON_AELAFLMode, "4", N_("AF-ON")},
	{PTP_DPC_NIKON_AELAFLMode, "5", N_("FV Lock")},
	{PTP_DPC_NIKON_MeterOff, "0", N_("4s")},
	{PTP_DPC_NIKON_MeterOff, "1", N_("6s")},
	{PTP_DPC_NIKON_MeterOff, "2", N_("8s")},
	{PTP_DPC_NIKON_MeterOff, "3", N_("16s")},
	{PTP_DPC_NIKON_MeterOff, "4", N_("30min")},
	{PTP_DPC_NIKON_SelfTimer, "0", N_("2s")},
	{PTP_DPC_NIKON_SelfTimer, "1", N_("5s")},
	{PTP_DPC_NIKON_SelfTimer, "2", N_("10s")},
	{PTP_DPC_NIKON_SelfTimer, "3", N_("20s")},
	{PTP_DPC_NIKON_MonitorOff, "0", N_("10s")},
	{PTP_DPC_NIKON_MonitorOff, "1", N_("20s")},
	{PTP_DPC_NIKON_MonitorOff, "2", N_("1m")},
	{PTP_DPC_NIKON_MonitorOff, "3", N_("5m")},
	{PTP_DPC_NIKON_MonitorOff, "4", N_("10m")},
	{PTP_DPC_NIKON_LongExposureNoiseReduction, "0", N_("Off")},
	{PTP_DPC_NIKON_LongExposureNoiseReduction, "1", N_("On")},
	{PTP_DPC_NIKON_FileNumberSequence, "0", N_("Off")},
	{PTP_DPC_NIKON_FileNumberSequence, "1", N_("On")},
	{PTP_DPC_NIKON_FileNumberSequence, "2", N_("Reset!")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "0", N_("1/60")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "1", N_("1/30")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "2", N_("1/15")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "3", N_("1/8")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "4", N_("1/4")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "5", N_("1/2")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "6", N_("1s")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "7", N_("2s")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "8", N_("4s")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "9", N_("8s")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "10", N_("15s")},
	{PTP_DPC_NIKON_FlashShutterSpeed, "11", N_("30s")},
	{PTP_DPC_NIKON_BracketSet, "0", N_("AE & Flash")},
	{PTP_DPC_NIKON_BracketSet, "1", N_("AE Only")},
	{PTP_DPC_NIKON_BracketSet, "2", N_("Flash Only")},
	{PTP_DPC_NIKON_BracketSet, "3", N_("White Balance")},
	{PTP_DPC_NIKON_BracketOrder, "0", N_("MTR->Under->Over")},
	{PTP_DPC_NIKON_BracketOrder, "1", N_("Under->MTR->Over")},
	{PTP_DPC_NIKON_ReverseCommandDial, "0", N_("No")},
	{PTP_DPC_NIKON_ReverseCommandDial, "1", N_("Yes")},
	{PTP_DPC_NIKON_NoCFCard, "1", N_("Release Locked")},
	{PTP_DPC_NIKON_NoCFCard, "0", N_("Enabled Release")},
	{PTP_DPC_NIKON_ImageCommentAttach, "1", N_("On")},
	{PTP_DPC_NIKON_ImageCommentAttach, "0", N_("Off")},
	{PTP_DPC_NIKON_ImageRotation, "0", N_("Automatic")},
	{PTP_DPC_NIKON_ImageRotation, "1", N_("Off")},
	{PTP_DPC_NIKON_BracketingProgram, "0", N_("-2F")},
	{PTP_DPC_NIKON_BracketingProgram, "1", N_("+2F")},
	{PTP_DPC_NIKON_BracketingProgram, "2", N_("3F")},
	{PTP_DPC_NIKON_Bracketing, "0", N_("Off")},
	{PTP_DPC_NIKON_Bracketing, "1", N_("On")},
	{PTP_DPC_NIKON_AutofocusArea, "0", N_("<CENTER>")},
	{PTP_DPC_NIKON_AutofocusArea, "1", N_("<UP>")},
	{PTP_DPC_NIKON_AutofocusArea, "2", N_("<DOWN>")},
	{PTP_DPC_NIKON_AutofocusArea, "3", N_("<LEFT>")},
	{PTP_DPC_NIKON_AutofocusArea, "4", N_("<RIGHT>")},
	{PTP_DPC_NIKON_CameraOrientation, "0", N_("Landscape")},
	{PTP_DPC_NIKON_CameraOrientation, "1", N_("Left Hand")},
	{PTP_DPC_NIKON_CameraOrientation, "2", N_("Right Hand")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-18", N_("-3")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-16", N_("-2.7")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-15", N_("-2.5")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-14", N_("-2.3")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-12", N_("-2.0")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-10", N_("-1.7")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-9", N_("-1.5")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-8", N_("-1.3")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-6", N_("-1.0")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-4", N_("-0.7")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-3", N_("-0.5")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "-2", N_("-0.3")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "0", N_("0.0")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "2", N_("+0.3")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "3", N_("+0.5")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "4", N_("+0.7")},
	{PTP_DPC_NIKON_FlashExposureCompensation, "6", N_("+1.0")},
	{PTP_DPC_NIKON_BeepOff, "1", N_("Off")},
	{PTP_DPC_NIKON_BeepOff, "0", N_("On")},
	{PTP_DPC_NIKON_AutofocusMode, "0", N_("AF-S (single-servo)")},
	{PTP_DPC_NIKON_AutofocusMode, "1", N_("AF-C (continuous-servo)")},
	{PTP_DPC_NIKON_AFAssist, "0", N_("On")},
	{PTP_DPC_NIKON_AFAssist, "1", N_("Off")},
	{PTP_DPC_NIKON_PADVPMode, "0", N_("1/125")},
	{PTP_DPC_NIKON_PADVPMode, "1", N_("1/60")},
	{PTP_DPC_NIKON_PADVPMode, "2", N_("1/30")},
	{PTP_DPC_NIKON_PADVPMode, "3", N_("1/15")},
	{PTP_DPC_NIKON_PADVPMode, "4", N_("1/8")},
	{PTP_DPC_NIKON_PADVPMode, "5", N_("1/4")},
	{PTP_DPC_NIKON_PADVPMode, "6", N_("1/2")},
	{PTP_DPC_NIKON_PADVPMode, "7", N_("1")},
	{PTP_DPC_NIKON_PADVPMode, "8", N_("2")},
	{PTP_DPC_NIKON_PADVPMode, "9", N_("4")},
	{PTP_DPC_NIKON_PADVPMode, "10", N_("8")},
	{PTP_DPC_NIKON_PADVPMode, "11", N_("15")},
	{PTP_DPC_NIKON_PADVPMode, "12", N_("30")},
	{PTP_DPC_NIKON_ImageReview, "0", N_("On")},
	{PTP_DPC_NIKON_ImageReview, "1", N_("Off")},
	{PTP_DPC_NIKON_AFAreaIllumination, "0", N_("Auto")},
	{PTP_DPC_NIKON_AFAreaIllumination, "1", N_("Off")},
	{PTP_DPC_NIKON_AFAreaIllumination, "2", N_("On")},
	{PTP_DPC_NIKON_FlashMode, "0", N_("TTL")},
	{PTP_DPC_NIKON_FlashMode, "1", N_("Manual")},
	{PTP_DPC_NIKON_FlashMode, "2", N_("Commander Mode")},
	{PTP_DPC_NIKON_FlashCommanderMode, "0", N_("TTL")},
	{PTP_DPC_NIKON_FlashCommanderMode, "1", N_("AA")},
	{PTP_DPC_NIKON_FlashCommanderMode, "2", N_("M")},
	{PTP_DPC_NIKON_FlashSign, "0", N_("On")},
	{PTP_DPC_NIKON_FlashSign, "1", N_("Off")},
	{PTP_DPC_NIKON_RemoteTimeout, "0", N_("1min")},
	{PTP_DPC_NIKON_RemoteTimeout, "1", N_("5min")},
	{PTP_DPC_NIKON_RemoteTimeout, "2", N_("10min")},
	{PTP_DPC_NIKON_RemoteTimeout, "3", N_("15min")},
	{PTP_DPC_NIKON_GridDisplay, "0", N_("Off")},
	{PTP_DPC_NIKON_GridDisplay, "1", N_("On")},
	{PTP_DPC_NIKON_FlashModeManualPower, "0", N_("Full power")},
	{PTP_DPC_NIKON_FlashModeManualPower, "1", N_("1/2 power")},
	{PTP_DPC_NIKON_FlashModeManualPower, "2", N_("1/4 power")},
	{PTP_DPC_NIKON_FlashModeManualPower, "3", N_("1/8 power")},
	{PTP_DPC_NIKON_FlashModeManualPower, "4", N_("1/16 power")},
	{PTP_DPC_NIKON_FlashModeCommanderPower, "0", N_("FULL")},
	{PTP_DPC_NIKON_FlashModeCommanderPower, "1", N_("1/2")},
	{PTP_DPC_NIKON_FlashModeCommanderPower, "2", N_("1/4")},
	{PTP_DPC_NIKON_FlashModeCommanderPower, "3", N_("1/8")},
	{PTP_DPC_NIKON_FlashModeCommanderPower, "4", N_("1/16")},
	{PTP_DPC_NIKON_FlashModeCommanderPower, "5", N_("1/32")},
	{PTP_DPC_NIKON_FlashModeCommanderPower, "6", N_("1/64")},
	{PTP_DPC_NIKON_FlashModeCommanderPower, "7", N_("1/128")},
	{PTP_DPC_NIKON_CSMMenu, "0", N_("Simple")},
	{PTP_DPC_NIKON_CSMMenu, "1", N_("Detailed")},
	{PTP_DPC_NIKON_CSMMenu, "2", N_("Yet Unknown/Hidden?")},
	{PTP_DPC_NIKON_BracketingIncrement, "12", N_("2.0")},
	{PTP_DPC_NIKON_BracketingIncrement, "10", N_("1.7")},
	{PTP_DPC_NIKON_BracketingIncrement, "9", N_("1.5")},
	{PTP_DPC_NIKON_BracketingIncrement, "8", N_("1.3")},
	{PTP_DPC_NIKON_BracketingIncrement, "6", N_("1.0")},
	{PTP_DPC_NIKON_BracketingIncrement, "4", N_("0.7")},
	{PTP_DPC_NIKON_BracketingIncrement, "3", N_("0.5")},
	{PTP_DPC_NIKON_BracketingIncrement, "2", N_("0.3")},
	{PTP_DPC_NIKON_BracketingIncrement, "0", N_("0.0")},
	{PTP_DPC_NIKON_EVStep, "0", N_("1/3")},
	{PTP_DPC_NIKON_EVStep, "1", N_("1/2")},
	{PTP_DPC_NIKON_LowLight, "0", N_("No")},
	{PTP_DPC_NIKON_LowLight, "1", N_("Yes")},
	{PTP_DPC_NIKON_FlashOpen, "0", N_("No")},
	{PTP_DPC_NIKON_FlashOpen, "1", N_("Yes")},
	{PTP_DPC_NIKON_FlashCharged, "0", N_("No")},
	{PTP_DPC_NIKON_FlashCharged, "1", N_("Yes")},
	{PTP_DPC_NIKON_AutoExposureLock, "1", N_("Locked")},
	{PTP_DPC_NIKON_AutoExposureLock, "0", N_("Not Locked")},

	{PTP_DPC_ExposureTime, "4294967295", N_("bulb")},
	{PTP_DPC_NIKON_ExposureTime, "4294967295", N_("bulb")},

	/* returned by function call */
	{PTP_DPC_NIKON_FocalLengthMin, (char*) ptp_prop_getdescscale100, NULL},
	{PTP_DPC_NIKON_FocalLengthMax, (char*) ptp_prop_getdescscale100, NULL},
	{PTP_DPC_NIKON_MaxApAtMinFocalLength,(char*) ptp_prop_getdescscale100, NULL},
	{PTP_DPC_NIKON_MaxApAtMaxFocalLength,(char*) ptp_prop_getdescscale100, NULL},
	{PTP_DPC_NIKON_ExposureTime,(char*) ptp_prop_NIKON_d100, NULL},
	{0, NULL, NULL}
};


/* return property value description */
#define RETPROPDESC(desc)	\
	{\
		for (i=0; desc[i].dpc!=0; i++)	{ \
			if (desc[i].txt!=NULL) { \
				if (desc[i].dpc==dpd->DevicePropertyCode && \
					!strcmp(desc[i].val,strval))\
					return (desc[i].txt);\
			} \
			else {\
				if (desc[i].dpc==dpd->DevicePropertyCode) \
					return (((const char* (*) ()) desc[i].val) (params, dpd, strval)); \
			}\
		}	\
	}

/**
 * ptp_prop_getdescbystring:
 * params:	PTPParams*
 * 		PTPDevicePropDesc *dpd	- Device Property structure
 *		void *value		- if not null convert this value
 *					  (used internaty to convert
 *					   values other than current)
 *
 * Returns:	pointer to staticaly allocated buffer with property value
 *		meaning as string
 *
 **/
const char*
ptp_prop_getdescbystring(PTPParams* params,PTPDevicePropDesc *dpd,const char *strval)
{
	int i;

	switch (params->deviceinfo.VendorExtensionID) {
		case PTP_VENDOR_NIKON:
			RETPROPDESC(ptp_property_meaning_NIKON);
			break;
	}

	RETPROPDESC(ptp_property_meaning);

	return NULL;
}

/**
 * ptp_prop_getdesc:
 * params:	PTPParams*
 * 		PTPDevicePropDesc *dpd	- Device Property structure
 *		void *value		- if not null convert this value
 *					  (used internaty to convert
 *					   values other than current)
 *
 * Returns:	pointer to staticaly allocated buffer with property value
 *		meaning as string
 *
 **/
const char*
ptp_prop_getdesc(PTPParams* params, PTPDevicePropDesc *dpd, void *val)
{
	const char *strval;
	/* Get Device Property value as string */
	strval=ptp_prop_tostr(params, dpd, val);
	
	return ptp_prop_getdescbystring(params, dpd, strval);
}

/**
 * ptp_prop_tostr:
 * params:	PTPParams*
 * 		PTPDevicePropDesc *dpd	- Device Property structure
 *		void *value		- if not null convert this value
 *					  (used internaty to convert
 *					   values other than current)
 *
 * Returns:	pointer to staticaly allocated buffer with property value
 *		representation as string
 *
 **/

const char *
ptp_prop_tostr (PTPParams* params, PTPDevicePropDesc *dpd, void *val)
{
	static char strval[SVALLEN];
	int n;
	void *value=val==NULL?dpd->CurrentValue:val;

	memset(&strval, 0, SVALLEN);

	switch (dpd->DataType) {
		case PTP_DTC_INT8:
			n=snprintf(strval,SVALLEN,"%hhi",*(char*)value);
			SVALRET(strval);
		case PTP_DTC_UINT8:
			n=snprintf(strval,SVALLEN,"%hhu",*(unsigned char*)value);
			SVALRET(strval);
		case PTP_DTC_INT16:
			n=snprintf(strval,SVALLEN,"%hi",*(int16_t*)value);
			SVALRET(strval);
		case PTP_DTC_UINT16:
			n=snprintf(strval,SVALLEN,"%hu",*(uint16_t*)value);
			SVALRET(strval);
		case PTP_DTC_INT32:
			n=snprintf(strval,SVALLEN,"%li",(long int)*(int32_t*)value);
			SVALRET(strval);
		case PTP_DTC_UINT32:
			n=snprintf(strval,SVALLEN,"%lu",(unsigned long)*(uint32_t*)value);
			SVALRET(strval);
		case PTP_DTC_STR:
			n=snprintf(strval,SVALLEN,"\"%s\"",(char *)value);
			SVALRET(strval);
	}
	return NULL;
}

const char*
ptp_prop_getvalbyname(PTPParams* params, char* name, uint16_t dpc)
{
	int i;
	/* doeasn't match for function interpretation */
	for (i=0; ptp_property_meaning[i].txt!=NULL; i++)
		if (ptp_property_meaning[i].dpc==dpc)
			if (! strncasecmp(ptp_property_meaning[i].txt,name,
				strlen(name)))
				return ptp_property_meaning[i].val;

	/* XXX */
	for (i=0; ptp_property_meaning_NIKON[i].txt!=NULL; i++)
		if (ptp_property_meaning_NIKON[i].dpc==dpc)
			if (!strncasecmp(ptp_property_meaning_NIKON[i].txt,name,
				strlen(name)))
				return ptp_property_meaning_NIKON[i].val;

	return NULL;
}

