//******************************************************************************
// File: ModbusProtocol.h
// Purpose: Generic Modbus RTU master wrapper.
//******************************************************************************

#ifndef __MODBUS_PROTOCOL_H
#define __MODBUS_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

bool ModbusProtocol_Init(void);
void ModbusProtocol_DeInit(void);
bool ModbusProtocol_IsInitialised(void);

bool Modbus_ReadInputRegisters(
    uint16_t address,
    uint16_t quantity,
    uint16_t *outValues);

bool Modbus_ReadHoldingRegisters(
    uint16_t address,
    uint16_t quantity,
    uint16_t *outValues);

bool Modbus_WriteHoldingRegister(
    uint16_t address,
    uint16_t value);

bool Modbus_WriteHoldingRegisters(
    uint16_t address,
    const uint16_t *values,
    uint16_t quantity);

uint8_t Modbus_GetLastError(void);

#endif // __MODBUS_PROTOCOL_H
