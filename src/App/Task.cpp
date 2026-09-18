//******************************************************************************
// File: Task.cpp
//******************************************************************************

#include "Task.h"

#include <Arduino.h>

#include "../user_config.h"
#include "Core.h"
#include "Store.h"

bool Task_ReadMppt(MPPT_SAMPLE_TYPE *sample)
{
    if (sample == nullptr)
        return false;

    if (!EpeverProtocol_Init())
    {
        Core_RecordModbusError();
        APP.LastMpptReadOk = false;
        return false;
    }

    if (!Epever_ReadSample(sample))
    {
        Core_RecordModbusError();
        APP.LastMpptReadOk = false;
        return false;
    }

    APP.LastSample = *sample;
    APP.LastMpptReadOk = true;

    return true;
}

void Task_PrintEnergy(const MPPT_SAMPLE_TYPE *sample)
{
    if (sample == nullptr)
        return;

    Serial.println("ENERGY STATISTICS");
    Serial.println("----------------------------------------");

    Serial.print("Today     : ");
    Serial.print(sample->EnergyTodayWh, 0);
    Serial.println(" Wh");

    Serial.print("This month: ");
    Serial.print(sample->EnergyMonthWh / 1000.0, 3);
    Serial.println(" kWh");

    Serial.print("This year : ");
    Serial.print(sample->EnergyYearWh / 1000.0, 3);
    Serial.println(" kWh");

    Serial.print("Lifetime  : ");
    Serial.print(sample->EnergyTotalWh / 1000.0, 3);
    Serial.println(" kWh");

    Serial.println("----------------------------------------");
}

void Task_PrintMpptSample(const MPPT_SAMPLE_TYPE *s)
{
    if (s == nullptr)
        return;

    Serial.println();
    Serial.println("MPPT SAMPLE");
    Serial.println("----------------------------------------");

    Serial.printf(
        "Time               : %04u-%02u-%02u %02u:%02u:%02u\n",
        s->Time.Year,
        s->Time.Month,
        s->Time.Day,
        s->Time.Hour,
        s->Time.Minute,
        s->Time.Second);

    Serial.print("PV voltage         : ");
    Serial.print(s->PvVoltageV, 2);
    Serial.println(" V");

    Serial.print("PV current         : ");
    Serial.print(s->PvCurrentA, 2);
    Serial.println(" A");

    Serial.print("PV power           : ");
    Serial.print(s->PvPowerW, 2);
    Serial.println(" W");

    Serial.print("Battery voltage    : ");
    Serial.print(s->BatteryVoltageV, 2);
    Serial.println(" V");

    Serial.print("Charge current     : ");
    Serial.print(s->ChargeCurrentA, 2);
    Serial.println(" A");

    Serial.print("Charge power       : ");
    Serial.print(s->ChargePowerW, 2);
    Serial.println(" W");

    Serial.print("Battery SOC        : ");
    Serial.print(s->BatterySocPercent);
    Serial.println(" %");

    Serial.print("Battery status raw : 0x");
    Serial.println(s->BatteryStatusRaw, HEX);

    Serial.print("Charge status raw  : 0x");
    Serial.println(s->ChargingStatusRaw, HEX);

    Serial.println("----------------------------------------");

    Task_PrintEnergy(s);
}

bool Task_RunNormalCycle(void)
{
    MPPT_SAMPLE_TYPE sample = {};

    const bool readOk = Task_ReadMppt(&sample);

    if (readOk)
    {
#if NORMAL_WAKE_SERIAL_ENABLE
        Task_PrintMpptSample(&sample);
#endif

        if (!Store_AddSample(&sample))
        {
#if NORMAL_WAKE_SERIAL_ENABLE
            Serial.println("WARNING: sample was not stored.");
#endif
        }
#if NORMAL_WAKE_SERIAL_ENABLE
        else
        {
            Serial.print("Buffered samples: ");
            Serial.println(Store_GetBufferedCount());
        }
#endif
    }
#if NORMAL_WAKE_SERIAL_ENABLE
    else
    {
        Serial.print("MPPT read failed. Modbus error: 0x");
        Serial.println(Epever_GetLastModbusError(), HEX);
    }
#endif

    EpeverProtocol_DeInit();

    return readOk;
}
