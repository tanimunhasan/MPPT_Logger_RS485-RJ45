//******************************************************************************
// File: hal_rs485.cpp
//******************************************************************************

#include "hal_rs485.h"

#include "../user_config.h"

namespace
{
    HardwareSerial gRs485Serial(1);
    bool gInitialised = false;

    int inactiveLevel(int activeLevel)
    {
        return (activeLevel == HIGH) ? LOW : HIGH;
    }
}

bool HAL_RS485_Init(void)
{
    if (gInitialised)
        return true;

    pinMode(TCAN485_5V_BOOST_EN_PIN, OUTPUT);
    pinMode(TCAN485_RS485_EN_PIN, OUTPUT);
    pinMode(TCAN485_RS485_CALLBACK_PIN, OUTPUT);

    digitalWrite(
        TCAN485_5V_BOOST_EN_PIN,
        TCAN485_5V_BOOST_ACTIVE_LEVEL);

    digitalWrite(
        TCAN485_RS485_EN_PIN,
        TCAN485_RS485_EN_ACTIVE_LEVEL);

    digitalWrite(
        TCAN485_RS485_CALLBACK_PIN,
        TCAN485_RS485_CALLBACK_ACTIVE_LEVEL);

    delay(80);

    gRs485Serial.begin(
        EPEVER_MODBUS_BAUD,
        SERIAL_8N1,
        TCAN485_RS485_RX_PIN,
        TCAN485_RS485_TX_PIN);

    delay(20);

    gInitialised = true;
    return true;
}

void HAL_RS485_DeInit(void)
{
    if (gInitialised)
        gRs485Serial.end();

    pinMode(TCAN485_RS485_CALLBACK_PIN, OUTPUT);
    pinMode(TCAN485_RS485_EN_PIN, OUTPUT);
    pinMode(TCAN485_5V_BOOST_EN_PIN, OUTPUT);

    digitalWrite(
        TCAN485_RS485_CALLBACK_PIN,
        inactiveLevel(TCAN485_RS485_CALLBACK_ACTIVE_LEVEL));

    digitalWrite(
        TCAN485_RS485_EN_PIN,
        inactiveLevel(TCAN485_RS485_EN_ACTIVE_LEVEL));

    digitalWrite(
        TCAN485_5V_BOOST_EN_PIN,
        inactiveLevel(TCAN485_5V_BOOST_ACTIVE_LEVEL));

    gInitialised = false;
}

bool HAL_RS485_IsInitialised(void)
{
    return gInitialised;
}

Stream& HAL_RS485_Stream(void)
{
    return gRs485Serial;
}
