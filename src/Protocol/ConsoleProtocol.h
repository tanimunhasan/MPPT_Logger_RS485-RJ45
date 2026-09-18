//******************************************************************************
// File: ConsoleProtocol.h
// Purpose: User serial command-line interface.
//******************************************************************************

#ifndef __CONSOLE_PROTOCOL_H
#define __CONSOLE_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

void Console_Init(void);
void Console_Process(void);
void Console_PrintHelp(void);

// Used only on a non-timer cold boot after commissioning.
// Returns true if the operator entered the console with '?'.
bool Console_WaitForStartupRequest(uint32_t timeoutMs);

#endif // __CONSOLE_PROTOCOL_H
