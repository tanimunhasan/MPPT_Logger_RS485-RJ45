//******************************************************************************
// File: EpeverProtocol.cpp
//******************************************************************************

#include "EpeverProtocol.h"

#include <Arduino.h>
#include <math.h>

#include "EpeverRegisters.h"
#include "ModbusProtocol.h"

namespace
{
    uint32_t combineLowHigh(uint16_t lowWord, uint16_t highWord)
    {
        return (uint32_t)lowWord | ((uint32_t)highWord << 16);
    }

    uint16_t scale100(float value)
    {
        if (value <= 0.0f)
            return 0U;

        return (uint16_t)lroundf(value * 100.0f);
    }

    bool floatClose(float a, float b)
    {
        return fabsf(a - b) <= 0.011f;
    }

    bool rtcIsValid(const MPPT_RTC_TYPE *rtc)
    {
        if (rtc == nullptr)
            return false;

        return
            (rtc->Year >= 2000U) && (rtc->Year <= 2099U) &&
            (rtc->Month >= 1U) && (rtc->Month <= 12U) &&
            (rtc->Day >= 1U) && (rtc->Day <= 31U) &&
            (rtc->Hour <= 23U) &&
            (rtc->Minute <= 59U) &&
            (rtc->Second <= 59U);
    }
}

bool EpeverProtocol_Init(void)
{
    return ModbusProtocol_Init();
}

void EpeverProtocol_DeInit(void)
{
    ModbusProtocol_DeInit();
}

bool Epever_ReadRtc(MPPT_RTC_TYPE *rtc)
{
    if (rtc == nullptr)
        return false;

    uint16_t values[3] = {0};

    if (!Modbus_ReadHoldingRegisters(
            EpeverReg::RTC_SECONDS_MINUTES,
            3,
            values))
    {
        return false;
    }

    rtc->Second = (uint8_t)(values[0] & 0xFFU);
    rtc->Minute = (uint8_t)((values[0] >> 8) & 0xFFU);

    rtc->Hour = (uint8_t)(values[1] & 0xFFU);
    rtc->Day = (uint8_t)((values[1] >> 8) & 0xFFU);

    rtc->Month = (uint8_t)(values[2] & 0xFFU);
    rtc->Year = 2000U + (uint16_t)((values[2] >> 8) & 0xFFU);

    return rtcIsValid(rtc);
}

bool Epever_SetRtc(const MPPT_RTC_TYPE *rtc)
{
    if (!rtcIsValid(rtc))
        return false;

    uint16_t values[3];

    values[0] =
        (uint16_t)rtc->Second |
        ((uint16_t)rtc->Minute << 8);

    values[1] =
        (uint16_t)rtc->Hour |
        ((uint16_t)rtc->Day << 8);

    values[2] =
        (uint16_t)rtc->Month |
        ((uint16_t)(rtc->Year - 2000U) << 8);

    return Modbus_WriteHoldingRegisters(
        EpeverReg::RTC_SECONDS_MINUTES,
        values,
        3);
}

bool Epever_ReadSample(MPPT_SAMPLE_TYPE *sample)
{
    if (sample == nullptr)
        return false;

    uint16_t live[8] = {0};

    if (!Modbus_ReadInputRegisters(
            EpeverReg::PV_VOLTAGE,
            8,
            live))
    {
        return false;
    }

    sample->PvVoltageV = live[0] / 100.0f;
    sample->PvCurrentA = live[1] / 100.0f;
    sample->PvPowerW =
        combineLowHigh(live[2], live[3]) / 100.0f;

    sample->BatteryVoltageV = live[4] / 100.0f;
    sample->ChargeCurrentA = live[5] / 100.0f;
    sample->ChargePowerW =
        combineLowHigh(live[6], live[7]) / 100.0f;

    uint16_t soc[1] = {0};

    if (!Modbus_ReadInputRegisters(
            EpeverReg::BATTERY_SOC,
            1,
            soc))
    {
        return false;
    }

    sample->BatterySocPercent = soc[0];

    uint16_t status[2] = {0};

    if (!Modbus_ReadInputRegisters(
            EpeverReg::BATTERY_STATUS,
            2,
            status))
    {
        return false;
    }

    sample->BatteryStatusRaw = status[0];
    sample->ChargingStatusRaw = status[1];

    uint16_t energy[8] = {0};

    if (!Modbus_ReadInputRegisters(
            EpeverReg::ENERGY_TODAY_L,
            8,
            energy))
    {
        return false;
    }

    // EPEVER energy statistics use 0.01 kWh/count = 10 Wh/count.
    sample->EnergyTodayWh =
        (double)combineLowHigh(energy[0], energy[1]) * 10.0;

    sample->EnergyMonthWh =
        (double)combineLowHigh(energy[2], energy[3]) * 10.0;

    sample->EnergyYearWh =
        (double)combineLowHigh(energy[4], energy[5]) * 10.0;

    sample->EnergyTotalWh =
        (double)combineLowHigh(energy[6], energy[7]) * 10.0;

    return Epever_ReadRtc(&sample->Time);
}

bool Epever_ReadConfiguration(MPPT_CONFIG_TYPE *config)
{
    if (config == nullptr)
        return false;

    uint16_t mainBlock[15] = {0};

    if (!Modbus_ReadHoldingRegisters(
            EpeverReg::BATTERY_TYPE,
            15,
            mainBlock))
    {
        return false;
    }

    config->BatteryType = mainBlock[0];
    config->BatteryCapacityAh = mainBlock[1];

    config->TempCompensation = mainBlock[2] / 100.0f;

    config->HighVoltageDisconnectV = mainBlock[3] / 100.0f;
    config->ChargingLimitV = mainBlock[4] / 100.0f;
    config->OverVoltageReconnectV = mainBlock[5] / 100.0f;
    config->EqualizationV = mainBlock[6] / 100.0f;
    config->BoostV = mainBlock[7] / 100.0f;
    config->FloatV = mainBlock[8] / 100.0f;
    config->BoostReconnectV = mainBlock[9] / 100.0f;
    config->LowVoltageReconnectV = mainBlock[10] / 100.0f;
    config->UndervoltageRecoverV = mainBlock[11] / 100.0f;
    config->UndervoltageWarningV = mainBlock[12] / 100.0f;
    config->LowVoltageDisconnectV = mainBlock[13] / 100.0f;
    config->DischargeLimitV = mainBlock[14] / 100.0f;

    uint16_t durations[2] = {0};

    if (!Modbus_ReadHoldingRegisters(
            EpeverReg::EQUALIZATION_DURATION,
            2,
            durations))
    {
        return false;
    }

    config->EqualizationDurationMin = durations[0];
    config->BoostDurationMin = durations[1];

    return true;
}

bool Epever_ValidateConfiguration(
    const MPPT_CONFIG_TYPE *config,
    const char **errorText)
{
    static const char *okText = "OK";

    if (errorText != nullptr)
        *errorText = okText;

    if (config == nullptr)
    {
        if (errorText != nullptr)
            *errorText = "null configuration";
        return false;
    }

    if ((config->BatteryCapacityAh == 0U) ||
        (config->BatteryCapacityAh > 2000U))
    {
        if (errorText != nullptr)
            *errorText = "battery capacity is invalid";
        return false;
    }

    const float voltages[] =
    {
        config->HighVoltageDisconnectV,
        config->ChargingLimitV,
        config->OverVoltageReconnectV,
        config->EqualizationV,
        config->BoostV,
        config->FloatV,
        config->BoostReconnectV,
        config->LowVoltageReconnectV,
        config->UndervoltageRecoverV,
        config->UndervoltageWarningV,
        config->LowVoltageDisconnectV,
        config->DischargeLimitV
    };

    for (uint8_t i = 0; i < (sizeof(voltages) / sizeof(voltages[0])); ++i)
    {
        // Deliberately broad 12V-system sanity range. EPEVER itself applies
        // its own parameter relationship/range validation when written.
        if ((voltages[i] < 8.0f) || (voltages[i] > 18.0f))
        {
            if (errorText != nullptr)
                *errorText = "one or more battery voltage values are unset/out of range";
            return false;
        }
    }

    if ((config->EqualizationDurationMin > 600U) ||
        (config->BoostDurationMin > 600U))
    {
        if (errorText != nullptr)
            *errorText = "charge duration is out of range";
        return false;
    }

    return true;
}

bool Epever_WriteConfiguration(const MPPT_CONFIG_TYPE *config)
{
    const char *validationError = nullptr;

    if (!Epever_ValidateConfiguration(config, &validationError))
        return false;

    // Write battery type separately first. USER-defined configuration values
    // are then written as the contiguous 0x9001..0x900E block.
    if (!Modbus_WriteHoldingRegister(
            EpeverReg::BATTERY_TYPE,
            config->BatteryType))
    {
        return false;
    }

    delay(100);

    uint16_t mainBlock[14] =
    {
        config->BatteryCapacityAh,
        scale100(config->TempCompensation),

        scale100(config->HighVoltageDisconnectV),
        scale100(config->ChargingLimitV),
        scale100(config->OverVoltageReconnectV),
        scale100(config->EqualizationV),
        scale100(config->BoostV),
        scale100(config->FloatV),
        scale100(config->BoostReconnectV),
        scale100(config->LowVoltageReconnectV),
        scale100(config->UndervoltageRecoverV),
        scale100(config->UndervoltageWarningV),
        scale100(config->LowVoltageDisconnectV),
        scale100(config->DischargeLimitV)
    };

    if (!Modbus_WriteHoldingRegisters(
            EpeverReg::BATTERY_CAPACITY,
            mainBlock,
            14))
    {
        return false;
    }

    delay(100);

    const uint16_t durations[2] =
    {
        config->EqualizationDurationMin,
        config->BoostDurationMin
    };

    return Modbus_WriteHoldingRegisters(
        EpeverReg::EQUALIZATION_DURATION,
        durations,
        2);
}

bool Epever_VerifyConfiguration(const MPPT_CONFIG_TYPE *expected)
{
    if (expected == nullptr)
        return false;

    MPPT_CONFIG_TYPE actual = {};

    if (!Epever_ReadConfiguration(&actual))
        return false;

    return
        (actual.BatteryType == expected->BatteryType) &&
        (actual.BatteryCapacityAh == expected->BatteryCapacityAh) &&
        floatClose(actual.TempCompensation, expected->TempCompensation) &&

        floatClose(actual.HighVoltageDisconnectV, expected->HighVoltageDisconnectV) &&
        floatClose(actual.ChargingLimitV, expected->ChargingLimitV) &&
        floatClose(actual.OverVoltageReconnectV, expected->OverVoltageReconnectV) &&
        floatClose(actual.EqualizationV, expected->EqualizationV) &&
        floatClose(actual.BoostV, expected->BoostV) &&
        floatClose(actual.FloatV, expected->FloatV) &&
        floatClose(actual.BoostReconnectV, expected->BoostReconnectV) &&
        floatClose(actual.LowVoltageReconnectV, expected->LowVoltageReconnectV) &&
        floatClose(actual.UndervoltageRecoverV, expected->UndervoltageRecoverV) &&
        floatClose(actual.UndervoltageWarningV, expected->UndervoltageWarningV) &&
        floatClose(actual.LowVoltageDisconnectV, expected->LowVoltageDisconnectV) &&
        floatClose(actual.DischargeLimitV, expected->DischargeLimitV) &&

        (actual.EqualizationDurationMin == expected->EqualizationDurationMin) &&
        (actual.BoostDurationMin == expected->BoostDurationMin);
}

uint8_t Epever_GetLastModbusError(void)
{
    return Modbus_GetLastError();
}
