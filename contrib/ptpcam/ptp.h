/* ptp.h
 *
 * Copyright (C) 2001-2004 Mariusz Woloszyn <emsi@ipartners.pl>
 *
 *  This file is part of libptp2.
 *
 *  libptp2 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  libptp2 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libptp2; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __PTP_H__
#define __PTP_H__

#include <stdarg.h>
#include <time.h>
#include "libptp-endian.h"
#include "ptp-eos-oc.h"
/* PTP datalayer byteorder */

#define PTP_DL_BE			0xF0
#define	PTP_DL_LE			0x0F

/* PTP request/response/event general PTP container (transport independent) */

struct _PTPContainer {
	uint16_t Code;
	uint32_t SessionID;
	uint32_t Transaction_ID;
	/* params  may be of any type of size less or equal to uint32_t */
	uint32_t Param1;
	uint32_t Param2;
	uint32_t Param3;
	/* events can only have three parameters */
	uint32_t Param4;
	uint32_t Param5;
	/* the number of meaningfull parameters */
	uint8_t	 Nparam;
};
typedef struct _PTPContainer PTPContainer;

/* PTP USB Bulk-Pipe container */
/* USB bulk max max packet length for high speed endpoints */
#define PTP_USB_BULK_HS_MAX_PACKET_LEN	512
#define PTP_USB_BULK_HDR_LEN		(2*sizeof(uint32_t)+2*sizeof(uint16_t))
#define PTP_USB_BULK_PAYLOAD_LEN	(PTP_USB_BULK_HS_MAX_PACKET_LEN-PTP_USB_BULK_HDR_LEN)
#define PTP_USB_BULK_REQ_LEN	(PTP_USB_BULK_HDR_LEN+5*sizeof(uint32_t))

struct _PTPUSBBulkContainer {
	uint32_t length;
	uint16_t type;
	uint16_t code;
	uint32_t trans_id;
	union {
		struct {
			uint32_t param1;
			uint32_t param2;
			uint32_t param3;
			uint32_t param4;
			uint32_t param5;
		} params;
		unsigned char data[PTP_USB_BULK_PAYLOAD_LEN];
	} payload;
};
typedef struct _PTPUSBBulkContainer PTPUSBBulkContainer;

#define PTP_USB_INT_PACKET_LEN	8

/* PTP USB Asynchronous Event Interrupt Data Format */
struct _PTPUSBEventContainer {
	uint32_t length;
	uint16_t type;
	uint16_t code;
	uint32_t trans_id;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3;
};
typedef struct _PTPUSBEventContainer PTPUSBEventContainer;


/* USB container types */

#define PTP_USB_CONTAINER_UNDEFINED		0x0000
#define PTP_USB_CONTAINER_COMMAND		0x0001
#define PTP_USB_CONTAINER_DATA			0x0002
#define PTP_USB_CONTAINER_RESPONSE		0x0003
#define PTP_USB_CONTAINER_EVENT			0x0004

/* Vendor IDs */
#define PTP_VENDOR_EASTMAN_KODAK	0x00000001
#define PTP_VENDOR_SEIKO_EPSON		0x00000002
#define PTP_VENDOR_AGILENT		0x00000003
#define PTP_VENDOR_POLAROID		0x00000004
#define PTP_VENDOR_AGFA_GEVAERT		0x00000005
#define PTP_VENDOR_MICROSOFT		0x00000006
#define PTP_VENDOR_EQUINOX		0x00000007
#define PTP_VENDOR_VIEWQUEST		0x00000008
#define PTP_VENDOR_STMICROELECTRONICS	0x00000009
#define PTP_VENDOR_NIKON		0x0000000A
#define PTP_VENDOR_CANON		0x0000000B

/* Operation Codes */

#define PTP_OC_Undefined                0x1000
#define PTP_OC_GetDeviceInfo            0x1001
#define PTP_OC_OpenSession              0x1002
#define PTP_OC_CloseSession             0x1003
#define PTP_OC_GetStorageIDs            0x1004
#define PTP_OC_GetStorageInfo           0x1005
#define PTP_OC_GetNumObjects            0x1006
#define PTP_OC_GetObjectHandles         0x1007
#define PTP_OC_GetObjectInfo            0x1008
#define PTP_OC_GetObject                0x1009
#define PTP_OC_GetThumb                 0x100A
#define PTP_OC_DeleteObject             0x100B
#define PTP_OC_SendObjectInfo           0x100C
#define PTP_OC_SendObject               0x100D
#define PTP_OC_InitiateCapture          0x100E
#define PTP_OC_FormatStore              0x100F
#define PTP_OC_ResetDevice              0x1010
#define PTP_OC_SelfTest                 0x1011
#define PTP_OC_SetObjectProtection      0x1012
#define PTP_OC_PowerDown                0x1013
#define PTP_OC_GetDevicePropDesc        0x1014
#define PTP_OC_GetDevicePropValue       0x1015
#define PTP_OC_SetDevicePropValue       0x1016
#define PTP_OC_ResetDevicePropValue     0x1017
#define PTP_OC_TerminateOpenCapture     0x1018
#define PTP_OC_MoveObject               0x1019
#define PTP_OC_CopyObject               0x101A
#define PTP_OC_GetPartialObject         0x101B
#define PTP_OC_InitiateOpenCapture      0x101C
/* Eastman Kodak extension Operation Codes */
#define PTP_OC_EK_SendFileObjectInfo	0x9005
#define PTP_OC_EK_SendFileObject	0x9006
/* Canon extension Operation Codes */
#define PTP_OC_CANON_GetObjectSize	0x9001
#define PTP_OC_CANON_StartShootingMode	0x9008
#define PTP_OC_CANON_EndShootingMode	0x9009
#define PTP_OC_CANON_ViewfinderOn	0x900B
#define PTP_OC_CANON_ViewfinderOff	0x900C
#define PTP_OC_CANON_ReflectChanges	0x900D
#define PTP_OC_CANON_CheckEvent		0x9013
#define PTP_OC_CANON_FocusLock		0x9014
#define PTP_OC_CANON_FocusUnlock	0x9015
#define PTP_OC_CANON_InitiateCaptureInMemory	0x901A
#define PTP_OC_CANON_GetPartialObject_CHDK	0x901B
#define PTP_OC_CANON_GetViewfinderImage	0x901d
#define PTP_OC_CANON_GetChanges		0x9020
#define PTP_OC_CANON_GetFolderEntries	0x9021
/* Nikon extensiion Operation Codes */
#define PTP_OC_NIKON_DirectCapture	0x90C0
#define PTP_OC_NIKON_SetControlMode	0x90C2
#define PTP_OC_NIKON_CheckEvent		0x90C7
#define PTP_OC_NIKON_KeepAlive		0x90C8


/* Proprietary vendor extension operations mask */
#define PTP_OC_EXTENSION_MASK		0xF000
#define PTP_OC_EXTENSION		0x9000

/* Response Codes */

#define PTP_RC_Undefined                0x2000
#define PTP_RC_OK                       0x2001
#define PTP_RC_GeneralError             0x2002
#define PTP_RC_SessionNotOpen           0x2003
#define PTP_RC_InvalidTransactionID	0x2004
#define PTP_RC_OperationNotSupported    0x2005
#define PTP_RC_ParameterNotSupported    0x2006
#define PTP_RC_IncompleteTransfer       0x2007
#define PTP_RC_InvalidStorageId         0x2008
#define PTP_RC_InvalidObjectHandle      0x2009
#define PTP_RC_DevicePropNotSupported   0x200A
#define PTP_RC_InvalidObjectFormatCode  0x200B
#define PTP_RC_StoreFull                0x200C
#define PTP_RC_ObjectWriteProtected     0x200D
#define PTP_RC_StoreReadOnly            0x200E
#define PTP_RC_AccessDenied             0x200F
#define PTP_RC_NoThumbnailPresent       0x2010
#define PTP_RC_SelfTestFailed           0x2011
#define PTP_RC_PartialDeletion          0x2012
#define PTP_RC_StoreNotAvailable        0x2013
#define PTP_RC_SpecificationByFormatUnsupported         0x2014
#define PTP_RC_NoValidObjectInfo        0x2015
#define PTP_RC_InvalidCodeFormat        0x2016
#define PTP_RC_UnknownVendorCode        0x2017
#define PTP_RC_CaptureAlreadyTerminated 0x2018
#define PTP_RC_DeviceBusy               0x2019
#define PTP_RC_InvalidParentObject      0x201A
#define PTP_RC_InvalidDevicePropFormat  0x201B
#define PTP_RC_InvalidDevicePropValue   0x201C
#define PTP_RC_InvalidParameter         0x201D
#define PTP_RC_SessionAlreadyOpened     0x201E
#define PTP_RC_TransactionCanceled      0x201F
#define PTP_RC_SpecificationOfDestinationUnsupported            0x2020
/* Eastman Kodak extension Response Codes */
#define PTP_RC_EK_FilenameRequired	0xA001
#define PTP_RC_EK_FilenameConflicts	0xA002
#define PTP_RC_EK_FilenameInvalid	0xA003

/* NIKON extension Response Codes */
#define PTP_RC_NIKON_PropertyReadOnly	0xA005

/* Proprietary vendor extension response code mask */
#define PTP_RC_EXTENSION_MASK		0xF000
#define PTP_RC_EXTENSION		0xA000

/* libptp2 extended ERROR codes */
#define PTP_ERROR_IO			0x02FF
#define PTP_ERROR_DATA_EXPECTED		0x02FE
#define PTP_ERROR_RESP_EXPECTED		0x02FD
#define PTP_ERROR_BADPARAM		0x02FC

/* PTP Event Codes */

#define PTP_EC_Undefined		0x4000
#define PTP_EC_CancelTransaction	0x4001
#define PTP_EC_ObjectAdded		0x4002
#define PTP_EC_ObjectRemoved		0x4003
#define PTP_EC_StoreAdded		0x4004
#define PTP_EC_StoreRemoved		0x4005
#define PTP_EC_DevicePropChanged	0x4006
#define PTP_EC_ObjectInfoChanged	0x4007
#define PTP_EC_DeviceInfoChanged	0x4008
#define PTP_EC_RequestObjectTransfer	0x4009
#define PTP_EC_StoreFull		0x400A
#define PTP_EC_DeviceReset		0x400B
#define PTP_EC_StorageInfoChanged	0x400C
#define PTP_EC_CaptureComplete		0x400D
#define PTP_EC_UnreportedStatus		0x400E
/* Canon extension Event Codes */
#define PTP_EC_CANON_DeviceInfoChanged	0xC008
#define PTP_EC_CANON_RequestObjectTransfer	0xC009
#define PTP_EC_CANON_CameraModeChanged	0xC00C

/* Nikon extension Event Codes */
#define PTP_EC_NIKON_ObjectReady	0xC101
#define PTP_EC_NIKON_CaptureOverflow	0xC102

/* PTP device info structure (returned by GetDevInfo) */

struct _PTPDeviceInfo {
	uint16_t StaqndardVersion;
	uint32_t VendorExtensionID;
	uint16_t VendorExtensionVersion;
	char	*VendorExtensionDesc;
	uint16_t FunctionalMode;
	uint32_t OperationsSupported_len;
	uint16_t *OperationsSupported;
	uint32_t EventsSupported_len;
	uint16_t *EventsSupported;
	uint32_t DevicePropertiesSupported_len;
	uint16_t *DevicePropertiesSupported;
	uint32_t CaptureFormats_len;
	uint16_t *CaptureFormats;
	uint32_t ImageFormats_len;
	uint16_t *ImageFormats;
	char	*Manufacturer;
	char	*Model;
	char	*DeviceVersion;
	char	*SerialNumber;
};
typedef struct _PTPDeviceInfo PTPDeviceInfo;

/* PTP storageIDs structute (returned by GetStorageIDs) */

struct _PTPStorageIDs {
	uint32_t n;
	uint32_t *Storage;
};
typedef struct _PTPStorageIDs PTPStorageIDs;

/* PTP StorageInfo structure (returned by GetStorageInfo) */
struct _PTPStorageInfo {
	uint16_t StorageType;
	uint16_t FilesystemType;
	uint16_t AccessCapability;
	uint64_t MaxCapability;
	uint64_t FreeSpaceInBytes;
	uint32_t FreeSpaceInImages;
	char 	*StorageDescription;
	char	*VolumeLabel;
};
typedef struct _PTPStorageInfo PTPStorageInfo;

/* PTP objecthandles structure (returned by GetObjectHandles) */

struct _PTPObjectHandles {
	uint32_t n;
	uint32_t *Handler;
};
typedef struct _PTPObjectHandles PTPObjectHandles;

#define PTP_HANDLER_SPECIAL	0xffffffff
#define PTP_HANDLER_ROOT	0x00000000


/* PTP objectinfo structure (returned by GetObjectInfo) */

struct _PTPObjectInfo {
	uint32_t StorageID;
	uint16_t ObjectFormat;
	uint16_t ProtectionStatus;
	uint32_t ObjectCompressedSize;
	uint16_t ThumbFormat;
	uint32_t ThumbCompressedSize;
	uint32_t ThumbPixWidth;
	uint32_t ThumbPixHeight;
	uint32_t ImagePixWidth;
	uint32_t ImagePixHeight;
	uint32_t ImageBitDepth;
	uint32_t ParentObject;
	uint16_t AssociationType;
	uint32_t AssociationDesc;
	uint32_t SequenceNumber;
	char 	*Filename;
	time_t	CaptureDate;
	time_t	ModificationDate;
	char	*Keywords;
};
typedef struct _PTPObjectInfo PTPObjectInfo;

/* max ptp string length INCLUDING terminating null character */

#define PTP_MAXSTRLEN				255

/* PTP Object Format Codes */

/* ancillary formats */
#define PTP_OFC_Undefined			0x3000
#define PTP_OFC_Association			0x3001
#define PTP_OFC_Script				0x3002
#define PTP_OFC_Executable			0x3003
#define PTP_OFC_Text				0x3004
#define PTP_OFC_HTML				0x3005
#define PTP_OFC_DPOF				0x3006
#define PTP_OFC_AIFF	 			0x3007
#define PTP_OFC_WAV				0x3008
#define PTP_OFC_MP3				0x3009
#define PTP_OFC_AVI				0x300A
#define PTP_OFC_MPEG				0x300B
#define PTP_OFC_ASF				0x300C
#define PTP_OFC_QT				0x300D /* guessing */
/* image formats */
#define PTP_OFC_EXIF_JPEG			0x3801
#define PTP_OFC_TIFF_EP				0x3802
#define PTP_OFC_FlashPix			0x3803
#define PTP_OFC_BMP				0x3804
#define PTP_OFC_CIFF				0x3805
#define PTP_OFC_Undefined_0x3806		0x3806
#define PTP_OFC_GIF				0x3807
#define PTP_OFC_JFIF				0x3808
#define PTP_OFC_PCD				0x3809
#define PTP_OFC_PICT				0x380A
#define PTP_OFC_PNG				0x380B
#define PTP_OFC_Undefined_0x380C		0x380C
#define PTP_OFC_TIFF				0x380D
#define PTP_OFC_TIFF_IT				0x380E
#define PTP_OFC_JP2				0x380F
#define PTP_OFC_JPX				0x3810
/* Eastman Kodak extension ancillary format */
#define PTP_OFC_EK_M3U				0xb002


/* PTP Association Types */

#define PTP_AT_Undefined			0x0000
#define PTP_AT_GenericFolder			0x0001
#define PTP_AT_Album				0x0002
#define PTP_AT_TimeSequence			0x0003
#define PTP_AT_HorizontalPanoramic		0x0004
#define PTP_AT_VerticalPanoramic		0x0005
#define PTP_AT_2DPanoramic			0x0006
#define PTP_AT_AncillaryData			0x0007

/* PTP Protection Status */

#define PTP_PS_NoProtection			0x0000
#define PTP_PS_ReadOnly				0x0001

/* PTP Storage Types */

#define PTP_ST_Undefined			0x0000
#define PTP_ST_FixedROM				0x0001
#define PTP_ST_RemovableROM			0x0002
#define PTP_ST_FixedRAM				0x0003
#define PTP_ST_RemovableRAM			0x0004

/* PTP FilesystemType Values */

#define PTP_FST_Undefined			0x0000
#define PTP_FST_GenericFlat			0x0001
#define PTP_FST_GenericHierarchical		0x0002
#define PTP_FST_DCF				0x0003

/* PTP StorageInfo AccessCapability Values */

#define PTP_AC_ReadWrite			0x0000
#define PTP_AC_ReadOnly				0x0001
#define PTP_AC_ReadOnly_with_Object_Deletion	0x0002

/* Property Describing Dataset, Range Form */

struct _PTPPropDescRangeForm {
	void *		MinimumValue;
	void *		MaximumValue;
	void *		StepSize;
};
typedef struct _PTPPropDescRangeForm PTPPropDescRangeForm;

/* Property Describing Dataset, Enum Form */

struct _PTPPropDescEnumForm {
	uint16_t	NumberOfValues;
	void **		SupportedValue;
};
typedef struct _PTPPropDescEnumForm PTPPropDescEnumForm;

/* Device Property Describing Dataset (DevicePropDesc) */

struct _PTPDevicePropDesc {
	uint16_t	DevicePropertyCode;
	uint16_t	DataType;
	uint8_t		GetSet;
	void *		FactoryDefaultValue;
	void *		CurrentValue;
	uint8_t		FormFlag;
	union	{
		PTPPropDescEnumForm	Enum;
		PTPPropDescRangeForm	Range;
	} FORM;
};
typedef struct _PTPDevicePropDesc PTPDevicePropDesc;

/* Canon filesystem's folder entry Dataset */

#define PTP_CANON_FilenameBufferLen	13
#define PTP_CANON_FolderEntryLen	sizeof(PTPCANONFolderEntry)

struct _PTPCANONFolderEntry {
	uint32_t	ObjectHandle;
	uint16_t	ObjectFormatCode;
	uint8_t		Flags;
	uint32_t	ObjectSize;
	time_t		Time;
    char     Filename[PTP_CANON_FilenameBufferLen];
};
typedef struct _PTPCANONFolderEntry PTPCANONFolderEntry;

/* DataType Codes */

#define PTP_DTC_UNDEF		0x0000
#define PTP_DTC_INT8		0x0001
#define PTP_DTC_UINT8		0x0002
#define PTP_DTC_INT16		0x0003
#define PTP_DTC_UINT16		0x0004
#define PTP_DTC_INT32		0x0005
#define PTP_DTC_UINT32		0x0006
#define PTP_DTC_INT64		0x0007
#define PTP_DTC_UINT64		0x0008
#define PTP_DTC_INT128		0x0009
#define PTP_DTC_UINT128		0x000A
#define PTP_DTC_AINT8		0x4001
#define PTP_DTC_AUINT8		0x4002
#define PTP_DTC_AINT16		0x4003
#define PTP_DTC_AUINT16		0x4004
#define PTP_DTC_AINT32		0x4005
#define PTP_DTC_AUINT32		0x4006
#define PTP_DTC_AINT64		0x4007
#define PTP_DTC_AUINT64		0x4008
#define PTP_DTC_AINT128		0x4009
#define PTP_DTC_AUINT128	0x400A
#define PTP_DTC_STR		0xFFFF

/* Device Properties Codes */

#define PTP_DPC_Undefined		0x5000
#define PTP_DPC_BatteryLevel		0x5001
#define PTP_DPC_FunctionalMode		0x5002
#define PTP_DPC_ImageSize		0x5003
#define PTP_DPC_CompressionSetting	0x5004
#define PTP_DPC_WhiteBalance		0x5005
#define PTP_DPC_RGBGain			0x5006
#define PTP_DPC_FNumber			0x5007
#define PTP_DPC_FocalLength		0x5008
#define PTP_DPC_FocusDistance		0x5009
#define PTP_DPC_FocusMode		0x500A
#define PTP_DPC_ExposureMeteringMode	0x500B
#define PTP_DPC_FlashMode		0x500C
#define PTP_DPC_ExposureTime		0x500D
#define PTP_DPC_ExposureProgramMode	0x500E
#define PTP_DPC_ExposureIndex		0x500F
#define PTP_DPC_ExposureBiasCompensation	0x5010
#define PTP_DPC_DateTime		0x5011
#define PTP_DPC_CaptureDelay		0x5012
#define PTP_DPC_StillCaptureMode	0x5013
#define PTP_DPC_Contrast		0x5014
#define PTP_DPC_Sharpness		0x5015
#define PTP_DPC_DigitalZoom		0x5016
#define PTP_DPC_EffectMode		0x5017
#define PTP_DPC_BurstNumber		0x5018
#define PTP_DPC_BurstInterval		0x5019
#define PTP_DPC_TimelapseNumber		0x501A
#define PTP_DPC_TimelapseInterval	0x501B
#define PTP_DPC_FocusMeteringMode	0x501C
#define PTP_DPC_UploadURL		0x501D
#define PTP_DPC_Artist			0x501E
#define PTP_DPC_CopyrightInfo		0x501F

/* Proprietary vendor extension device property mask */
#define PTP_DPC_EXTENSION_MASK		0xF000
#define PTP_DPC_EXTENSION		0xD000

/* Vendor Extensions device property codes */

/* Eastman Kodak extension device property codes */
#define PTP_DPC_EK_ColorTemperature	0xD001
#define PTP_DPC_EK_DateTimeStampFormat	0xD002
#define PTP_DPC_EK_BeepMode		0xD003
#define PTP_DPC_EK_VideoOut		0xD004
#define PTP_DPC_EK_PowerSaving		0xD005
#define PTP_DPC_EK_UI_Language		0xD006
/* Canon extension device property codes */
#define PTP_DPC_CANON_BeepMode		0xD001
#define PTP_DPC_CANON_ViewfinderMode	0xD003
#define PTP_DPC_CANON_ImageQuality	0xD006
#define PTP_DPC_CANON_D007		0xD007
#define PTP_DPC_CANON_ImageSize		0xD008
#define PTP_DPC_CANON_FlashMode		0xD00A
#define PTP_DPC_CANON_TvAvSetting	0xD00C
#define PTP_DPC_CANON_MeteringMode	0xD010
#define PTP_DPC_CANON_MacroMode		0xD011
#define PTP_DPC_CANON_FocusingPoint	0xD012
#define PTP_DPC_CANON_WhiteBalance	0xD013
#define PTP_DPC_CANON_ISOSpeed		0xD01C
#define PTP_DPC_CANON_Aperture		0xD01D
#define PTP_DPC_CANON_ShutterSpeed	0xD01E
#define PTP_DPC_CANON_ExpCompensation	0xD01F
#define PTP_DPC_CANON_D029		0xD029
#define PTP_DPC_CANON_Zoom		0xD02A
#define PTP_DPC_CANON_SizeQualityMode	0xD02C
#define PTP_DPC_CANON_FlashMemory	0xD031
#define PTP_DPC_CANON_CameraModel	0xD032
#define PTP_DPC_CANON_CameraOwner	0xD033
#define PTP_DPC_CANON_UnixTime		0xD034
#define PTP_DPC_CANON_ViewfinderOutput	0xD036
#define PTP_DPC_CANON_RealImageWidth	0xD039
#define PTP_DPC_CANON_PhotoEffect	0xD040
#define PTP_DPC_CANON_AssistLight	0xD041
#define PTP_DPC_CANON_D045		0xD045

/* Nikon extension device property codes */
#define PTP_DPC_NIKON_ShootingBank			0xD010
#define PTP_DPC_NIKON_ShootingBankNameA 		0xD011
#define PTP_DPC_NIKON_ShootingBankNameB			0xD012
#define PTP_DPC_NIKON_ShootingBankNameC			0xD013
#define PTP_DPC_NIKON_ShootingBankNameD			0xD014
#define PTP_DPC_NIKON_RawCompression			0xD016
#define PTP_DPC_NIKON_WhiteBalanceAutoBias		0xD017
#define PTP_DPC_NIKON_WhiteBalanceTungstenBias		0xD018
#define PTP_DPC_NIKON_WhiteBalanceFlourescentBias	0xD019
#define PTP_DPC_NIKON_WhiteBalanceDaylightBias		0xD01A
#define PTP_DPC_NIKON_WhiteBalanceFlashBias		0xD01B
#define PTP_DPC_NIKON_WhiteBalanceCloudyBias		0xD01C
#define PTP_DPC_NIKON_WhiteBalanceShadeBias		0xD01D
#define PTP_DPC_NIKON_WhiteBalanceColorTemperature	0xD01E
#define PTP_DPC_NIKON_ImageSharpening			0xD02A
#define PTP_DPC_NIKON_ToneCompensation			0xD02B
#define PTP_DPC_NIKON_ColorMode				0xD02C
#define PTP_DPC_NIKON_HueAdjustment			0xD02D
#define PTP_DPC_NIKON_NonCPULensDataFocalLength		0xD02E
#define PTP_DPC_NIKON_NonCPULensDataMaximumAperture	0xD02F
#define PTP_DPC_NIKON_CSMMenuBankSelect			0xD040
#define PTP_DPC_NIKON_MenuBankNameA			0xD041
#define PTP_DPC_NIKON_MenuBankNameB			0xD042
#define PTP_DPC_NIKON_MenuBankNameC			0xD043
#define PTP_DPC_NIKON_MenuBankNameD			0xD044
#define PTP_DPC_NIKON_A1AFCModePriority			0xD048
#define PTP_DPC_NIKON_A2AFSModePriority			0xD049
#define PTP_DPC_NIKON_A3GroupDynamicAF			0xD04A
#define PTP_DPC_NIKON_A4AFActivation			0xD04B
#define PTP_DPC_NIKON_A5FocusAreaIllumManualFocus	0xD04C
#define PTP_DPC_NIKON_FocusAreaIllumContinuous		0xD04D
#define PTP_DPC_NIKON_FocusAreaIllumWhenSelected 	0xD04E
#define PTP_DPC_NIKON_FocusAreaWrap			0xD04F
#define PTP_DPC_NIKON_A7VerticalAFON			0xD050
#define PTP_DPC_NIKON_ISOAuto				0xD054
#define PTP_DPC_NIKON_B2ISOStep				0xD055
#define PTP_DPC_NIKON_EVStep				0xD056
#define PTP_DPC_NIKON_B4ExposureCompEv			0xD057
#define PTP_DPC_NIKON_ExposureCompensation		0xD058
#define PTP_DPC_NIKON_CenterWeightArea			0xD059
#define PTP_DPC_NIKON_AELockMode			0xD05E
#define PTP_DPC_NIKON_AELAFLMode			0xD05F
#define PTP_DPC_NIKON_MeterOff				0xD062
#define PTP_DPC_NIKON_SelfTimer				0xD063
#define PTP_DPC_NIKON_MonitorOff			0xD064
#define PTP_DPC_NIKON_D1ShootingSpeed			0xD068
#define PTP_DPC_NIKON_D2MaximumShots			0xD069
#define PTP_DPC_NIKON_D3ExpDelayMode			0xD06A
#define PTP_DPC_NIKON_LongExposureNoiseReduction	0xD06B
#define PTP_DPC_NIKON_FileNumberSequence		0xD06C
#define PTP_DPC_NIKON_D6ControlPanelFinderRearControl	0xD06D
#define PTP_DPC_NIKON_ControlPanelFinderViewfinder	0xD06E
#define PTP_DPC_NIKON_D7Illumination			0xD06F
#define PTP_DPC_NIKON_E1FlashSyncSpeed			0xD074
#define PTP_DPC_NIKON_FlashShutterSpeed			0xD075
#define PTP_DPC_NIKON_E3AAFlashMode			0xD076
#define PTP_DPC_NIKON_E4ModelingFlash			0xD077
#define PTP_DPC_NIKON_BracketSet			0xD078
#define PTP_DPC_NIKON_E6ManualModeBracketing		0xD079	
#define PTP_DPC_NIKON_BracketOrder			0xD07A
#define PTP_DPC_NIKON_E8AutoBracketSelection		0xD07B
#define PTP_DPC_NIKON_BracketingSet			0xD07C

#define PTP_DPC_NIKON_F1CenterButtonShootingMode	0xD080
#define PTP_DPC_NIKON_CenterButtonPlaybackMode		0xD081
#define PTP_DPC_NIKON_F2Multiselector			0xD082
#define PTP_DPC_NIKON_F3PhotoInfoPlayback		0xD083
#define PTP_DPC_NIKON_F4AssignFuncButton		0xD084
#define PTP_DPC_NIKON_F5CustomizeCommDials		0xD085
#define PTP_DPC_NIKON_ReverseCommandDial		0xD086
#define PTP_DPC_NIKON_ApertureSetting			0xD087
#define PTP_DPC_NIKON_MenusAndPlayback			0xD088
#define PTP_DPC_NIKON_F6ButtonsAndDials			0xD089
#define PTP_DPC_NIKON_NoCFCard				0xD08A
#define PTP_DPC_NIKON_ImageCommentString		0xD090
#define PTP_DPC_NIKON_ImageCommentAttach		0xD091
#define PTP_DPC_NIKON_ImageRotation			0xD092
#define PTP_DPC_NIKON_Bracketing			0xD0C0
#define PTP_DPC_NIKON_ExposureBracketingIntervalDist	0xD0C1
#define PTP_DPC_NIKON_BracketingProgram			0xD0C2
#define PTP_DPC_NIKON_WhiteBalanceBracketStep		0xD0C4
#define PTP_DPC_NIKON_LensID                            0xD0E0
#define PTP_DPC_NIKON_FocalLengthMin                    0xD0E3
#define PTP_DPC_NIKON_FocalLengthMax                    0xD0E4
#define PTP_DPC_NIKON_MaxApAtMinFocalLength             0xD0E5
#define PTP_DPC_NIKON_MaxApAtMaxFocalLength             0xD0E6
#define PTP_DPC_NIKON_ExposureTime			0xD100
#define PTP_DPC_NIKON_ACPower				0xD101
#define PTP_DPC_NIKON_MaximumShots			0xD103
#define PTP_DPC_NIKON_AFLLock				0xD104
#define PTP_DPC_NIKON_AutoExposureLock			0xD105
#define PTP_DPC_NIKON_AutoFocusLock			0xD106
#define PTP_DPC_NIKON_AutofocusLCDTopMode2		0xD107
#define PTP_DPC_NIKON_AutofocusArea			0xD108
#define PTP_DPC_NIKON_LightMeter			0xD10A
#define PTP_DPC_NIKON_CameraOrientation			0xD10E
#define PTP_DPC_NIKON_ExposureApertureLock		0xD111
#define PTP_DPC_NIKON_BeepOff				0xD160
#define PTP_DPC_NIKON_AutofocusMode			0xD161
#define PTP_DPC_NIKON_AFAssist				0xD163
#define PTP_DPC_NIKON_PADVPMode                         0xD164
#define PTP_DPC_NIKON_ImageReview			0xD165
#define PTP_DPC_NIKON_AFAreaIllumination                0xD166
#define PTP_DPC_NIKON_FlashMode                         0xD167
#define PTP_DPC_NIKON_FlashCommanderMode		0xD168
#define PTP_DPC_NIKON_FlashSign				0xD169
#define PTP_DPC_NIKON_GridDisplay                       0xD16C
#define PTP_DPC_NIKON_FlashModeManualPower		0xD16D
#define PTP_DPC_NIKON_FlashModeCommanderPower		0xD16E
#define PTP_DPC_NIKON_RemoteTimeout                     0xD16B
#define PTP_DPC_NIKON_GridDisplay			0xD16C
#define PTP_DPC_NIKON_BracketingIncrement		0xD190
#define PTP_DPC_NIKON_LowLight                          0xD1B0
#define PTP_DPC_NIKON_FlashOpen                         0xD1C0
#define PTP_DPC_NIKON_FlashCharged                      0xD1C1
#define PTP_DPC_NIKON_FlashExposureCompensation         0xD126
#define PTP_DPC_NIKON_CSMMenu			        0xD180
#define PTP_DPC_NIKON_OptimizeImage		        0xD140
#define PTP_DPC_NIKON_Saturation		        0xD142

/* Device Property Form Flag */

#define PTP_DPFF_None			0x00
#define PTP_DPFF_Range			0x01
#define PTP_DPFF_Enumeration		0x02

/* Device Property GetSet type */
#define PTP_DPGS_Get			0x00
#define PTP_DPGS_GetSet			0x01

/* Glue stuff starts here */

typedef struct _PTPParams PTPParams;

/* raw write functions */
typedef short (* PTPIOReadFunc)	(unsigned char *bytes, unsigned int size,
				 void *data);
typedef short (* PTPIOWriteFunc)(unsigned char *bytes, unsigned int size,
				 void *data);
/*
 * This functions take PTP oriented arguments and send them over an
 * appropriate data layer doing byteorder conversion accordingly.
 */
typedef uint16_t (* PTPIOSendReq)	(PTPParams* params, PTPContainer* req);
typedef uint16_t (* PTPIOSendData)	(PTPParams* params, PTPContainer* ptp,
					unsigned char *data, unsigned int size);
typedef uint16_t (* PTPIOGetResp)	(PTPParams* params, PTPContainer* resp);
typedef uint16_t (* PTPIOGetData)	(PTPParams* params, PTPContainer* ptp,
					unsigned char **data);
/* debug functions */
typedef void (* PTPErrorFunc) (void *data, const char *format, va_list args);
typedef void (* PTPDebugFunc) (void *data, const char *format, va_list args);

struct _PTPParams {
	/* data layer byteorder */
	uint8_t	byteorder;

	/* Data layer IO functions */
	PTPIOReadFunc	read_func;
	PTPIOWriteFunc	write_func;
	PTPIOReadFunc	check_int_func;
	PTPIOReadFunc	check_int_fast_func;

	/* Custom IO functions */
	PTPIOSendReq	sendreq_func;
	PTPIOSendData	senddata_func;
	PTPIOGetResp	getresp_func;
	PTPIOGetData	getdata_func;
	PTPIOGetResp	event_check;
	PTPIOGetResp	event_wait;

	/* Custom error and debug function */
	PTPErrorFunc error_func;
	PTPDebugFunc debug_func;

	/* Data passed to above functions */
	void *data;

	/* ptp transaction ID */
	uint32_t transaction_id;
	/* ptp session ID */
	uint32_t session_id;

	/* internal structures used by ptp driver */
	PTPObjectHandles handles;
	PTPObjectInfo * objectinfo;
	PTPDeviceInfo deviceinfo;
};

/* last, but not least - ptp functions */
uint16_t ptp_usb_sendreq	(PTPParams* params, PTPContainer* req);
uint16_t ptp_usb_senddata	(PTPParams* params, PTPContainer* ptp,
				unsigned char *data, unsigned int size);
uint16_t ptp_usb_getresp	(PTPParams* params, PTPContainer* resp);
uint16_t ptp_usb_getdata	(PTPParams* params, PTPContainer* ptp, 
				unsigned char **data);
uint16_t ptp_usb_event_check	(PTPParams* params, PTPContainer* event);
uint16_t ptp_usb_event_wait		(PTPParams* params, PTPContainer* event);

uint16_t ptp_getdeviceinfo	(PTPParams* params, PTPDeviceInfo* deviceinfo);

uint16_t ptp_opensession	(PTPParams *params, uint32_t session);
uint16_t ptp_closesession	(PTPParams *params);

uint16_t ptp_getstorageids	(PTPParams* params, PTPStorageIDs* storageids);
uint16_t ptp_getstorageinfo 	(PTPParams* params, uint32_t storageid,
				PTPStorageInfo* storageinfo);

uint16_t ptp_getobjecthandles 	(PTPParams* params, uint32_t storage,
				uint32_t objectformatcode,
				uint32_t associationOH,
				PTPObjectHandles* objecthandles);

uint16_t ptp_getobjectinfo	(PTPParams *params, uint32_t handle,
				PTPObjectInfo* objectinfo);

uint16_t ptp_getobject		(PTPParams *params, uint32_t handle,
				char** object);
uint16_t ptp_getthumb		(PTPParams *params, uint32_t handle,
				char** object);

uint16_t ptp_deleteobject	(PTPParams* params, uint32_t handle,
				uint32_t ofc);

uint16_t ptp_sendobjectinfo	(PTPParams* params, uint32_t* store,
				uint32_t* parenthandle, uint32_t* handle,
				PTPObjectInfo* objectinfo);
uint16_t ptp_sendobject		(PTPParams* params, char* object,
				uint32_t size);

uint16_t ptp_initiatecapture	(PTPParams* params, uint32_t storageid,
				uint32_t ofc);

uint16_t ptp_getdevicepropdesc	(PTPParams* params, uint16_t propcode,
				PTPDevicePropDesc *devicepropertydesc);
uint16_t ptp_getdevicepropvalue	(PTPParams* params, uint16_t propcode,
				void** value, uint16_t datatype);
uint16_t ptp_setdevicepropvalue (PTPParams* params, uint16_t propcode,
                        	void* value, uint16_t datatype);


uint16_t ptp_ek_sendfileobjectinfo (PTPParams* params, uint32_t* store,
				uint32_t* parenthandle, uint32_t* handle,
				PTPObjectInfo* objectinfo);
uint16_t ptp_ek_sendfileobject	(PTPParams* params, char* object,
				uint32_t size);
				
/* Canon PTP extensions */

uint16_t ptp_canon_getobjectsize (PTPParams* params, uint32_t handle,
				uint32_t p2, uint32_t* size, uint32_t* rp2);

uint16_t ptp_canon_startshootingmode (PTPParams* params);
uint16_t ptp_canon_endshootingmode (PTPParams* params);

uint16_t ptp_canon_viewfinderon (PTPParams* params);
uint16_t ptp_canon_viewfinderoff (PTPParams* params);

uint16_t ptp_canon_reflectchanges (PTPParams* params, uint32_t p1);
uint16_t ptp_canon_checkevent (PTPParams* params, 
				PTPUSBEventContainer* event, int* isevent);
uint16_t ptp_canon_focuslock (PTPParams* params);
uint16_t ptp_canon_focusunlock (PTPParams* params);
uint16_t ptp_canon_initiatecaptureinmemory (PTPParams* params);
uint16_t ptp_canon_getpartialobject (PTPParams* params, uint32_t handle, 
				uint32_t offset, uint32_t size,
				uint32_t pos, char** block, 
				uint32_t* readnum);
uint16_t ptp_canon_getviewfinderimage (PTPParams* params, char** image,
				uint32_t* size);
uint16_t ptp_canon_getchanges (PTPParams* params, uint16_t** props,
				uint32_t* propnum); 
uint16_t ptp_canon_getfolderentries (PTPParams* params, uint32_t store,
				uint32_t p2, uint32_t parenthandle,
				uint32_t handle, 
				PTPCANONFolderEntry** entries,
				uint32_t* entnum);

/* Nikon extensions */
uint16_t ptp_nikon_setcontrolmode (PTPParams* params, uint32_t mode);
uint16_t ptp_nikon_directcapture (PTPParams* params, uint32_t unknown);
uint16_t ptp_nikon_checkevent (PTPParams* params,
				PTPUSBEventContainer** event, uint16_t* evnum);
uint16_t ptp_nikon_keepalive (PTPParams* params);


/* Non PTP protocol functions */
int ptp_operation_issupported	(PTPParams* params, uint16_t operation);
int ptp_property_issupported	(PTPParams* params, uint16_t property);

void ptp_free_devicepropdesc	(PTPDevicePropDesc* dpd);
void ptp_perror			(PTPParams* params, uint16_t error);
const char*
ptp_get_datatype_name		(PTPParams* params, uint16_t dt);
const char*
ptp_get_operation_name		(PTPParams* params, uint16_t oc);
const char*
ptp_prop_getname		(PTPParams* params, uint16_t dpc);

/* Properties handling functions */
const char* ptp_prop_getdesc	(PTPParams* params, PTPDevicePropDesc *dpd,
					void *val);
const char* ptp_prop_getdescbystring
				(PTPParams* params,PTPDevicePropDesc *dpd,
				const char *strval);

const char * ptp_prop_tostr	(PTPParams* params, PTPDevicePropDesc *dpd,
					void *val);
uint16_t ptp_prop_getcodebyname	(PTPParams* params, char* propname);
const char* ptp_prop_getvalbyname
				(PTPParams* params, char* name, uint16_t dpc);


/******************/
/* CHDK extension */
/******************/
#define PTP_CHDK_VERSION_MAJOR 2  // increase only with backwards incompatible changes (and reset minor)
#define PTP_CHDK_VERSION_MINOR 0  // increase with extensions of functionality
/*
protocol version history
0.1 - initial proposal from mweerden, + luar
0.2 - Added ScriptStatus and ScriptSupport, based on work by ultimA
1.0 - removed old script result code (luar), replace with message system
2.0 - return PTP_CHDK_TYPE_TABLE for tables instead of TYPE_STRING, allow return of empty strings
*/

#define PTP_OC_CHDK 0x9999

// N.B.: unused parameters should be set to 0
enum ptp_chdk_command {
  PTP_CHDK_Version = 0,     // return param1 is major version number
                            // return param2 is minor version number
  PTP_CHDK_GetMemory,       // param2 is base address (not NULL; circumvent by taking 0xFFFFFFFF and size+1)
                            // param3 is size (in bytes)
                            // return data is memory block
  PTP_CHDK_SetMemory,       // param2 is address
                            // param3 is size (in bytes)
                            // data is new memory block
  PTP_CHDK_CallFunction,    // data is array of function pointer and (long) arguments  (max: 10 args)
                            // return param1 is return value
  PTP_CHDK_TempData,        // data is data to be stored for later
                            // param2 is for the TD flags below
  PTP_CHDK_UploadFile,      // data is 4-byte length of filename, followed by filename and contents
  PTP_CHDK_DownloadFile,    // preceded by PTP_CHDK_TempData with filename
                            // return data are file contents
  PTP_CHDK_ExecuteScript,   // data is script to be executed
                            // param2 is language of script
                            // return param1 is script id, like a process id
                            // return param2 is status, PTP_CHDK_S_ERRTYPE*
  PTP_CHDK_ScriptStatus,    // Script execution status
                            // return param1 bits
                            // PTP_CHDK_SCRIPT_STATUS_RUN is set if a script running, cleared if not
                            // PTP_CHDK_SCRIPT_STATUS_MSG is set if script messages from script waiting to be read
                            // all other bits and params are reserved for future use
  PTP_CHDK_ScriptSupport,   // Which scripting interfaces are supported in this build
                            // param1 CHDK_PTP_SUPPORT_LUA is set if lua is supported, cleared if not
                            // all other bits and params are reserved for future use
  PTP_CHDK_ReadScriptMsg,   // read next message from camera script system
                            // return param1 is chdk_ptp_s_msg_type
                            // return param2 is message subtype:
                            //   for script return and users this is ptp_chdk_script_data_type
                            //   for error ptp_chdk_script_error_type
                            // return param3 is script id of script that generated the message
                            // return param4 is length of the message data. 
                            // return data is message.
                            // A minimum of 1 bytes of zeros is returned if the message has no data (empty string or type NONE)
  PTP_CHDK_WriteScriptMsg,  // write a message for scripts running on camera
                            // input param2 is target script id, 0=don't care. Messages for a non-running script will be discarded
                            // data length is handled by ptp data phase
                            // input messages do not have type or subtype, they are always a string destined for the script (similar to USER/string)
                            // output param1 is ptp_chdk_script_msg_status
  PTP_CHDK_GDBStub_Upload,
                            // param2 is the transfer buffer size
  PTP_CHDK_GDBStub_Download,
                            // param2 is the transfer buffer size
};

// data types as used by ReadScriptMessage
enum ptp_chdk_script_data_type {
  PTP_CHDK_TYPE_UNSUPPORTED = 0, // type name will be returned in data
  PTP_CHDK_TYPE_NIL,
  PTP_CHDK_TYPE_BOOLEAN,
  PTP_CHDK_TYPE_INTEGER,
  PTP_CHDK_TYPE_STRING, // Empty strings are returned with length=0
  PTP_CHDK_TYPE_TABLE,  // tables are converted to a string by usb_msg_table_to_string, 
                        // this function can be overridden in lua to change the format
                        // the string may be empty for an empty table
};

// TempData flags
#define PTP_CHDK_TD_DOWNLOAD  0x1  // download data instead of upload
#define PTP_CHDK_TD_CLEAR     0x2  // clear the stored data; with DOWNLOAD this
                                   // means first download, then clear and
                                   // without DOWNLOAD this means no uploading,
                                   // just clear

// Script Languages - for execution only lua is supported for now
#define PTP_CHDK_SL_LUA    0
#define PTP_CHDK_SL_UBASIC 1

// bit flags for script status
#define PTP_CHDK_SCRIPT_STATUS_RUN   0x1 // script running
#define PTP_CHDK_SCRIPT_STATUS_MSG   0x2 // messages waiting
// bit flags for scripting support
#define PTP_CHDK_SCRIPT_SUPPORT_LUA  0x1

// message types
enum ptp_chdk_script_msg_type {
    PTP_CHDK_S_MSGTYPE_NONE = 0, // no messages waiting
    PTP_CHDK_S_MSGTYPE_ERR,      // error message
    PTP_CHDK_S_MSGTYPE_RET,      // script return value
    PTP_CHDK_S_MSGTYPE_USER,     // message queued by script
// TODO chdk console data ?
};

// error subtypes for PTP_CHDK_S_MSGTYPE_ERR and script startup status
enum ptp_chdk_script_error_type {
    PTP_CHDK_S_ERRTYPE_NONE = 0,
    PTP_CHDK_S_ERRTYPE_COMPILE,
    PTP_CHDK_S_ERRTYPE_RUN,
};

// message status
enum ptp_chdk_script_msg_status {
    PTP_CHDK_S_MSGSTATUS_OK = 0, // queued ok
    PTP_CHDK_S_MSGSTATUS_NOTRUN, // no script is running
    PTP_CHDK_S_MSGSTATUS_QFULL,  // queue is full
    PTP_CHDK_S_MSGSTATUS_BADID,  // specified ID is not running
};

int ptp_chdk_shutdown_hard(PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_shutdown_soft(PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_reboot(PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_reboot_fw_update(char *path, PTPParams* params, PTPDeviceInfo* deviceinfo);
char* ptp_chdk_get_memory(int start, int num, PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_set_memory_long(int addr, int val, PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_call(int *args, int size, int *ret, PTPParams* params, PTPDeviceInfo* deviceinfo);
int* ptp_chdk_get_propcase(int start, int num, PTPParams* params, PTPDeviceInfo* deviceinfo);
char* ptp_chdk_get_paramdata(int start, int num, PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_upload(char *local_fn, char *remote_fn, PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_download(char *remote_fn, char *local_fn, PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_switch_mode(int mode, PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_exec_lua(char *script, int get_result, PTPParams* params, PTPDeviceInfo* deviceinfo);
int ptp_chdk_get_version(PTPParams* params, PTPDeviceInfo* deviceinfo, int *major, int *minor);
int ptp_chdk_get_script_support(PTPParams* params, PTPDeviceInfo* deviceinfo, int *status);
int ptp_chdk_get_script_status(PTPParams* params, PTPDeviceInfo* deviceinfo, int *status);


int ptp_chdk_gdb_download(char *buf, PTPParams* params, PTPDeviceInfo* deviceinfo);
char* ptp_chdk_gdb_upload(PTPParams* params, PTPDeviceInfo* deviceinfo);

typedef struct {
    unsigned size;
    unsigned script_id; // id of script message is to/from 
    unsigned type;
    unsigned subtype;
    char data[];
} ptp_chdk_script_msg;

#endif /* __PTP_H__ */
