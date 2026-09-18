//******************************************************************************
// File: hal_power.h
// Purpose: ESP32 reset/wake/low-power hardware abstraction.
//******************************************************************************

#ifndef __HAL_POWER_H
#define __HAL_POWER_H

#include <stdint.h>
#include <stdbool.h>

void HAL_Power_Init(void);

bool HAL_Power_IsTimerWake(void);
const char* HAL_Power_GetWakeReasonString(void);
const char* HAL_Power_GetResetReasonString(void);

[[noreturn]] void HAL_Power_DeepSleep(uint32_t seconds);
[[noreturn]] void HAL_Power_Hibernate(void);
[[noreturn]] void HAL_Power_Reset(void);

#endif // __HAL_POWER_H
