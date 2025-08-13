#ifndef _TASK_UTILS_H_
#define _TASK_UTILS_H_

// Small functions related to DryOS tasks.  Few dependencies,
// so it's easy for them to be included in various different build contexts,
// e.g. installer, modules.

#include "dryos.h"
/*
#include "property.h"
#include "bmp.h"
#include "tskmon.h"
#include "menu.h"
#include "crop-mode-hack.h"
*/

const char *get_current_task_name();
const char *get_task_name_from_id(int id);
int get_current_task_id();

#endif // _TASK_UTILS_H_
