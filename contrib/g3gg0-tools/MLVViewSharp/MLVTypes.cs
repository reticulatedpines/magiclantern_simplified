using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using uint8_t = System.Byte;
using uint16_t = System.UInt16;
using uint32_t = System.UInt32;
using uint64_t = System.UInt64;
using int8_t = System.Byte;
using int16_t = System.Int16;
using int32_t = System.Int32;
using int64_t = System.Int64;
using System.Runtime.InteropServices;

namespace mlv_view_sharp
{
    public static class MLVTypes
    {
        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct raw_info_crop
        {
            public int x, y;           // DNG JPEG top left corner
            public int width, height;  // DNG JPEG size
        }

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct raw_info_active_area
        {
            public int y1, x1, y2, x2;
        }

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct raw_info
        {
            public int api_version;            // increase this when changing the structure
            public uint32_t buffer;               // points to image data

            public int height, width, pitch;
            public int frame_size;
            public int bits_per_pixel;         // 14

            public int black_level;            // autodetected
            public int white_level;            // somewhere around 13000 - 16000, varies with camera, settings etc

            public raw_info_crop jpeg;
            public raw_info_active_area active_area;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
            public int[] exposure_bias;       // DNG Exposure Bias (idk what's that)
            public int cfa_pattern;            // stick to 0x02010100 (RGBG) if you can
            public int calibration_illuminant1;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 18)]
            public int[] color_matrix1;      // DNG Color Matrix

            public int dynamic_range;          // EV x100, from analyzing black level and noise (very close to DxO)
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_file_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string fileMagic;    /* Magic Lantern Video file header */
            public uint32_t blockSize;    /* size of the whole header */
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 8)]
            public string versionString;    /* null-terminated C-string of the exact revision of this format */
            public uint64_t fileGuid;    /* UID of the file (group) generated using hw counter, time of day and PRNG */
            public uint16_t fileNum;    /* the ID within fileCount this file has (0 to fileCount-1) */
            public uint16_t fileCount;    /* how many files belong to this group (splitting or parallel) */
            public uint32_t fileFlags;    /* 1=out-of-order data, 2=dropped frames, 4=single image mode, 8=stopped due to error */
            public uint16_t videoClass;    /* 0=none, 1=RAW, 2=YUV, 3=JPEG, 4=H.264 */
            public uint16_t audioClass;    /* 0=none, 1=WAV */
            public uint32_t videoFrameCount;    /* number of video frames in this file. set to 0 on start, updated when finished. */
            public uint32_t audioFrameCount;    /* number of audio frames in this file. set to 0 on start, updated when finished. */
            public uint32_t sourceFpsNom;    /* configured fps in 1/s multiplied by sourceFpsDenom */
            public uint32_t sourceFpsDenom;    /* denominator for fps. usually set to 1000, but may be 1001 for NTSC */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_vidf_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* this block contains one frame of video data */
            public uint32_t blockSize;    /* total frame size */
            public uint64_t timestamp;    /* hardware counter timestamp for this frame (relative to recording start) */
            public uint32_t frameNumber;    /* unique video frame number */
            public uint16_t cropPosX;    /* specifies from which sensor row/col the video frame was copied (8x2 blocks) */
            public uint16_t cropPosY;    /* (can be used to process dead/hot pixels) */
            public uint16_t panPosX;    /* specifies the panning offset which is cropPos, but with higher resolution (1x1 blocks) */
            public uint16_t panPosY;    /* (it's the frame area from sensor the user wants to see) */
            public uint32_t frameSpace;    /* size of dummy data before frameData starts, necessary for EDMAC alignment */
            /* uint8_t     frameData[variable] */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_audf_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* this block contains audio data */
            public uint32_t blockSize;    /* total frame size */
            public uint64_t timestamp;    /* hardware counter timestamp for this frame (relative to recording start) */
            public uint32_t frameNumber;    /* unique audio frame number */
            public uint32_t frameSpace;    /* size of dummy data before frameData starts, necessary for EDMAC alignment */
            /* uint8_t     frameData[variable] */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_rawi_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* when videoClass is RAW, this block will contain detailed format information */
            public uint32_t blockSize;    /* total frame size */
            public uint64_t timestamp;    /* hardware counter timestamp for this frame (relative to recording start) */
            public uint16_t xRes;    /* Configured video resolution, may differ from payload resolution */
            public uint16_t yRes;    /* Configured video resolution, may differ from payload resolution */
            public raw_info raw_info;    /* the raw_info structure delivered by raw.c of ML Core */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_wavi_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* when audioClass is WAV, this block contains format details  compatible to RIFF */
            public uint32_t blockSize;    /* total frame size */
            public uint64_t timestamp;    /* hardware counter timestamp for this frame (relative to recording start) */
            public uint16_t format;    /* 1=Integer PCM, 6=alaw, 7=mulaw */
            public uint16_t channels;    /* audio channel count: 1=mono, 2=stereo */
            public uint32_t samplingRate;    /* audio sampling rate in 1/s */
            public uint32_t bytesPerSecond;    /* audio data rate */
            public uint16_t blockAlign;    /* see RIFF WAV hdr description */
            public uint16_t bitsPerSample;    /* audio ADC resolution */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_expo_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;
            public uint32_t blockSize;    /* total frame size */
            public uint64_t timestamp;    /* hardware counter timestamp for this frame (relative to recording start) */
            public uint32_t isoMode;    /* 0=manual, 1=auto */
            public uint32_t isoValue;    /* camera delivered ISO value */
            public uint32_t isoAnalog;    /* ISO obtained by hardware amplification (most full-stop ISOs, except extreme values) */
            public uint32_t digitalGain;    /* digital ISO gain (1024 = 1 EV) - it's not baked in the raw data, so you may want to scale it or adjust the white level */
            public uint64_t shutterValue;    /* exposure time in microseconds */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_lens_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;
            public uint32_t blockSize;    /* total frame size */
            public uint64_t timestamp;    /* hardware counter timestamp for this frame (relative to recording start) */
            public uint16_t focalLength;    /* in mm */
            public uint16_t focalDist;    /* in mm (65535 = infinite) */
            public uint16_t aperture;    /* f-number * 100 */
            public uint8_t stabilizerMode;    /* 0=off, 1=on, (is the new L mode relevant) */
            public uint8_t autofocusMode;    /* 0=off, 1=on */
            public uint32_t flags;    /* 1=CA avail, 2=Vign avail, ... */
            public uint32_t lensID;    /* hexadecimal lens ID (delivered by properties?) */
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string lensName;    /* full lens string */
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string lensSerial; /* full lens serial number */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_rtci_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;
            public uint32_t blockSize;    /* total frame size */
            public uint64_t timestamp;    /* hardware counter timestamp for this frame (relative to recording start) */
            public uint16_t tm_sec;    /* seconds (0-59) */
            public uint16_t tm_min;    /* minute (0-59) */
            public uint16_t tm_hour;    /* hour (0-24) */
            public uint16_t tm_mday;    /* day of month (1-31) */
            public uint16_t tm_mon;    /* month (1-12) */
            public uint16_t tm_year;    /* year */
            public uint16_t tm_wday;    /* day of week */
            public uint16_t tm_yday;    /* day of year */
            public uint16_t tm_isdst;    /* daylight saving */
            public uint16_t tm_gmtoff;    /* GMT offset */
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 8)]
            public string tm_zone;    /* time zone string */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_idnt_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;
            public uint32_t blockSize;    /* total frame size */
            public uint64_t timestamp;    /* hardware counter timestamp for this frame (relative to recording start) */
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string cameraName;    /* PROP (0x00000002), offset 0, length 32 */
            public uint32_t cameraModel;    /* PROP (0x00000002), offset 32, length 4 */
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string cameraSerial;    /* Camera serial number (if available) */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_xref_t
        {
            public uint16_t fileNumber;    /* the logical file number as specified in header */
            public uint16_t empty;    /* for future use. set to zero. */
            public uint64_t frameOffset;    /* the file offset at which the frame is stored (VIDF/AUDF) */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_xref_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* can be added in post processing when out of order data is present */
            public uint32_t blockSize;    /* this can also be placed in a separate file with only file header plus this block */
            public uint64_t timestamp;
            public uint32_t frameType;    /* bitmask: 1=video, 2=audio */
            public uint32_t entryCount;    /* number of xrefs that follow here */
            public mlv_xref_t xrefEntries;    /* this structure refers to the n'th video/audio frame offset in the files */
            /* uint8_t     xrefData[variable] */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_info_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* user definable info string. take number, location, etc. */
            public uint32_t blockSize;
            public uint64_t timestamp;
            /* uint8_t     stringData[variable] */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_diso_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* Dual-ISO information */
            public uint32_t blockSize;
            public uint64_t timestamp;
            public uint32_t dualMode;    /* bitmask: 0=off, 1=odd lines, 2=even lines, upper bits may be defined later */
            public uint32_t isoValue;
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_mark_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* markers set by user while recording */
            public uint32_t blockSize;
            public uint64_t timestamp;
            public uint32_t type;    /* value may depend on the button being pressed or counts up (t.b.d) */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_styl_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;
            public uint32_t blockSize;
            public uint64_t timestamp;
            public uint32_t picStyleId;
            public int32_t contrast;
            public int32_t sharpness;
            public int32_t saturation;
            public int32_t colortone;
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string picStyleName;
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_elvl_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* Electronic level (orientation) data */
            public uint32_t blockSize;
            public uint64_t timestamp;
            public uint32_t roll;    /* degrees x100 (here, 45.00 degrees) */
            public uint32_t pitch;    /* 10.00 degrees */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_wbal_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* White balance info */
            public uint32_t blockSize;
            public uint64_t timestamp;
            public uint32_t wb_mode;    /* WB_AUTO 0, WB_SUNNY 1, WB_SHADE 8, WB_CLOUDY 2, WB_TUNGSTEN 3, WB_FLUORESCENT 4, WB_FLASH 5, WB_CUSTOM 6, WB_KELVIN 9 */
            public uint32_t kelvin;    /* only when wb_mode is WB_KELVIN */
            public uint32_t wbgain_r;    /* only when wb_mode is WB_CUSTOM */
            public uint32_t wbgain_g;    /* 1024 = 1.0 */
            public uint32_t wbgain_b;    /* note: it's 1/canon_gain (uses dcraw convention) */
            public uint32_t wbs_gm;    /* WBShift (no idea how to use these in post) */
            public uint32_t wbs_ba;    /* range: -9...9 */
        };

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public unsafe struct mlv_null_hdr_t
        {
            [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 4)]
            public string blockType;    /* empty block */
            public uint32_t blockSize;    /* total frame size */
        };


        public static T ReadStruct<T>(this byte[] buffer)
        where T : struct
        {
            GCHandle handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            T result = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
            handle.Free();
            return result;
        }

        internal static object ToStruct(string type, byte[] buf)
        {
            switch (type)
            {
                case "MLVI":
                    return ReadStruct<mlv_file_hdr_t>(buf);
                case "VIDF":
                    return ReadStruct<mlv_vidf_hdr_t>(buf);
                case "AUDF":
                    return ReadStruct<mlv_audf_hdr_t>(buf);
                case "RAWI":
                    return ReadStruct<mlv_rawi_hdr_t>(buf);
                case "WAVI":
                    return ReadStruct<mlv_wavi_hdr_t>(buf);
                case "EXPO":
                    return ReadStruct<mlv_expo_hdr_t>(buf);
                case "LENS":
                    return ReadStruct<mlv_lens_hdr_t>(buf);
                case "RTCI":
                    return ReadStruct<mlv_rtci_hdr_t>(buf);
                case "IDNT":
                    return ReadStruct<mlv_idnt_hdr_t>(buf);
                case "XREF":
                    return ReadStruct<mlv_xref_hdr_t>(buf);
                case "INFO":
                    return ReadStruct<mlv_info_hdr_t>(buf);
                case "DISO":
                    return ReadStruct<mlv_diso_hdr_t>(buf);
                case "MARK":
                    return ReadStruct<mlv_mark_hdr_t>(buf);
                case "STYL":
                    return ReadStruct<mlv_styl_hdr_t>(buf);
                case "ELVL":
                    return ReadStruct<mlv_elvl_hdr_t>(buf);
                case "WBAL":
                    return ReadStruct<mlv_wbal_hdr_t>(buf);
                case "NULL":
                    return ReadStruct<mlv_null_hdr_t>(buf);
            }

            return null;
        }
    }
}
