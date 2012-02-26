#ifndef __PTP_ML_H
#define __PTP_ML_H

// N.B.: not checking to see if CAM_CHDK_PTP is set as ptp.h is currently
// only included by ptp.c (which already checks this before including ptp.h)

#define PTP_ML_VERSION_MAJOR 0  // increase only with backwards incompatible changes (and reset minor)
#define PTP_ML_VERSION_MINOR 1  // increase with extensions of functionality

#define PTP_ML_CHDK 0xA1E8

// N.B.: unused parameters should be set to 0
enum {
  PTP_ML_USB_Version = 0,   // return param1 is major version number
                            // return param2 is minor version number
  PTP_ML_Version,
} ptp_ml_command;

// data types as used by TempData and ExecuteScript
enum {
  PTP_ML_TYPE_NOTHING = 0,
  PTP_ML_TYPE_NIL,
  PTP_ML_TYPE_BOOLEAN,
  PTP_ML_TYPE_INTEGER,
  PTP_ML_TYPE_STRING
} ptp_ml_type;

#endif // __PTP_ML_H

