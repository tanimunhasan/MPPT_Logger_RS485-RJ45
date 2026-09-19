//******************************************************************************
// File: Store.cpp
//******************************************************************************

#include "Store.h"

#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

#include "../user_config.h"
#include "../Hal/hal_sd.h"

namespace
{
    RTC_DATA_ATTR MPPT_SAMPLE_TYPE gSampleBuffer[LOGGER_SD_BUFFER_SAMPLES];
    RTC_DATA_ATTR uint8_t gSampleCount = 0;
    RTC_DATA_ATTR uint32_t gLostSamples = 0;

    uint32_t crc32(
        const uint8_t *data,
        size_t length)
    {
        uint32_t crc = 0xFFFFFFFFUL;

        for (size_t i = 0; i < length; ++i)
        {
            crc ^= data[i];

            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                const uint32_t mask =
                    (uint32_t)-(int32_t)(crc & 1U);

                crc =
                    (crc >> 1) ^
                    (0xEDB88320UL & mask);
            }
        }

        return ~crc;
    }

    uint32_t configCrc(const APP_CONFIG_TYPE &config)
    {
        APP_CONFIG_TYPE copy = config;
        copy.Crc32 = 0;

        return crc32(
            reinterpret_cast<const uint8_t*>(&copy),
            sizeof(copy));
    }

    void makeMonthlyPath(
        const MPPT_SAMPLE_TYPE &sample,
        char *outPath,
        size_t outLength)
    {
        snprintf(
            outPath,
            outLength,
            "/%04u-%02u.csv",
            sample.Time.Year,
            sample.Time.Month);
    }

    bool appendSample(
        const MPPT_SAMPLE_TYPE &s)
    {
        char path[20];
        makeMonthlyPath(s, path, sizeof(path));

        const bool exists = HAL_SD_Exists(path);

        File file = HAL_SD_OpenAppend(path);

        if (!file)
            return false;

        if (!exists)
        {
            file.println(
                "DateTime,"
                "PV_V,PV_A,PV_W,"
                "Battery_V,Charge_A,Charge_W,"
                "SOC_percent,"
                "BatteryStatus,ChargingStatus,"
                "EnergyToday_Wh,EnergyMonth_Wh,"
                "EnergyYear_Wh,EnergyTotal_Wh");
        }

        char timestamp[24];

        snprintf(
            timestamp,
            sizeof(timestamp),
            "%04u-%02u-%02u %02u:%02u:%02u",
            s.Time.Year,
            s.Time.Month,
            s.Time.Day,
            s.Time.Hour,
            s.Time.Minute,
            s.Time.Second);

        file.print(timestamp);
        file.print(',');

        file.print(s.PvVoltageV, 2);
        file.print(',');
        file.print(s.PvCurrentA, 2);
        file.print(',');
        file.print(s.PvPowerW, 2);
        file.print(',');

        file.print(s.BatteryVoltageV, 2);
        file.print(',');
        file.print(s.ChargeCurrentA, 2);
        file.print(',');
        file.print(s.ChargePowerW, 2);
        file.print(',');

        file.print(s.BatterySocPercent);
        file.print(',');
        file.print(s.BatteryStatusRaw);
        file.print(',');
        file.print(s.ChargingStatusRaw);
        file.print(',');

        file.print(s.EnergyTodayWh, 0);
        file.print(',');
        file.print(s.EnergyMonthWh, 0);
        file.print(',');
        file.print(s.EnergyYearWh, 0);
        file.print(',');
        file.println(s.EnergyTotalWh, 0);

        file.flush();
        file.close();

        return true;
    }
}

void Store_Init(bool timerWake)
{
    if (!timerWake)
    {
        gSampleCount = 0;
        gLostSamples = 0;
    }
}

bool Store_LoadConfiguration(APP_CONFIG_TYPE *config)
{
    if (config == nullptr)
        return false;

    Preferences prefs;

    if (!prefs.begin(NVS_NAMESPACE, true))
        return false;

    const size_t storedLength =
        prefs.getBytesLength(NVS_CONFIG_KEY);

    if (storedLength != sizeof(APP_CONFIG_TYPE))
    {
        prefs.end();
        return false;
    }

    APP_CONFIG_TYPE loaded = {};

    const size_t readLength =
        prefs.getBytes(
            NVS_CONFIG_KEY,
            &loaded,
            sizeof(loaded));

    prefs.end();

    if (readLength != sizeof(loaded))
        return false;

    if ((loaded.Magic != APP_CONFIG_MAGIC) ||
        (loaded.SchemaVersion != APP_CONFIG_SCHEMA_VERSION))
    {
        return false;
    }

    if (loaded.Crc32 != configCrc(loaded))
        return false;

    *config = loaded;
    return true;
}

bool Store_SaveConfiguration(APP_CONFIG_TYPE *config)
{
    if (config == nullptr)
        return false;

    config->Magic = APP_CONFIG_MAGIC;
    config->SchemaVersion = APP_CONFIG_SCHEMA_VERSION;
    config->Crc32 = 0;
    config->Crc32 = configCrc(*config);

    Preferences prefs;

    if (!prefs.begin(NVS_NAMESPACE, false))
        return false;

    const size_t written =
        prefs.putBytes(
            NVS_CONFIG_KEY,
            config,
            sizeof(APP_CONFIG_TYPE));

    prefs.end();

    return written == sizeof(APP_CONFIG_TYPE);
}

void Store_ClearConfiguration(void)
{
    Preferences prefs;

    if (!prefs.begin(NVS_NAMESPACE, false))
        return;

    prefs.remove(NVS_CONFIG_KEY);
    prefs.end();
}

bool Store_FlushSamples(void)
{
    if (gSampleCount == 0U)
        return true;

    if (!HAL_SD_Init())
    {
        Core_RecordSdError();
        APP.LastSdWriteOk = false;
        return false;
    }

    for (uint8_t i = 0; i < gSampleCount; ++i)
    {
        if (!appendSample(gSampleBuffer[i]))
        {
            HAL_SD_DeInit();
            Core_RecordSdError();
            APP.LastSdWriteOk = false;
            return false;
        }
    }

    HAL_SD_DeInit();

    gSampleCount = 0U;
    APP.LastSdWriteOk = true;
    return true;
}

bool Store_AddSample(const MPPT_SAMPLE_TYPE *sample)
{
    if (sample == nullptr)
        return false;

    // If the previous batch was full because an SD write failed, retry it
    // before accepting another sample.
    if (gSampleCount >= LOGGER_SD_BUFFER_SAMPLES)
    {
        if (!Store_FlushSamples())
        {
            gLostSamples++;
            return false;
        }
    }

    gSampleBuffer[gSampleCount++] = *sample;

    if (gSampleCount >= LOGGER_SD_BUFFER_SAMPLES)
        return Store_FlushSamples();

    return true;
}
bool Store_PrintMonthlyLog(uint16_t year, uint8_t month)
{
    MPPT_SAMPLE_TYPE sample = {};
    sample.Time.Year = year;
    sample.Time.Month = month;

    char path[20];
    makeMonthlyPath(sample, path, sizeof(path));

    File file = HAL_SD_OpenRead(path);

    if (!file)
    {
        HAL_SD_DeInit();
        return false;
    }

    Serial.print("SD LOG: ");
    Serial.println(path);
    Serial.println("----------------------------------------");

    while (file.available())
        Serial.write(file.read());

    file.close();
    HAL_SD_DeInit();

    Serial.println("----------------------------------------");
    return true;
}

uint8_t Store_GetBufferedCount(void)
{
    return gSampleCount;
}

uint32_t Store_GetLostSampleCount(void)
{
    return gLostSamples;
}
