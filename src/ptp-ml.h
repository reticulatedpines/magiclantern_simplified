#ifndef __PTP_ML_H
#define __PTP_ML_H

// N.B.: not checking to see if CAM_CHDK_PTP is set as ptp.h is currently
// only included by ptp.c (which already checks this before including ptp.h)

#define PTP_ML_VERSION_MAJOR 0  // increase only with backwards incompatible changes (and reset minor)
#define PTP_ML_VERSION_MINOR 1  // increase with extensions of functionality

#define PTP_ML_CODE 0xA1E8      // ALEX in hexa

// parameter count; data direction; return parameter values
enum {
	PTP_ML_USB_Version    = 0x0000,     // 0 param; no data; P1: major, P2: minor ML version
	PTP_ML_Version        = 0x0001,     // 1 param; out data - ML version strings; no result
										//   P1: 0:build_version 1:build_id, 2:build_date 3:build_user
	PTP_ML_GetMemory      = 0x0002,     // 2 param; out data - memory contents; no result
										//   P1: position; P2: size
	PTP_ML_SetMemory      = 0x0003,     // 2 param; in data - memoty contents; no result
										//   P1: position; P2: size
	PTP_ML_GetMenus       = 0x0004,     // 0 param; out data - menu structure; no result
	PTP_ML_GetMenuStructs = 0x0005,     // 1 param; out data - menu structure; no result
										//   P1: submenu id
	PTP_ML_GetMenuStruct  = 0x0006,     // 1 param; out data - menu structure; no result
										//   P1: submenu id
	PTP_ML_SetMenu        = 0x0007,     // 3 param; no data; P1: new value
										//   P1: submenu id; P2: change type; P3: new value
										//   change type: 0: select, 1: selrev, 2: Q, 3: use P3; any other: get data only
} ptp_ml_command;

// flags for submenus
#define PTP_ML_SUBMENU_HAS_SELECT (1<<0)
#define PTP_ML_SUBMENU_HAS_SELECT_REVERSE (1<<1)
#define PTP_ML_SUBMENU_HAS_SELECT_Q (1<<2)
#define PTP_ML_SUBMENU_IS_ESSENTIAL (1<<3)
#define PTP_ML_SUBMENU_HAS_CHOICE (1<<4)
#define PTP_ML_SUBMENU_HAS_SUBMENU (1<<5)
#define PTP_ML_SUBMENU_IS_SELECTED (1<<6)
#define PTP_ML_SUBMENU_HAS_DISPLAY (1<<7)

// shifts for some flags
#define PTP_ML_SUBMENU_EDIT_MODE_SHIFT 12
#define PTP_ML_SUBMENU_ICON_TYPE_SHIFT 16
#define PTP_ML_SUBMENU_UNIT_SHIFT 20

#endif // __PTP_ML_H

