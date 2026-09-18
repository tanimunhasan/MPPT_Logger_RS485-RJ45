//******************************************************************************
// File: ConfigStart.h
// Purpose: Startup and operation-mode state machine.
//******************************************************************************

#ifndef __CONFIG_START_H
#define __CONFIG_START_H

#include "Core.h"

OPERATION_MODE_TYPE_ENUM run_startup(bool isFirstBootup);

void run_config_mode(void);
void run_test_mode(void);
[[noreturn]] void run_normal_mode(void);
[[noreturn]] void run_hibernate_mode(void);
[[noreturn]] void run_error_mode(void);

void AnnounceStartUp(bool isFirstBootup);

#endif // __CONFIG_START_H
