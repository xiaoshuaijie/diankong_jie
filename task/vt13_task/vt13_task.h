#ifndef _vt13_task_h
#define _vt13_task_h

#include "comp_type.h"
#include "main.h"
#include "vt13.h"

extern vt13_t vt13;
extern vt13_cmd_rc_t vt13_cmd_rc;
extern err_t vt13_status[4];

void Start_vt13_task(void *argument);

#endif
