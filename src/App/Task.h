//******************************************************************************
// File: Task.h
// Purpose: Application-level normal/test work.
//******************************************************************************

#ifndef __TASK_H
#define __TASK_H

#include <stdbool.h>

#include "../Protocol/EpeverProtocol.h"

bool Task_ReadMppt(MPPT_SAMPLE_TYPE *sample);
void Task_PrintMpptSample(const MPPT_SAMPLE_TYPE *sample);
void Task_PrintEnergy(const MPPT_SAMPLE_TYPE *sample);

bool Task_RunNormalCycle(void);

#endif // __TASK_H
