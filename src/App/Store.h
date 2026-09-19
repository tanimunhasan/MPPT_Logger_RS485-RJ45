//******************************************************************************
// File: Store.h
// Purpose: Persistent NVS configuration + RTC-RAM sample buffer + SD logging.
//******************************************************************************

#ifndef __STORE_H
#define __STORE_H

#include <stdint.h>
#include <stdbool.h>

#include "Core.h"

void Store_Init(bool timerWake);

bool Store_LoadConfiguration(APP_CONFIG_TYPE *config);
bool Store_SaveConfiguration(APP_CONFIG_TYPE *config);
void Store_ClearConfiguration(void);

bool Store_AddSample(const MPPT_SAMPLE_TYPE *sample);
bool Store_FlushSamples(void);
bool Store_PrintMonthlyLog(uint16_t year, uint8_t month);

uint8_t Store_GetBufferedCount(void);
uint32_t Store_GetLostSampleCount(void);

#endif // __STORE_H
