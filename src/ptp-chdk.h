#ifndef __PTP_CHDK_H
#define __PTP_CHDK_H

// N.B.: not checking to see if CAM_CHDK_PTP is set as ptp.h is currently
// only included by ptp.c (which already checks this before including ptp.h)

#define PTP_CHDK_VERSION_MAJOR 0  // increase only with backwards incompatible changes (and reset minor)
#define PTP_CHDK_VERSION_MINOR 1  // increase with extensions of functionality

#define PTP_OC_CHDK 0x9999

// N.B.: unused parameters should be set to 0
enum {
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

} ptp_chdk_command;

// data types as used by TempData and ExecuteScript
enum {
	PTP_CHDK_TYPE_NOTHING = 0,
	PTP_CHDK_TYPE_NIL,
	PTP_CHDK_TYPE_BOOLEAN,
	PTP_CHDK_TYPE_INTEGER,
	PTP_CHDK_TYPE_STRING
} ptp_chdk_type;

// TempData flags
#define PTP_CHDK_TD_DOWNLOAD  0x1  // download data instead of upload
#define PTP_CHDK_TD_CLEAR     0x2  // clear the stored data; with DOWNLOAD this
// means first download, then clear and
// without DOWNLOAD this means no uploading,
// just clear

// ExecuteScript flags
#define PTP_CHDK_ES_WAIT      0x1  // do not return after script initialisation
// but wait until execution has finished
// (should only be used with short execution
// times)
#define PTP_CHDK_ES_RESULT    0x2  // only in combination with WAIT; return
// param1 will be the ptp_chdk_type of the
// code result and param2 the value (booleans
// and integers) or length (strings)

// Script Languages
#define PTP_CHDK_SL_LUA    0
#define PTP_CHDK_SL_UBASIC 1

#endif // __PTP_CHDK_H
