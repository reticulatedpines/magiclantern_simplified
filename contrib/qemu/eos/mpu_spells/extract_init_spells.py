# Parse mpu_send/mpu_recv logs (from dm-spy-experiments branch)
# and generate MPU init spells code for QEMU.
#
# Very rough proof of concept, far from complete. Tested on 60D.

from __future__ import print_function
import os, sys, re
from outils import *

# first two chars are message size, next two chars identify the property
# the following spells are indexed by spell[6:11], e.g. "06 05 04 00 ..." => "04 00"
# take them with a grain of salt - some might be wrong
known_spells = {
    "00 00"  :   (0xFFFFFFFF, "Complete WaitID"),
    "01 00"  :   (0x80000000, "PROP_SHOOTING_MODE",         (4, "ARG0")),
    "01 01"  :   (0x80000001, "PROP_SHOOTING_MODE_CUSTOM"),
    "01 02"  :   (0x80000002, "PROP_METERING_MODE"),
    "01 03"  :   (0x80000003, "PROP_DRIVE_MODE"),
    "01 04"  :   (0x80000004, "PROP_AF_MODE"),
    "01 05"  :   (0x80000005, "PROP_SHUTTER"),
    "01 06"  :   (0x80000006, "PROP_APERTURE"),
    "01 07"  :   (0x80000007, "PROP_ISO"),
    "01 08"  :   (0x80000008, "PROP_AE"),
    "01 09"  :   (0x80000009, "PROP_FEC"),                              # PROP_STROBO_AECOMP
    "01 0a"  :   (0x8000000A, "PROP_AFPOINT"),
    "01 0b"  :   (0x8000000B, "PROP_AEB"),
    "01 0c"  :   (0x8000000C, "PROP 8000000C"),
    "01 0d"  :   (0x8000000D, "PROP_WB_MODE_PH"),                       # PROP_WB_MODE
    "01 0e"  :   (0x8000000E, "PROP_WB_KELVIN_PH"),                     # PROP_COLOR_TEMP
    "01 0f"  :   (0x8000000F, "PROP 8000000F"),
    "01 10"  :   (0x80000010, "PROP_WBS_GM"),
    "01 11"  :   (0x80000011, "PROP_WBS_BA"),
    "01 12"  :   (0x80000012, "PROP_WBB_GM"),
    "01 13"  :   (0x80000013, "PROP_WBB_BA"),
    "01 1d"  :   (0x80000028, "PROP_PICTURE_STYLE"),                    # PROP_FLAVOR_MODE
    "01 1e"  :   (0x80000026, "PROP 80000026"),
    "01 1f"  :   (0x80000024, "PROP_AUTO_POWEROFF_TIME"),
    "01 20"  :   (0x8000001D, "PROP_CARD1_EXISTS"),                     # PROP_CARD1_EXIST
    "01 21"  :   (0x8000001E, "PROP_CARD2_EXISTS"),                     # PROP_CARD2_EXIST
    "01 22"  :   (0x8000001F, "PROP_CARD3_EXISTS"),                     # PROP_CARD3_EXIST
    "01 23"  :   (0x80000020, "PROP_CARD1_STATUS",          (5, "ARG0")),
    "01 24"  :   (0x80000021, "PROP_CARD2_STATUS",          (5, "ARG0")),
    "01 25"  :   (0x80000022, "PROP_CARD3_STATUS",          (5, "ARG0")),
    "01 26"  :   (0x02010000, "PROP_CARD1_FOLDER_NUMBER",   (5, "ARG0")),   # PROP_FOLDER_NUMBER_A (to MPU only?)
    "01 27"  :   (0x02010001, "PROP_CARD2_FOLDER_NUMBER",   (5, "ARG0")),   # PROP_FOLDER_NUMBER_B (to MPU only?)
    "01 28"  :   (0x02010002, "PROP_CARD3_FOLDER_NUMBER",   (5, "ARG0")),   # PROP_FOLDER_NUMBER_C (to MPU only?)
    "01 29"  :   (0x02010003, "PROP_CARD1_FILE_NUMBER",     (4, "ARG0"), (5, "ARG1"), (7, "ARG2")), # PROP_FILE_NUMBER_A (to MPU only?)
    "01 2a"  :   (0x02010004, "PROP_CARD2_FILE_NUMBER",     (4, "ARG0"), (5, "ARG1"), (7, "ARG2")), # PROP_FILE_NUMBER_B (to MPU only?)
    "01 2b"  :   (0x02010005, "PROP_CARD3_FILE_NUMBER",     (4, "ARG0"), (5, "ARG1"), (7, "ARG2")), # PROP_FILE_NUMBER_C (to MPU only?)
    "01 2c"  :   (0x80040002, "PROP_CURRENT_MEDIA"),                    # PROP_CARD_SELECT
    "01 2d"  :   (0x80040003, "PROP 80040003"),
    "01 2e"  :   (0x8000002A, "PROP_SAVE_MODE"),
    "01 30"  :   (0x80000023, "PROP_BEEP"),
    "01 31"  :   (0x80000027, "PROP_NO_CARD_RELEASE"),                  # PROP_RELEASE_WITHOUT_CARD
    "01 32"  :   (0x80000025, "PROP_RED_EYE_REDUCTION"),                # PROP_STROBO_REDEYE
    "01 33"  :   (0x80000029, "PROP 80000029"),                         # card related
    "01 34"  :   (0x8000002F, "PROP_CARD1_IMAGE_QUALITY"),              # PROP_PIC_QUALITY
    "01 35"  :   (0x80000030, "PROP_CARD2_IMAGE_QUALITY"),              # PROP_PIC_QUALITY2
    "01 36"  :   (0x80000031, "PROP_CARD3_IMAGE_QUALITY"),              # PROP_PIC_QUALITY3
    "01 37"  :   (0x80040001, "PROP_CARD_EXTENSION"),                   # 5D3 card options
    "01 38"  :   (0x80040005, "PROP 80040005"),
    "01 39"  :   (0x80040006, "PROP 80040006"),
    "01 3a"  :   (0x80040007, "PROP 80040007"),
    "01 3b"  :   (0x8004000A, "PROP_USBDEVICE_CONNECT"),                # to MPU only?
    "01 3c"  :   (0x8000002B, "PROP 8000002B"),
    "01 3d"  :   (0x8004000F, "PROP_TEMP_STATUS"),
    "01 3e"  :   (0x80040011, "PROP_ELECTRIC_SHUTTER_MODE"),            # PROP_ELECTRIC_SHUTTER - silent shooting
    "01 3f"  :   (0x80040013, "PROP_FLASH_ENABLE"),                     # PROP_STROBO_FIRING
    "01 40"  :   (0x80040014, "PROP_STROBO_ETTLMETER"),
    "01 41"  :   (0x80040015, "PROP_STROBO_CURTAIN"),
    "01 42"  :   (0x80040016, "PROP_PHOTO_STUDIO_MODE"),
    "01 43"  :   (0x80040017, "PROP 80040017"),
    "01 44"  :   (0x80040018, "PROP 80040018"),
    "01 45"  :   (0x8004001A, "PROP_METERING_TIMER_FOR_LV"),
    "01 46"  :   (0x8004001B, "PROP_PHOTO_STUDIO_ENABLE_ISOCOMP"),
    "01 47"  :   (0x80000032, "PROP_SELFTIMER_CONTINUOUS_NUM"),
    "01 48"  :   (0x8004001C, "PROP_LIVE_VIEW_MOVIE_SELECT"),           # also enable/disable LV; PROP_LV_MOVIE_SELECT
    "01 49"  :   (0x8004001D, "PROP_LIVE_VIEW_AF_SYSTEM"),              # PROP_LVAF_MODE
    "01 4a"  :   (0x80000033, "PROP_PROGRAM_SHIFT"),
    "01 4b"  :   (0x80000034, "PROP_LIVE_VIEW_VIEWTYPE_SELECT"),        # ExpSim; PROP_LIVE_VIEW_VIEWTYPE
    "01 4c"  :   (0x80000037, "PROP_ONESHOT_RAW"),                      # 7D
    "01 4d"  :   (0x80000038, "PROP_VIEWFINDER_GRID") ,                 # 70D, 5D3; PROP_FINDER_GRID_SELECT
    "01 4e"  :   (0x80000039, "PROP_VIDEO_MODE"),                       # PROP_MOVIE_PARAM
    "01 4f"  :   (0x80030037, "PROP_FIXED_MOVIE"),
    "01 50"  :   (0x8000003A, "PROP_AE_MODE_MOVIE"),
    "01 51"  :   (0x8000003B, "PROP_AUTO_ISO_RANGE"),
    "01 52"  :   (0x8000003D, "PROP_ALO"),                              # PROP_AUTO_LIGHTING_OPTIMIZER_MODE
    "01 53"  :   (0x8000003C, "PROP_AF_DURING_RECORD"),                 # PROP_MOVIE_REC_AF
    "01 54"  :   (0x8000003E, "PROP_SUBDIAL_LOCK_MODE"),
    "01 55"  :   (0x8000003F, "PROP_MULTIPLE_EXPOSURE_SETTING"),
    "01 57"  :   (0x80040021, "PROP_BUILTIN_STROBO_MODE"),
    "01 58"  :   (0x80000041, "PROP_VIDEOSNAP_MODE"),
    "01 59"  :   (0x80000042, "PROP_MOVIE_SERVO_AF"),                   # PROP_CONTINUOUS_AF_MODE
    "01 5a"  :   (0x80000043, "PROP_CONTINUOUS_AF_VALID"),              # to MPU only?
    "01 5b"  :   (0x80000044, "PROP_REGISTRATION_DATA_UPDATE_FUNC"),
    "01 5c"  :   (0x80040022, "PROP_AF_USM_LENS_ELECTRONIC_MF"),
    "01 5d"  :   (0x80040023, "PROP_AF_AISERVO_1FRAME_ACT_PRIORITY"),
    "01 5e"  :   (0x80040024, "PROP_AF_AISERVO_CONTINUOUS_ACT_PRIORITY"),
    "01 5f"  :   (0x80040025, "PROP_AF_ONESHOT_ACT_PRIORITY"),
    "01 60"  :   (0x80040026, "PROP_AF_LENSDRIVE_WHEN_AFIMPOSSIBLE"),
    "01 61"  :   (0x80040027, "PROP 80040027"),
    "01 62"  :   (0x80040028, "PROP 80040028"),
    "01 63"  :   (0x80040029, "PROP 80040029"),
    "01 64"  :   (0x8004002A, "PROP 8004002A"),
    "01 65"  :   (0x8004002B, "PROP 8004002B"),
    "01 66"  :   (0x8004002C, "PROP 8004002C"),
    "01 68"  :   (0x8004002D, "PROP 8004002D"),
    "01 69"  :   (0x8004002E, "PROP_AF_VF_DISPLAY_ILLUMINATION"),
    "01 6a"  :   (0x8004002F, "PROP 8004002F"),
    "01 6b"  :   (0x80040030, "PROP 80040030"),
    "01 6c"  :   (0x80040031, "PROP 80040031"),
    "01 6d"  :   (0x80040032, "PROP 80040032"),
    "01 6e"  :   (0x80040033, "PROP_ISO_RANGE"),
    "01 6f"  :   (0x80040034, "PROP_LIMITED_TV_VALUE_AT_AUTOISO"),      # 6D, 70D
    "01 70"  :   (0x80000045, "PROP_HDR_SETTING"),
    "01 71"  :   (0x80000046, "PROP_AF_METHOD_SELECT_FOCUS_AREA"),
    "01 72"  :   (0x80000047, "PROP_MLU"),                              # PROP_MIRRORUP_SETTING
    "01 73"  :   (0x80000048, "PROP_LONGEXPO_NOISE_REDUCTION"),         # PROP_LONG_EXPOSURE_NOISE_REDUCTION
    "01 74"  :   (0x80000049, "PROP_HIGHISO_NOISE_REDUCTION"),          # PROP_HI_ISO_SETTING_NOISE_REDUCTION
    "01 75"  :   (0x8000004A, "PROP_HTP"),                              # PROP_HILIGHT_TONE_PRIORITY
    "01 76"  :   (0x8000004B, "PROP_SILENT_CONTROL_SETTING"),
    "01 77"  :   (0x8000004C, "PROP 8000004C"),
    "01 78"  :   (0x8000004D, "PROP 8000004D"),
    "01 79"  :   (0x80040035, "PROP_AF_CURRENT_AISERVO_STYLE"),
    "01 7a"  :   (0x80030057, "PROP_GPS_SATELITE_STATUS"),
    "01 7b"  :   (0x80040040, "PROP_CONTINUOUS_AF"),                    # 70D
    "01 7c"  :   (0x80040041, "PROP_GPS_AUTO_TIME_SETTING"),
    "01 7d"  :   (0x80040042, "PROP_GPS_PINPOINTING_INTERVAL_SETTING"),
    "01 7e"  :   (0x80040043, "PROP_GPS_COMPAS_SELECT"),
    "01 7f"  :   (0x80040044, "PROP_GPS_CALIBRATION"),
    "01 80"  :   (0x80040045, "PROP_GPS_TIME_SYNC"),
    "01 81"  :   (0x80040046, "PROP 80040046"),
    "01 82"  :   (0x80040047, "PROP_GPS_BATTERY"),
    "01 83"  :   (0x80040048, "PROP 80040048"),
    "01 84"  :   (0x80040049, "PROP 80040049"),
    "01 85"  :   (0x8000004e, "PROP_GIS_SETTING"),
    "01 86"  :   (0x8004004A, "PROP 8004004A"),
    "01 87"  :   (0x8004004B, "PROP 8004004B"),
    "01 88"  :   (0x8000004F, "PROP 8000004F"),
    "01 89"  :   (0x8004004C, "PROP_GPS_DEVICE_ACTIVE"),                # PROP_GPS
    "01 8a"  :   (0x8004004D, "PROP_SW2_MOVIE_START"),
    "01 8b"  :   (0x8004004E, "PROP 8004004E"),
    "01 8c"  :   (0x8004004F, "PROP_LEVEL_INDICATOR_FINDER_SETTING"),   # 70D
    "01 8d"  :   (0x80040050, "PROP 80040050"),
    "01 8e"  :   (0x80040051, "PROP_GPSLOG_SETTING"),
    "01 8f"  :   (0x80040053, "PROP_LV_CFILTER"),
    "01 90"  :   (0x80040052, "PROP_WIFI_SETTING"),
    "01 91"  :   (0x80040054, "PROP_BUILTINGPS_PINPOINTING_INTERVAL_SETTING"), # PROP_BUILTINGPS_INTERVAL
    "01 92"  :   (0x80040059, "PROP_IMAGE_ASPECT_RATIO"),
    "01 93"  :   (0x80040056, "PROP_TIMECODE_HDMI_OUTPUT"),             # 5D3 1.2.3
    "01 94"  :   (0x80040057, "PROP_TIMECODE_HDMI_REC_COMMAND"),        # 5D3 1.2.3
    "01 95"  :   (0x8004005C, "PROP 8004005C"),
    "01 96"  :   (0x8004005A, "PROP 8004005A"),
    "01 97"  :   (0x8003006F, "PROP 8003006F"),
    "01 98"  :   (0x80040056, "PROP 80040056"),
    "01 99"  :   (0x80040057, "PROP 80040057"),
    "01 9a"  :   (0x80040058, "PROP 80040058"),
    "01 9b"  :   (0x80030074, "PROP 80030074"),
    "01 9c"  :   (0x8004005B, "PROP_NETWORK_SYSTEM"),                   # 70D
    "01 9d"  :   (0x8004005D, "PROP 8004005D"),
    "01 9e"  :   (0x8004005E, "PROP 8004005E"),
    "01 9f"  :   (0x8004005F, "PROP 8004005F"),
    "01 a0"  :   (0x80040060, "PROP 80040060"),
    "01 a1"  :   (0x80040061, "PROP 80040061"),
    "02 00"  :   (0xCCCCCCCC, "Init group"),    # 80000000,80000001,80000002,80000003,80000004,80000008,80000007,80000026,8000000b,8000000d,8000000e,80000012,80000013,80000010,80000011,80000028,8000002f,80000030,80000031,80000027,80000023,80000025,80000024,80000009,80000005,80000006
    "02 04"  :   (0x80010004, "PROP_CFN"),
    "02 05"  :   (0x80010004, "PROP_CFN_1"),
    "02 06"  :   (0x80010005, "PROP_CFN_2"),
    "02 07"  :   (0x80010006, "PROP_CFN_3"),
    "02 08"  :   (0x80010007, "PROP_CFN_4"),
    "02 0a"  :   (0x80010000, "PROP_PERMIT_ICU_EVENT"),                 # to MPU only?
    "02 0b"  :   (0x80010001, "PROP_TERMINATE_SHUT_REQ"),
    "02 0c"  :   (0x80010002, "PROP 80010002"),
    "02 0d"  :   (0xCCCCCCCC, "Card group"),    # 80000029,8000001D,8000001E,8000001F,80000020,80000021,80000022,80040001,8000002A,80040002,8004000F,80040016,80040017,8004001B,80040018,8003000B,8003000C,8003000D,8003001E,80030042
    "02 0e"  :   (0xCCCCCCCC, "Mode group"),    # 80000000,80000001,80000002,80000003,80000004,80000008,80000007,80000026,8000000B,8000000D,8000000E,80000012,80000013,80000010,80000011,80000028,8000002F,80000030,80000031,80000027,80000023,80000025,80000024
    "02 0f"  :   (0xCCCCCCCC, "Movie group"),   # 8004001D,80040011,8004001A,8004001C,80000034,80000039,80000041,8000004D,8000003A,8000003C,80000042
    "02 10"  :   (0xCCCCCCCC, "AF group"),      # 80040022,80040023,80040024,80040025,80040026,80040028,80040029,8004002A,8004002B,8004002C,80040027,80040035,80040036,8004002D,8004002E,8004002F,80040030,80040031
    "02 11"  :   (0xCCCCCCCC, "AF2 group"),     # 8000000A,8003004F,80000004,80000006,80040027,80030034
    "02 12"  :   (0xCCCCCCCC, "Lens group"),    # 80030011,8003003C,80030028,8003002A,80030029,80030004
    "02 24"  :   (0x80030021, "PROP_LENS_NAME"),
    "03 00"  :   (0x80030000, "PROP 80030000"),
    "03 02"  :   (0x80030001, "PROP 80030001"),
    "03 04"  :   (0x80030003, "PROP_POWER_KIND"),
    "03 05"  :   (0x80030004, "PROP_POWER_LEVEL"),                      # PROP_BATTERY_POWER
    "03 06"  :   (0x80030005, "PROP_AVAIL_SHOT"),                       # to MPU only?
    "03 07"  :   (0x80030006, "PROP_BURST_COUNT"),                      # to MPU only?
    "03 0b"  :   (0x80030007, "PROP 80030007"),                         # to MPU only?
    "03 0c"  :   (0x8003000B, "PROP_CARD1_RECORD"),                     # PROP_CARD_RECORD_A
    "03 0d"  :   (0x8003000C, "PROP_CARD2_RECORD"),                     # PROP_CARD_RECORD_B
    "03 0e"  :   (0x8003000D, "PROP_CARD3_RECORD"),                     # PROP_CARD_RECORD_C
    "03 10"  :   (0x80030008, "PROP 80030008"),                         # to MPU only?
    "03 11"  :   (0x80030009, "PROP_ICU_AUTO_POWEROFF"),                # also 80030024?!
    "03 13"  :   (0x8003000E, "PROP_LOGICAL_CONNECT"),                  # to MPU only?
    "03 14"  :   (0x80030010, "PROP 80030010"),
    "03 15"  :   (0x80030011, "PROP_LENS"),
    "03 16"  :   (0x80030013, "PROP_BATTERY_CHECK"),
    "03 17"  :   (0x80030014, "PROP_EFIC_TEMP"),
    "03 18"  :   (0x8003000F, "PROP 8003000F"),                         # to MPU only?
    "03 19"  :   (0x80030015, "PROP_TFT_STATUS"),                       # to MPU only?
    "03 1b"  :   (0x8003001C, "PROP 8003001C"),                         # swapped
    "03 1c"  :   (0x8003001B, "PROP 8003001B"),
    "03 1d"  :   (0x8003001D, "PROP_BATTERY_REPORT"),                   # PROP_BAT_INFO
    "03 1e"  :   (0x8003001A, "PROP 8003001A"),                         # to MPU only?
    "03 1f"  :   (0x80030019, "PROP 80030019"),                         # to MPU only?
    "03 20"  :   (0x8003001E, "PROP_STARTUP_CONDITION"),
    "03 21"  :   (0x8003001F, "PROP 8003001F"),
    "03 24"  :   (0x80030021, "PROP_LENS_NAME"),                        # PROP_CURRENT_LENS_NAME
    "03 27"  :   (0x80030022, "PROP 80030022"),
    "03 28"  :   (0x80030022, "PROP 80030022"),
    "03 29"  :   (0x80030023, "PROP 80030023"),
    "03 2b"  :   (0x80030026, "PROP 80030026"),
    "03 2c"  :   (0x80030027, "PROP 80030027"),
    "03 2e"  :   (0x80030029, "PROP_SHUTTER_COUNTER"),
    "03 2f"  :   (0x80030028, "PROP_SPECIAL_OPTION"),
    "03 30"  :   (0x8003002A, "PROP 8003002A"),
    "03 31"  :   (0x8003002D, "PROP 8003002D"),
    "03 32"  :   (0x8003003A, "PROP 8003003A"),
    "03 33"  :   (0x8003003B, "PROP 8003003B"),
    "03 34"  :   (0x0205000D, "PROP_Q_POSITION"),                       # to MPU only?
    "03 35"  :   (0xFFFFFFFF, "PROP_BATTERY_REPORT_COUNTER"),
    "03 36"  :   (0xFFFFFFFF, "PROP_BATTERY_REPORT_FINISHED"),
    "03 37"  :   (0x80030034, "PROP_MIRROR_DOWN_IN_MOVIE_MODE"),
    "03 38"  :   (0x80030035, "PROP 80030035"),
    "03 39"  :   (0x80030038, "PROP_STROBO_SETTING_COMPOSITION"),       # PROP_STROBO_SETTING
    "03 3a"  :   (0x80030039, "PROP_ROLLING_PITCHING_LEVEL"),
    "03 3c"  :   (0x8003003C, "PROP 8003003C"),
    "03 3d"  :   (0x8003003D, "PROP_AFSHIFT_LVASSIST_STATUS"),
    "03 3e"  :   (0x8003003E, "PROP_AFSHIFT_LVASSIST_SHIFT_RESULT"),
    "03 3f"  :   (0x8003003F, "PROP 8003003F"),
    "03 40"  :   (0x80030040, "PROP 80030040"),                         # to MPU only?
    "03 41"  :   (0x80030041, "PROP 80030041"),
    "03 42"  :   (0x80030042, "PROP_LED_LIGHT"),
    "03 43"  :   (0x80030044, "PROP_STROBO_SETTING_EXP_COMPOSITION"),
    "03 44"  :   (0x80030045, "PROP 80030045"),
    "03 45"  :   (0x80030046, "PROP 80030046"),
    "03 46"  :   (0x80030047, "PROP 80030047"),
    "03 4d"  :   (0x8003004E, "PROP 8003004E"),
    "03 4e"  :   (0x8003004F, "PROP_AFFRAME_ENABLE_SETTING"),
    "03 4f"  :   (0x80030050, "PROP 80030050"),
    "03 50"  :   (0x80030052, "PROP 80030052"),
    "03 51"  :   (0x80030053, "PROP 80030053"),
    "03 53"  :   (0x80030058, "PROP 80030058"),
    "03 54"  :   (0x80030059, "PROP_MPU_GPS"),
    "03 55"  :   (0x8003005A, "PROP 8003005A"),
    "03 56"  :   (0x8003005B, "PROP 8003005B"),
    "03 57"  :   (0x8003005E, "PROP 8003005E"),
    "03 5a"  :   (0x80030060, "PROP 80030060"),
    "03 5b"  :   (0x8003005D, "PROP 8003005D"),
    "03 5c"  :   (0x80030061, "PROP 80030061"),
    "03 5d"  :   (0x80030062, "PROP 80030062"),
    "03 64"  :   (0x80030065, "PROP 80030065"),
    "03 65"  :   (0x80030066, "PROP_GPSLOG_RESULT"),
    "03 66"  :   (0x80030067, "PROP 80030067"),
    "03 6c"  :   (0x80030073, "PROP 80030073"),
    "03 6d"  :   (0x80030075, "PROP 80030075"),
    "03 6e"  :   (0x80030076, "PROP 80030076"),
    "03 6f"  :   (0x80030077, "PROP 80030077"),
    "04 00"  :   (0x80020000, "NotifyGUIEvent"),                        # PROP_GUI_STATE
    "04 01"  :   (0x80020009, "PROP_ICU_UILOCK",            (4, "ARG0")),
    "04 06"  :   (0x80020006, "PROP_DEFAULT_LV_MANIP"),
    "04 07"  :   (0x80010003, "PROP_REBOOT"),
    "04 0c"  :   (0x80040004, "PROP_SHOOTING_TYPE"),
    "04 0d"  :   (0x8002000C, "PROP_ACTIVE_SWEEP_STATUS"),
    "04 0e"  :   (0x8002000D, "PROP 8002000D"),
    "04 10"  :   (0x8002000E, "PROP_REMOTE_BULB_RELEASE_START"),
    "04 11"  :   (0x8002000F, "PROP_REMOTE_BULB_RELEASE_END"),
    "04 13"  :   (0x80020012, "PROP 80020012"),
    "04 15"  :   (0x80020013, "PROP_DL_ACTION"),
    "04 16"  :   (0x80020015, "PROP_REMOTE_SW1"),
    "04 17"  :   (0x80020016, "PROP_REMOTE_SW2"),
    "04 18"  :   (0x80020017, "PROP_REMOTE_AFSTART_BUTTON"),
    "04 19"  :   (0x80020018, "PROP_REMOTE_SET_BUTTON"),
    "04 1a"  :   (0x80020019, "PROP_POPUP_BUILTIN_FLASH"),
    "04 1b"  :   (0x8002001A, "PROP 8002001A"),
    "05 00"  :   (0xFFFFFFFF, "EVENTID_METERING_START_SW1ON"),
    "05 01"  :   (0xFFFFFFFF, "EVENTID_RELEASE_ON"),
    "05 02"  :   (0xFFFFFFFF, "EVENTID_RELEASE_DATA_SW2ON"),
    "05 03"  :   (0xFFFFFFFF, "EVENTID_AFTER_RELEASE_DATA"),
    "05 04"  :   (0xFFFFFFFF, "EVENTID_RELEASE_OFF_SW2OFF"),
    "05 05"  :   (0xFFFFFFFF, "EVENTID_BULB_END"),
    "05 06"  :   (0xFFFFFFFF, "EVENTID_RELEASE_CANCEL"),
    "05 07"  :   (0xFFFFFFFF, "EVENTID_ACCUMULATION_STOP"),
    "05 09"  :   (0xFFFFFFFF, "EVENTID_GERO"),
    "05 0b"  :   (0xFFFFFFFF, "EVENTID_METERING_TIMER_START_SW1OFF"),
    "05 0e"  :   (0xFFFFFFFF, "EVENTID_RELEASE_START"),
    "05 0f"  :   (0xFFFFFFFF, "EVENTID_RELEASE_END"),
    "05 11"  :   (0xFFFFFFFF, "EVENTID_AEICData"),
    "05 12"  :   (0xFFFFFFFF, "EVENTID_AFPintData"),
    "05 13"  :   (0xFFFFFFFF, "EVENTID_ImageParamData"),
    "08 06"  :   (0xFFFFFFFF, "COM_FA_CHECK_FROM"),
    "09 00"  :   (0x80050000, "PROP_LV_LENS"),
    "09 01"  :   (0x80050013, "PROP_LV_LENS_DRIVE_REMOTE"),             # to MPU only?
    "09 02"  :   (0x80050002, "PROP_LV_FOCUS_DONE"),
    "09 05"  :   (0x80050005, "PROP_LV_LENS_STABILIZE"),
    "09 0a"  :   (0x80050008, "PROP_LV_BV"),
    "09 0b"  :   (0x80050029, "PROP_LV_AF_RESULT"),                     # PROP_LV_FOCUS_BAD
    "09 0c"  :   (0x8005000A, "PROP_LV_HALF_SHUTTER"),                  # PROP_HALF_SHUTTER
    "09 0d"  :   (0x8005000B, "PROP_LV_DOF_PREVIEW"),                   # PROP_DOF_PREVIEW_MAYBE
    "09 0e"  :   (0x8005000C, "PROP_STROBO_CHARGE_INFO_MAYBE"),
    "09 0f"  :   (0x8005000D, "PROP_ORIENTATION"),
    "09 10"  :   (0x80050010, "PROP_BV"),
    "09 11"  :   (0x80050015, "PROP_LV_DISPSIZE"),                      # to MPU only?
    "09 12"  :   (0x8005001B, "PROP_LVCAF_STATE"),
    "09 14"  :   (0x8005001E, "PROP 8005001E"),
    "09 15"  :   (0x80050020, "PROP 80050020"),                         # to MPU only?
    "09 17"  :   (0x80050026, "PROP_LV_FOCUS_DATA"),                    # to MPU only?
    "09 18"  :   (0x80050027, "PROP_LV_FOCUS_CMD"),
    "09 19"  :   (0x80050028, "PROP 80050028"),
    "09 1a"  :   (0x8005002A, "PROP 8005002A"),                         # to MPU only?
    "09 1b"  :   (0x8005002B, "PROP 8005002B"),
    "09 1f"  :   (0x80050034, "PROP 80050034"),                         # to MPU only?
    "0a 08"  :   (0xFFFFFFFF, "PD_NotifyOlcInfoChanged"),
}

# our MPU messages from logs are printed in lower case
for a in known_spells.keys():
    assert a == a.lower()

# without arguments, export known spells as C header
if len(sys.argv) == 1:
    print("Exporting known spells...", file=sys.stderr)
    print("""
/* autogenerated with extract_init_spells.py */

struct known_spell {
    uint8_t class;
    uint8_t id;
    uint32_t property;
    const char * description;
};

const struct known_spell known_spells[] = {""")
    for spell, data in sorted(known_spells.iteritems()):
        prop = data[0]
        desc = data[1]
        spell = ", ".join(["0x%s" % x for x in spell.split(" ")])
        print('    { %s, 0x%08X, "%s" },' % (spell, prop, desc))
    print("};")
    print("Done.", file=sys.stderr)
    raise SystemExit


processed_spells = {}

first_mpu_send_only = False

def replace_spell_arg(spell, pos, newarg):
    bytes = spell.split(" ")
    bytes[pos] = newarg
    return " ".join(bytes)

def format_spell(spell):
    bytes = spell.split(" ")
    bytes = [("0x" + b if len(b) == 2 else b) for b in bytes]
    return "{ " + ", ".join(bytes) + " }"

log_fullpath = sys.argv[1]
f = open(log_fullpath, "r")
lines = f.readlines()

# logs start with camera model, e.g. 60D-startup.log
[log_path, log_filename] = os.path.split(log_fullpath)
model = log_filename[:log_filename.index("-")]

print("static struct mpu_init_spell mpu_init_spells_%s[] = {" % model)
first_block = True
num = 0
num2 = 0

first_send = True
waitid_prop = None
commented_block = False

switch_names = get_switch_names(model)
bind_switches = {}
last_bind_switch = None

for l in lines:
    # match bindReceiveSwitch with GUI_Control, both from MainCtrl task
    # note: GUI_Control messages can be sent from other tasks
    m = re.match(".* MainCtrl:.*bindReceiveSwitch *\(([^()]*)\)", l)
    if not m:
        # VxWorks (450D)
        m = re.match(".* tMainCtrl:.*\[BIND\] Switch *\(([^()]*)\)", l)
    if m:
        args = m.groups()[0].split(",")
        args = tuple([int(a) for a in args])
        # old models have some extra bindReceiveSwitch lines with a single argument; ignore them
        if len(args) == 2:
            last_bind_switch = args
        continue
    m = re.match(".* MainCtrl:.*GUI_Control:([0-9]+) +0x([0-9])+", l)
    if not m:
        m = re.match(".* tMainCtrl:.*\[BIND\] bindReceiveSwitch \(([0-9]+)\)", l)
    if m:
        if last_bind_switch is not None:
            arg1 = int(m.groups()[0])
            arg2 = int(m.groups()[1],16) if len(m.groups()) == 2 else 0
            if last_bind_switch in bind_switches:
                assert(bind_switches[last_bind_switch] == (arg1,arg2))
            else:
                bind_switches[last_bind_switch] = arg1,arg2
            last_bind_switch = None
        else:
            print("GUI_Control without bindReceiveSwitch", file=sys.stderr)
            print(l, file=sys.stderr)

prev_hwcount = 0
overflows = 0
timestamp = 0
last_mpu_timestamp = 0

for l in lines:
    if len(l) > 5 and l[5] == ">":
        hwcount = int(l[:5], 16)
        if hwcount < prev_hwcount:
            overflows += 1
        prev_hwcount = hwcount
        timestamp = overflows * 0x100000 + hwcount

    m = re.match(".* mpu_send\(([^()]*)\)", l)
    if m:
        last_mpu_timestamp = timestamp
        spell = m.groups()[0].strip()
        parm_spell = spell

        if first_send or not first_mpu_send_only:
            first_send = False
            
            # spell counters
            num += 1
            num2 = 0
            
            if first_block:
                first_block = False
            elif commented_block:
                print("     // { 0 } } },");
                commented_block = False
                num -= 1
            else:
                print("        { 0 } } },")

            description = ""

            if spell[6:11] in known_spells:
                metadata = known_spells[spell[6:11]]
                description = metadata[1]

                # parameterized spell?
                if len(metadata) > 2:
                    for pos,newarg in metadata[2:]:
                        parm_spell = replace_spell_arg(parm_spell, pos, newarg)

            if spell.startswith("06 04 02 00 "):
                description = "Init"

            if spell.startswith("08 06 00 00 "):
                description = "Complete WaitID ="
                if waitid_prop:
                    description += " " + waitid_prop
                if spell[12:17] == "02 00":
                    description += " " + "Init"
                elif spell[12:17] in known_spells:
                    description += " " + known_spells[spell[12:17]][1]

            # comment out NotifyGuiEvent / PROP_GUI_STATE and its associated Complete WaitID
            if description == "NotifyGUIEvent" or description == "Complete WaitID = 0x80020000 NotifyGUIEvent":
                commented_block = True

            # comment out PROP_ICU_UILOCK - we have it in UILock.h
            if description == "PROP_ICU_UILOCK":
                commented_block = True

            # comment out PROP_BATTERY_CHECK
            if description == "PROP_BATTERY_CHECK":
                commented_block = True

            # include PROP_BATTERY_REPORT only once
            if description == "PROP_BATTERY_REPORT":
                if spell in processed_spells:
                    commented_block = True

            if commented_block:
                # commented blocks are not numbered, to match the numbers used at runtime
                if description:
                    print(" // { %-58s" % (format_spell(parm_spell) + ", .description = \"" + description + "\", .out_spells = { "))
                else:
                    print(" // { %-58s" % (format_spell(parm_spell) + ", {"))
            else:
                if description:
                    print("    { %-58s/* spell #%d */" % (format_spell(parm_spell) + ", .description = \"" + description + "\", .out_spells = { ", num))
                else:
                    print("    { %-58s/* spell #%d */" % (format_spell(parm_spell) + ", {", num))

            processed_spells[spell] = True

            continue

    m = re.match(".* mpu_recv\(([^()]*)\)", l)
    if m:
        reply = m.groups()[0].strip()
        num2 += 1

        cmt = "  "
        warning = ""
        description = ""

        dt = timestamp - last_mpu_timestamp
        if dt > 100000:
            warning = "delayed by %d ms, likely external input" % (dt/1000)
            cmt = "//"
        last_mpu_timestamp = timestamp

        # comment out entire block?
        if commented_block:
            cmt = "//"

        # comment out button codes
        if reply.startswith("06 05 06 "):
            args = reply.split(" ")[3:5]
            args = tuple([int(a,16) for a in args])
            if args in bind_switches:
                btn_code = bind_switches[args][0]
                if btn_code in switch_names:
                    cmt = "//"
                    warning = ""

        if reply.startswith("06 05 06 "):
            args = reply.split(" ")[3:5]
            args = tuple([int(a,16) for a in args])
            if args in bind_switches:
                btn_code = bind_switches[args][0]
                if btn_code in switch_names:
                    description += ", %s" % switch_names[btn_code]
                description += ", GUI_Control:%d" % btn_code
            description += ", bindReceiveSwitch(%d, %d)" % (args[0], args[1])
            description = description[2:]

        if reply[6:11] in known_spells:
            metadata = known_spells[reply[6:11]]

            # generic description from name and arguments
            if not description:
                description = metadata[1]
                args = []
                for pos,newarg in metadata[2:]:
                    args.append(reply.split(" ")[pos])
                if args:
                    description += "(%s)" % ", ".join(args)

            # replace parameters with ARG0, ARG1 etc where the choice is obvious
            for pos,newarg in metadata[2:]:
                if reply[6:11] == spell[6:11] and \
                   len(reply.split(" ")) == len(spell.split(" ")):          # same length?
                      assert newarg == parm_spell.split(" ")[pos]           # same arg in this position?
                      assert reply.split(" ")[pos] == spell.split(" ")[pos] # same numeric argument?
                      reply = replace_spell_arg(reply, pos, newarg)

        # disable sensor cleaning
        if description == "PROP_ACTIVE_SWEEP_STATUS":
            reply = replace_spell_arg(reply, 4, "00")
            warning = ("disabled, " + warning).strip(" ,")

        # comment out mode switches
        if reply[6:11] in [ "02 00", "02 0e" ] and num > 1:
            cmt = "//"
            warning = "mode switch?"


        # show lens name as comment
        if description == "PROP_LENS_NAME":
            description += ": "
            for ch in reply.split(" ")[4:]:
                ch = int(ch, 16)
                if ch:
                    description += chr(ch)

        print("     %s %-56s/* reply #%d.%d" % (cmt, format_spell(reply) + ",", num, num2), end="")

        if description:
            print(", %s" % description, end="")

        if warning:
            print(", %s" % warning, end="")

        print(" */")
        continue
    
    # after a Complete WaitID line, the ICU sends to the MPU a message saying it's ready
    # so the MPU can then send data for the next property that requires a "Complete WaitID"
    # (if those are not synced, you will get ERROR TWICE ACK REQUEST)
    # example:
    #    PropMgr:ff31ec3c:01:03: Complete WaitID = 0x80020000, 0xFF178514(0)
    #    PropMgr:00c5c318:00:00: *** mpu_send(08 06 00 00 04 00 00), from 616c
    # the countdown at the end of the line must be 0

    m = re.match(".*Complete WaitID = ([0-9A-Fx]+), ([0-9A-Fx]+)\(0\)", l)
    if m:
        waitid_prop = m.groups()[0]

print("     // { 0 } } }," if commented_block else "        { 0 } } },")
print("")
print('    #include "NotifyGUIEvent.h"')
print('    #include "UILock.h"')
print('    #include "CardFormat.h"')
print('    #include "MpuProperties.h"')
print('    #include "GPS.h"')
print('    #include "Shutdown.h"')
print("};")
