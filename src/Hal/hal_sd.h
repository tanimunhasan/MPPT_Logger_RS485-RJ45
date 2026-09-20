//******************************************************************************
// File: hal_sd.h
// Purpose: TF/microSD hardware abstraction.
//******************************************************************************

#ifndef __HAL_SD_H
#define __HAL_SD_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

bool HAL_SD_Init(void);
void HAL_SD_DeInit(void);
void HAL_SD_PrepareForSleep(void);
bool HAL_SD_IsInitialised(void);

bool HAL_SD_Exists(const char *path);
File HAL_SD_OpenAppend(const char *path);
File HAL_SD_OpenRead(const char *path);

#endif // __HAL_SD_H
