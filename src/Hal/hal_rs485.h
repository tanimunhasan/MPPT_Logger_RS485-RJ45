//******************************************************************************
// File: hal_rs485.h
// Purpose: T-CAN485 UART/transceiver hardware abstraction.
//******************************************************************************

#ifndef __HAL_RS485_H
#define __HAL_RS485_H

#include <Arduino.h>

bool HAL_RS485_Init(void);
void HAL_RS485_DeInit(void);
bool HAL_RS485_IsInitialised(void);
Stream& HAL_RS485_Stream(void);

#endif // __HAL_RS485_H
