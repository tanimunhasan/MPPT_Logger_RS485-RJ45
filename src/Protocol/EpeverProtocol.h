//******************************************************************************
// File: EpeverProtocol.h
// Purpose: EPEVER-specific register interpretation above generic Modbus.
//******************************************************************************

#ifndef __EPEVER_PROTOCOL_H
#define __EPEVER_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint16_t Year;
    uint8_t Month;
    uint8_t Day;
    uint8_t Hour;
    uint8_t Minute;
    uint8_t Second;
} MPPT_RTC_TYPE;

typedef struct
{
    uint16_t BatteryType;
    uint16_t BatteryCapacityAh;

    float TempCompensation;

    float HighVoltageDisconnectV;
    float ChargingLimitV;
    float OverVoltageReconnectV;
    float EqualizationV;
    float BoostV;
    float FloatV;
    float BoostReconnectV;
    float LowVoltageReconnectV;
    float UndervoltageRecoverV;
    float UndervoltageWarningV;
    float LowVoltageDisconnectV;
    float DischargeLimitV;

    uint16_t EqualizationDurationMin;
    uint16_t BoostDurationMin;
} MPPT_CONFIG_TYPE;

typedef struct
{
    MPPT_RTC_TYPE Time;

    float PvVoltageV;
    float PvCurrentA;
    float PvPowerW;

    float BatteryVoltageV;
    float ChargeCurrentA;
    float ChargePowerW;

    uint16_t BatterySocPercent;
    uint16_t BatteryStatusRaw;
    uint16_t ChargingStatusRaw;

    // EPEVER generated-energy values converted to Wh.
    double EnergyTodayWh;
    double EnergyMonthWh;
    double EnergyYearWh;
    double EnergyTotalWh;
} MPPT_SAMPLE_TYPE;

bool EpeverProtocol_Init(void);
void EpeverProtocol_DeInit(void);

bool Epever_ReadSample(MPPT_SAMPLE_TYPE *sample);
bool Epever_ReadRtc(MPPT_RTC_TYPE *rtc);
bool Epever_SetRtc(const MPPT_RTC_TYPE *rtc);

bool Epever_ReadConfiguration(MPPT_CONFIG_TYPE *config);
bool Epever_WriteConfiguration(const MPPT_CONFIG_TYPE *config);
bool Epever_VerifyConfiguration(const MPPT_CONFIG_TYPE *expected);

bool Epever_ValidateConfiguration(
    const MPPT_CONFIG_TYPE *config,
    const char **errorText);

uint8_t Epever_GetLastModbusError(void);

#endif // __EPEVER_PROTOCOL_H
