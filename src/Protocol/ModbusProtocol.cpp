//******************************************************************************
// File: ModbusProtocol.cpp
//******************************************************************************

#include "ModbusProtocol.h"

#include <Arduino.h>
#include <ModbusMaster.h>

#include "../user_config.h"
#include "../Hal/hal_rs485.h"

namespace
{
    ModbusMaster gModbus;
    bool gInitialised = false;
    uint8_t gLastError = ModbusMaster::ku8MBSuccess;

    bool copyResponse(uint16_t quantity, uint16_t *outValues)
    {
        if (outValues == nullptr)
            return false;

        for (uint16_t i = 0; i < quantity; ++i)
            outValues[i] = gModbus.getResponseBuffer(i);

        return true;
    }

    bool runReadInput(uint16_t address, uint16_t quantity, uint16_t *outValues)
    {
        for (uint8_t attempt = 0; attempt < MODBUS_RETRY_COUNT; ++attempt)
        {
            gLastError = gModbus.readInputRegisters(address, quantity);

            if (gLastError == ModbusMaster::ku8MBSuccess)
                return copyResponse(quantity, outValues);

            delay(MODBUS_RETRY_DELAY_MS);
        }

        return false;
    }

    bool runReadHolding(uint16_t address, uint16_t quantity, uint16_t *outValues)
    {
        for (uint8_t attempt = 0; attempt < MODBUS_RETRY_COUNT; ++attempt)
        {
            gLastError = gModbus.readHoldingRegisters(address, quantity);

            if (gLastError == ModbusMaster::ku8MBSuccess)
                return copyResponse(quantity, outValues);

            delay(MODBUS_RETRY_DELAY_MS);
        }

        return false;
    }
}

bool ModbusProtocol_Init(void)
{
    if (gInitialised)
        return true;

    if (!HAL_RS485_Init())
        return false;

    gModbus.begin(EPEVER_MODBUS_ADDRESS, HAL_RS485_Stream());

    gLastError = ModbusMaster::ku8MBSuccess;
    gInitialised = true;
    return true;
}

void ModbusProtocol_DeInit(void)
{
    if (!gInitialised)
        return;

    HAL_RS485_DeInit();
    gInitialised = false;
}

bool ModbusProtocol_IsInitialised(void)
{
    return gInitialised;
}

bool Modbus_ReadInputRegisters(
    uint16_t address,
    uint16_t quantity,
    uint16_t *outValues)
{
    if (!ModbusProtocol_Init())
        return false;

    return runReadInput(address, quantity, outValues);
}

bool Modbus_ReadHoldingRegisters(
    uint16_t address,
    uint16_t quantity,
    uint16_t *outValues)
{
    if (!ModbusProtocol_Init())
        return false;

    return runReadHolding(address, quantity, outValues);
}

bool Modbus_WriteHoldingRegister(
    uint16_t address,
    uint16_t value)
{
    if (!ModbusProtocol_Init())
        return false;

    for (uint8_t attempt = 0; attempt < MODBUS_RETRY_COUNT; ++attempt)
    {
        gLastError = gModbus.writeSingleRegister(address, value);

        if (gLastError == ModbusMaster::ku8MBSuccess)
            return true;

        delay(MODBUS_RETRY_DELAY_MS);
    }

    return false;
}

bool Modbus_WriteHoldingRegisters(
    uint16_t address,
    const uint16_t *values,
    uint16_t quantity)
{
    if ((values == nullptr) || (quantity == 0U) || (quantity > 64U))
        return false;

    if (!ModbusProtocol_Init())
        return false;

    gModbus.clearTransmitBuffer();

    for (uint16_t i = 0; i < quantity; ++i)
        gModbus.setTransmitBuffer(i, values[i]);

    for (uint8_t attempt = 0; attempt < MODBUS_RETRY_COUNT; ++attempt)
    {
        gLastError = gModbus.writeMultipleRegisters(address, quantity);

        if (gLastError == ModbusMaster::ku8MBSuccess)
        {
            gModbus.clearTransmitBuffer();
            return true;
        }

        delay(MODBUS_RETRY_DELAY_MS);
    }

    gModbus.clearTransmitBuffer();
    return false;
}

uint8_t Modbus_GetLastError(void)
{
    return gLastError;
}
