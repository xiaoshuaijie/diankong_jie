#ifndef __CHASSIS_TASK_H__
#define __CHASSIS_TASK_H__

#include "comp_cmd.h"
#include "comp_utils.h"

extern err_t chassis_status[4];

void start_chassis_task(void *argument);

#endif
