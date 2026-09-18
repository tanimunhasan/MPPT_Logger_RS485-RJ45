//******************************************************************************
// File: Core.cpp
//******************************************************************************

#include "Core.h"

#include <string.h>

#include "../user_config.h"
#include "../Hal/hal_power.h"
#include "ConfigStart.h"
#include "Store.h"

//******************************************************************************
// Global variables
//******************************************************************************

APP_RUNTIME_TYPE APP = {};
APP_CONFIG_TYPE APP_CONF = {};

namespace
{
    typedef struct
    {
        uint32_t Magic;
        uint32_t WakeCount;
        uint32_t ModbusErrorCount;
        uint32_t SdErrorCount;
    } APP_RTC_STATE_TYPE;

    static constexpr uint32_t RTC_STATE_MAGIC = 0x52544353UL; // "RTCS"

    RTC_DATA_ATTR APP_RTC_STATE_TYPE gRtcState = {};

    void initialiseRtcState(bool timerWake)
    {
        if ((!timerWake) || (gRtcState.Magic != RTC_STATE_MAGIC))
        {
            memset(&gRtcState, 0, sizeof(gRtcState));
            gRtcState.Magic = RTC_STATE_MAGIC;
        }

        gRtcState.WakeCount++;

        APP.WakeCount = gRtcState.WakeCount;
        APP.ModbusErrorCount = gRtcState.ModbusErrorCount;
        APP.SdErrorCount = gRtcState.SdErrorCount;
    }
}

void Core_LoadDefaultConfiguration(APP_CONFIG_TYPE *config)
{
    if (config == nullptr)
        return;

    memset(config, 0, sizeof(APP_CONFIG_TYPE));

    config->Magic = APP_CONFIG_MAGIC;
    config->SchemaVersion = APP_CONFIG_SCHEMA_VERSION;

    config->RunMode = OPM_CONFIG;
    config->SampleIntervalSec = LOGGER_SAMPLE_INTERVAL_SEC;

    config->MpptConfigured = false;
    config->MpptConfigRevision = 0;

    config->Mppt.BatteryType = DEF_BATTERY_TYPE;
    config->Mppt.BatteryCapacityAh = DEF_BATTERY_CAPACITY_AH;

    config->Mppt.TempCompensation = DEF_TEMP_COMPENSATION;

    config->Mppt.HighVoltageDisconnectV = DEF_HIGH_VOLTAGE_DISCONNECT_V;
    config->Mppt.ChargingLimitV = DEF_CHARGING_LIMIT_V;
    config->Mppt.OverVoltageReconnectV = DEF_OVER_VOLTAGE_RECONNECT_V;
    config->Mppt.EqualizationV = DEF_EQUALIZATION_V;
    config->Mppt.BoostV = DEF_BOOST_V;
    config->Mppt.FloatV = DEF_FLOAT_V;
    config->Mppt.BoostReconnectV = DEF_BOOST_RECONNECT_V;
    config->Mppt.LowVoltageReconnectV = DEF_LOW_VOLTAGE_RECONNECT_V;
    config->Mppt.UndervoltageRecoverV = DEF_UNDERVOLTAGE_RECOVER_V;
    config->Mppt.UndervoltageWarningV = DEF_UNDERVOLTAGE_WARNING_V;
    config->Mppt.LowVoltageDisconnectV = DEF_LOW_VOLTAGE_DISCONNECT_V;
    config->Mppt.DischargeLimitV = DEF_DISCHARGE_LIMIT_V;

    config->Mppt.EqualizationDurationMin =
        DEF_EQUALIZATION_DURATION_MIN;

    config->Mppt.BoostDurationMin =
        DEF_BOOST_DURATION_MIN;

    config->Crc32 = 0;
}

void Core_Init(void)
{
    memset(&APP, 0, sizeof(APP));

    APP.TimerWake = HAL_Power_IsTimerWake();

#if NORMAL_WAKE_SERIAL_ENABLE
    Serial.begin(DEBUG_BAUD_RATE);
    delay(150);
#else
    // Timer wakeups are intentionally silent in deployed NORMAL mode.
    // Cold boots/resets keep the console available.
    if (!APP.TimerWake)
    {
        Serial.begin(DEBUG_BAUD_RATE);
        delay(150);
    }
#endif

    HAL_Power_Init();

    initialiseRtcState(APP.TimerWake);

    Store_Init(APP.TimerWake);

    if (!Store_LoadConfiguration(&APP_CONF))
    {
        APP.FirstBoot = true;
        Core_LoadDefaultConfiguration(&APP_CONF);
        APP.RunMode = OPM_CONFIG;
    }
    else
    {
        APP.FirstBoot = false;

        if (!APP_CONF.MpptConfigured)
        {
            // A stored but incomplete commissioning state always returns to
            // CONFIG rather than allowing unattended NORMAL mode.
            APP.RunMode = OPM_CONFIG;
        }
        else
        {
            APP.RunMode = APP_CONF.RunMode;
        }
    }

    APP.ConfigDirty = false;
    APP.LastMpptReadOk = false;
    APP.LastSdWriteOk = true;
}

void Core_Run(void)
{
    run_startup(APP.FirstBoot);
}

OPERATION_MODE_TYPE_ENUM GetOperationMode(void)
{
    return APP.RunMode;
}

bool SetOperationMode(
    OPERATION_MODE_TYPE_ENUM mode,
    bool persistMode)
{
    switch (mode)
    {
        case OPM_ERROR_START:
        case OPM_CONFIG:
        case OPM_TEST:
        case OPM_NORMAL:
        case OPM_HIBERNATE:
        case OPM_RESET:
            APP.RunMode = mode;
            break;

        default:
            return false;
    }

    if (persistMode)
    {
        APP_CONF.RunMode = mode;
        return Store_SaveConfiguration(&APP_CONF);
    }

    return true;
}

const char* GetOperationModeName(
    OPERATION_MODE_TYPE_ENUM mode)
{
    switch (mode)
    {
        case OPM_ERROR_START: return "ERROR";
        case OPM_CONFIG:      return "CONFIG";
        case OPM_TEST:        return "TEST";
        case OPM_NORMAL:      return "NORMAL";
        case OPM_HIBERNATE:   return "HIBERNATE";
        case OPM_RESET:       return "RESET";
        default:              return "UNKNOWN";
    }
}

void PrintOperationMode(
    OPERATION_MODE_TYPE_ENUM mode)
{
    Serial.print("Operation Mode = ");
    Serial.print((int)mode);
    Serial.print(" (");
    Serial.print(GetOperationModeName(mode));
    Serial.println(")");
}

void Core_MarkConfigDirty(bool dirty)
{
    APP.ConfigDirty = dirty;
}

void Core_RecordModbusError(void)
{
    gRtcState.ModbusErrorCount++;
    APP.ModbusErrorCount = gRtcState.ModbusErrorCount;
}

void Core_RecordSdError(void)
{
    gRtcState.SdErrorCount++;
    APP.SdErrorCount = gRtcState.SdErrorCount;
}

void Core_PrintSystemStatus(void)
{
    Serial.println();
    Serial.println("SYSTEM STATUS");
    Serial.println("----------------------------------------");

    Serial.print("Firmware             : ");
    Serial.print(FW_VERSION_MAJOR);
    Serial.print('.');
    Serial.print(FW_VERSION_MINOR);
    Serial.print('.');
    Serial.println(FW_VERSION_RELEASE);

    Serial.print("Reset reason         : ");
    Serial.println(HAL_Power_GetResetReasonString());

    Serial.print("Wake reason          : ");
    Serial.println(HAL_Power_GetWakeReasonString());

    Serial.print("First boot           : ");
    Serial.println(APP.FirstBoot ? "YES" : "NO");

    Serial.print("Runtime mode         : ");
    Serial.println(GetOperationModeName(APP.RunMode));

    Serial.print("Saved deploy mode    : ");
    Serial.println(GetOperationModeName(APP_CONF.RunMode));

    Serial.print("MPPT configured      : ");
    Serial.println(APP_CONF.MpptConfigured ? "YES" : "NO");

    Serial.print("MPPT config revision : ");
    Serial.println(APP_CONF.MpptConfigRevision);

    Serial.print("Config dirty         : ");
    Serial.println(APP.ConfigDirty ? "YES" : "NO");

    Serial.print("Sample interval      : ");
    Serial.print(APP_CONF.SampleIntervalSec);
    Serial.println(" s");

    Serial.print("Wake count           : ");
    Serial.println(APP.WakeCount);

    Serial.print("Modbus errors        : ");
    Serial.println(APP.ModbusErrorCount);

    Serial.print("SD errors            : ");
    Serial.println(APP.SdErrorCount);

    Serial.print("Buffered samples     : ");
    Serial.println(Store_GetBufferedCount());

    Serial.println("----------------------------------------");
}

void Core_PrintLocalConfiguration(void)
{
    const MPPT_CONFIG_TYPE &c = APP_CONF.Mppt;

    Serial.println();
    Serial.println("LOCAL MPPT CONFIGURATION");
    Serial.println("----------------------------------------");

    Serial.print("Battery type         : ");
    Serial.println(c.BatteryType);

    Serial.print("Battery capacity     : ");
    Serial.print(c.BatteryCapacityAh);
    Serial.println(" Ah");

    Serial.print("Temp compensation    : ");
    Serial.println(c.TempCompensation, 2);

    Serial.print("High V disconnect    : ");
    Serial.print(c.HighVoltageDisconnectV, 2);
    Serial.println(" V");

    Serial.print("Charging limit       : ");
    Serial.print(c.ChargingLimitV, 2);
    Serial.println(" V");

    Serial.print("Over V reconnect     : ");
    Serial.print(c.OverVoltageReconnectV, 2);
    Serial.println(" V");

    Serial.print("Equalization         : ");
    Serial.print(c.EqualizationV, 2);
    Serial.println(" V");

    Serial.print("Boost                : ");
    Serial.print(c.BoostV, 2);
    Serial.println(" V");

    Serial.print("Float                : ");
    Serial.print(c.FloatV, 2);
    Serial.println(" V");

    Serial.print("Boost reconnect      : ");
    Serial.print(c.BoostReconnectV, 2);
    Serial.println(" V");

    Serial.print("Low V reconnect      : ");
    Serial.print(c.LowVoltageReconnectV, 2);
    Serial.println(" V");

    Serial.print("Undervolt recover    : ");
    Serial.print(c.UndervoltageRecoverV, 2);
    Serial.println(" V");

    Serial.print("Undervolt warning    : ");
    Serial.print(c.UndervoltageWarningV, 2);
    Serial.println(" V");

    Serial.print("Low V disconnect     : ");
    Serial.print(c.LowVoltageDisconnectV, 2);
    Serial.println(" V");

    Serial.print("Discharge limit      : ");
    Serial.print(c.DischargeLimitV, 2);
    Serial.println(" V");

    Serial.print("Equalization time    : ");
    Serial.print(c.EqualizationDurationMin);
    Serial.println(" min");

    Serial.print("Boost time           : ");
    Serial.print(c.BoostDurationMin);
    Serial.println(" min");

    Serial.print("Sample interval      : ");
    Serial.print(APP_CONF.SampleIntervalSec);
    Serial.println(" s");

    Serial.print("MPPT configured      : ");
    Serial.println(APP_CONF.MpptConfigured ? "YES" : "NO");

    Serial.print("Pending changes      : ");
    Serial.println(APP.ConfigDirty ? "YES" : "NO");

    Serial.println("----------------------------------------");
}
