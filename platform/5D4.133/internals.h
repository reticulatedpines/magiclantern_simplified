/**
 * Camera internals for 5D Mark IV 1.3.3
 */

// Has SD and CF slots
#define CONFIG_DUAL_SLOT

// This camera has a DIGIC VI chip
#define CONFIG_DIGIC_VI

// Digic 6 does not have bitmap font in ROM, try to load it from card
#define CONFIG_NO_BFNT

// SRM is untested, this define is to allowing building
// without SRM_BUFFER_SIZE being found
#define CONFIG_MEMORY_SRM_NOT_WORKING

#define CONFIG_NEW_TASK_STRUCTS
#define CONFIG_TASK_STRUCT_V2
#define CONFIG_TASK_ATTR_STRUCT_V4
