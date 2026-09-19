//******************************************************************************
// File: ConfigStart.cpp
//******************************************************************************

#include "ConfigStart.h"

#include <Arduino.h>

#include "../user_config.h"
#include "../Hal/hal_power.h"
#include "../Protocol/ConsoleProtocol.h"
#include "../Protocol/EpeverProtocol.h"

#include "Store.h"
#include "Task.h"

void AnnounceStartUp(bool isFirstBootup)
{
    Serial.println();
    Serial.println("================================================");
    Serial.print(" ");
    Serial.println(FW_PRODUCT_NAME);
    Serial.println("================================================");

    Serial.print("Firmware          : ");
    Serial.print(FW_VERSION_MAJOR);
    Serial.print('.');
    Serial.print(FW_VERSION_MINOR);
    Serial.print('.');
    Serial.println(FW_VERSION_RELEASE);

    Serial.print("Boot type         : ");
    Serial.println(
        APP.TimerWake ? "DEEP SLEEP WAKE" : "COLD / RESET");

    Serial.print("Reset reason      : ");
    Serial.println(HAL_Power_GetResetReasonString());

    Serial.print("Wake reason       : ");
    Serial.println(HAL_Power_GetWakeReasonString());

    Serial.print("First boot        : ");
    Serial.println(isFirstBootup ? "YES" : "NO");

    Serial.print("MPPT configured   : ");
    Serial.println(APP_CONF.MpptConfigured ? "YES" : "NO");

    Serial.print("Wake count        : ");
    Serial.println(APP.WakeCount);

    PrintOperationMode(GetOperationMode());

    Serial.println("================================================");
}

OPERATION_MODE_TYPE_ENUM run_startup(bool isFirstBootup)
{
    Console_Init();

#if NORMAL_WAKE_SERIAL_ENABLE
    AnnounceStartUp(isFirstBootup);
#else
    if (!APP.TimerWake)
        AnnounceStartUp(isFirstBootup);
#endif

    // First-ever flash / incomplete commissioning always enters CONFIG and
    // remains awake for the operator.
    if (isFirstBootup || !APP_CONF.MpptConfigured)
    {
        SetOperationMode(OPM_CONFIG, false);

        Serial.println();
        Serial.println("Commissioning required.");
        Serial.println("Type '?' for the command menu.");
    }
    // A timer wake must remain very short to minimize logger energy.
    // On a cold reset/power cycle after commissioning, give the operator a
    // short escape window to enter CONFIG mode.
    else if (!APP.TimerWake &&
             (GetOperationMode() == OPM_NORMAL))
    {
        if (Console_WaitForStartupRequest(
                STARTUP_CONSOLE_WINDOW_MS))
        {
            SetOperationMode(OPM_CONFIG, false);
        }
    }

    while (true)
    {
        const OPERATION_MODE_TYPE_ENUM mode =
            GetOperationMode();

        switch (mode)
        {
            case OPM_CONFIG:
                run_config_mode();
                break;

            case OPM_TEST:
                run_test_mode();
                break;

            case OPM_NORMAL:
                run_normal_mode();
                break;

            case OPM_HIBERNATE:
                run_hibernate_mode();
                break;

            case OPM_ERROR_START:
                run_error_mode();
                break;

            case OPM_RESET:
            default:
                HAL_Power_Reset();
                break;
        }
    }

    return GetOperationMode();
}

void run_config_mode(void)
{
    Serial.println();
    Serial.println("@01>> CONFIGURATION MODE");
    Serial.println("Type '?' for command menu.");
    Serial.print("> ");

    // Keep RS485 available in CONFIG mode. Public EPEVER calls are still
    // idempotent if this initialisation fails and is retried later.
    if (!EpeverProtocol_Init())
    {
        Serial.println();
        Serial.print("WARNING: initial RS485/Modbus init failed, error 0x");
        Serial.println(Epever_GetLastModbusError(), HEX);
        Serial.print("> ");
    }

    while (GetOperationMode() == OPM_CONFIG)
    {
        Console_Process();
        delay(5);
    }

    EpeverProtocol_DeInit();
}

void run_test_mode(void)
{
    Serial.println();
    Serial.println("@02>> TEST MODE");
    Serial.println("Live MPPT readout + SD logger test; MCU remains awake.");
    Serial.println("A successful six-sample batch is flushed to the monthly CSV.");
    Serial.println("Use R=1 for CONFIG or R=3 for NORMAL.");
    Serial.print("> ");

    EpeverProtocol_Init();

    uint32_t nextSampleMs = 0;

    while (GetOperationMode() == OPM_TEST)
    {
        Console_Process();

        const uint32_t now = millis();

        if ((int32_t)(now - nextSampleMs) >= 0)
        {
            nextSampleMs = now + TEST_SAMPLE_INTERVAL_MS;

            MPPT_SAMPLE_TYPE sample = {};

            if (Task_ReadMppt(&sample))
            {
                Task_PrintMpptSample(&sample);

                const uint8_t bufferedBefore = Store_GetBufferedCount();

                if (!Store_AddSample(&sample))
                {
                    Serial.println("TEST SD store: FAIL");
                }
                else if ((bufferedBefore > 0U) &&
                         (Store_GetBufferedCount() == 0U))
                {
                    Serial.println("TEST SD flush: PASS");
                }
                else
                {
                    Serial.print("TEST samples buffered: ");
                    Serial.print(Store_GetBufferedCount());
                    Serial.print('/');
                    Serial.println(LOGGER_SD_BUFFER_SAMPLES);
                }
            }
            else
            {
                Serial.print("TEST read failed. Modbus error 0x");
                Serial.println(Epever_GetLastModbusError(), HEX);
            }

            Serial.print("> ");
        }

        delay(5);
    }

    EpeverProtocol_DeInit();
}

[[noreturn]] void run_normal_mode(void)
{
#if NORMAL_WAKE_SERIAL_ENABLE
    Serial.println();
    Serial.println("@03>> NORMAL LOGGER MODE");
#endif

    Task_RunNormalCycle();

#if NORMAL_WAKE_SERIAL_ENABLE
    Serial.print("Entering deep sleep for ");
    Serial.print(APP_CONF.SampleIntervalSec);
    Serial.println(" seconds.");
#endif

    EpeverProtocol_DeInit();

    HAL_Power_DeepSleep(APP_CONF.SampleIntervalSec);
}

[[noreturn]] void run_hibernate_mode(void)
{
    Serial.println();
    Serial.println("@04>> HIBERNATE MODE");
    Serial.println("Flushing pending SD samples...");

    Store_FlushSamples();
    EpeverProtocol_DeInit();

    Serial.println(
        "Entering deep sleep with no timer wake. Reset/power-cycle to resume.");

    HAL_Power_Hibernate();
}

[[noreturn]] void run_error_mode(void)
{
    Serial.println();
    Serial.println("@05>> ERROR START MODE");

    EpeverProtocol_DeInit();

    Serial.print("Retrying after ");
    Serial.print(ERROR_RETRY_SLEEP_SEC);
    Serial.println(" seconds.");

    HAL_Power_DeepSleep(ERROR_RETRY_SLEEP_SEC);
}
