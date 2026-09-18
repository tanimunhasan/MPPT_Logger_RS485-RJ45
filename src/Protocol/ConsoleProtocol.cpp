//******************************************************************************
// File: ConsoleProtocol.cpp
//******************************************************************************

#include "ConsoleProtocol.h"

#include <Arduino.h>

#include "../App/Core.h"
#include "../App/Store.h"
#include "../App/Task.h"
#include "../Hal/hal_power.h"
#include "EpeverProtocol.h"

namespace
{
    static const size_t CONSOLE_BUFFER_LEN = 96;
    char gLine[CONSOLE_BUFFER_LEN] = {0};
    size_t gLineIndex = 0;

    String upperTrimmed(String text)
    {
        text.trim();
        text.toUpperCase();
        return text;
    }

    void printRtc(const MPPT_RTC_TYPE &rtc)
    {
        Serial.printf(
            "%04u-%02u-%02u %02u:%02u:%02u\n",
            rtc.Year,
            rtc.Month,
            rtc.Day,
            rtc.Hour,
            rtc.Minute,
            rtc.Second);
    }

    void printMpptConfig(const MPPT_CONFIG_TYPE &c)
    {
        Serial.println();
        Serial.println("EPEVER MPPT CONFIGURATION");
        Serial.println("----------------------------------------");

        Serial.print("Battery type         : "); Serial.println(c.BatteryType);
        Serial.print("Battery capacity     : "); Serial.print(c.BatteryCapacityAh); Serial.println(" Ah");
        Serial.print("Temp compensation    : "); Serial.println(c.TempCompensation, 2);

        Serial.print("High V disconnect    : "); Serial.print(c.HighVoltageDisconnectV, 2); Serial.println(" V");
        Serial.print("Charging limit       : "); Serial.print(c.ChargingLimitV, 2); Serial.println(" V");
        Serial.print("Over V reconnect     : "); Serial.print(c.OverVoltageReconnectV, 2); Serial.println(" V");
        Serial.print("Equalization         : "); Serial.print(c.EqualizationV, 2); Serial.println(" V");
        Serial.print("Boost                : "); Serial.print(c.BoostV, 2); Serial.println(" V");
        Serial.print("Float                : "); Serial.print(c.FloatV, 2); Serial.println(" V");
        Serial.print("Boost reconnect      : "); Serial.print(c.BoostReconnectV, 2); Serial.println(" V");
        Serial.print("Low V reconnect      : "); Serial.print(c.LowVoltageReconnectV, 2); Serial.println(" V");
        Serial.print("Undervolt recover    : "); Serial.print(c.UndervoltageRecoverV, 2); Serial.println(" V");
        Serial.print("Undervolt warning    : "); Serial.print(c.UndervoltageWarningV, 2); Serial.println(" V");
        Serial.print("Low V disconnect     : "); Serial.print(c.LowVoltageDisconnectV, 2); Serial.println(" V");
        Serial.print("Discharge limit      : "); Serial.print(c.DischargeLimitV, 2); Serial.println(" V");

        Serial.print("Equalization time    : "); Serial.print(c.EqualizationDurationMin); Serial.println(" min");
        Serial.print("Boost time           : "); Serial.print(c.BoostDurationMin); Serial.println(" min");

        Serial.println("----------------------------------------");
    }

    bool parseRtc(const String &line, MPPT_RTC_TYPE &rtc)
    {
        int year, month, day;
        int hour, minute, second;

        const int matched = sscanf(
            line.c_str(),
            "ST=%d-%d-%d %d:%d:%d",
            &year,
            &month,
            &day,
            &hour,
            &minute,
            &second);

        if (matched != 6)
            return false;

        if ((year < 2000) || (year > 2099) ||
            (month < 1) || (month > 12) ||
            (day < 1) || (day > 31) ||
            (hour < 0) || (hour > 23) ||
            (minute < 0) || (minute > 59) ||
            (second < 0) || (second > 59))
        {
            return false;
        }

        rtc.Year = (uint16_t)year;
        rtc.Month = (uint8_t)month;
        rtc.Day = (uint8_t)day;
        rtc.Hour = (uint8_t)hour;
        rtc.Minute = (uint8_t)minute;
        rtc.Second = (uint8_t)second;

        return true;
    }

    void markMpptConfigChanged(void)
    {
        APP_CONF.MpptConfigured = false;
        Core_MarkConfigDirty(true);
    }

    bool setConfigValue(
        const String &keyUpper,
        const String &valueText)
    {
        MPPT_CONFIG_TYPE &c = APP_CONF.Mppt;

        if (keyUpper == "TYPE")
        {
            c.BatteryType = (uint16_t)valueText.toInt();
            markMpptConfigChanged();
            return true;
        }

        if ((keyUpper == "CAP") || (keyUpper == "CAPACITY"))
        {
            c.BatteryCapacityAh = (uint16_t)valueText.toInt();
            markMpptConfigChanged();
            return true;
        }

        if (keyUpper == "TEMPCOMP")
        {
            c.TempCompensation = valueText.toFloat();
            markMpptConfigChanged();
            return true;
        }

        if (keyUpper == "HVD")
            c.HighVoltageDisconnectV = valueText.toFloat();
        else if (keyUpper == "CLV")
            c.ChargingLimitV = valueText.toFloat();
        else if (keyUpper == "OVR")
            c.OverVoltageReconnectV = valueText.toFloat();
        else if (keyUpper == "EQ")
            c.EqualizationV = valueText.toFloat();
        else if (keyUpper == "BOOST")
            c.BoostV = valueText.toFloat();
        else if (keyUpper == "FLOAT")
            c.FloatV = valueText.toFloat();
        else if (keyUpper == "BRV")
            c.BoostReconnectV = valueText.toFloat();
        else if (keyUpper == "LVR")
            c.LowVoltageReconnectV = valueText.toFloat();
        else if (keyUpper == "UVR")
            c.UndervoltageRecoverV = valueText.toFloat();
        else if (keyUpper == "UVW")
            c.UndervoltageWarningV = valueText.toFloat();
        else if (keyUpper == "LVD")
            c.LowVoltageDisconnectV = valueText.toFloat();
        else if (keyUpper == "DLV")
            c.DischargeLimitV = valueText.toFloat();
        else if (keyUpper == "EQMIN")
            c.EqualizationDurationMin = (uint16_t)valueText.toInt();
        else if (keyUpper == "BOOSTMIN")
            c.BoostDurationMin = (uint16_t)valueText.toInt();
        else if (keyUpper == "INTERVAL")
        {
            const long seconds = valueText.toInt();

            if ((seconds < 10L) || (seconds > 86400L))
                return false;

            APP_CONF.SampleIntervalSec = (uint32_t)seconds;
            Core_MarkConfigDirty(true);
            return true;
        }
        else
        {
            return false;
        }

        markMpptConfigChanged();
        return true;
    }

    void processSetCommand(const String &line)
    {
        // Format: SET KEY=value
        const int firstSpace = line.indexOf(' ');
        const int equals = line.indexOf('=');

        if ((firstSpace < 0) || (equals <= firstSpace))
        {
            Serial.println("Invalid SET format. Type ? for examples.");
            return;
        }

        String key = line.substring(firstSpace + 1, equals);
        String value = line.substring(equals + 1);

        key.trim();
        key.toUpperCase();
        value.trim();

        if (!setConfigValue(key, value))
        {
            Serial.println("Unknown/invalid SET parameter.");
            return;
        }

        Serial.print("Updated local value: ");
        Serial.print(key);
        Serial.print('=');
        Serial.println(value);

        Serial.println("Use SC to inspect. Use WC to write MPPT configuration.");
    }

    void processModeCommand(int modeValue)
    {
        const OPERATION_MODE_TYPE_ENUM mode =
            (OPERATION_MODE_TYPE_ENUM)modeValue;

        switch (mode)
        {
            case OPM_CONFIG:
                // Maintenance mode is temporary; saved deploy mode is unchanged.
                SetOperationMode(OPM_CONFIG, false);
                PrintOperationMode(GetOperationMode());
                break;

            case OPM_TEST:
                // TEST is deliberately temporary so a later power cycle cannot
                // accidentally leave a deployed logger running continuously.
                SetOperationMode(OPM_TEST, false);
                PrintOperationMode(GetOperationMode());
                break;

            case OPM_NORMAL:
                if (!APP_CONF.MpptConfigured)
                {
                    Serial.println(
                        "NORMAL rejected: MPPT configuration is not verified.");
                    Serial.println(
                        "Configure/write/verify with WC first.");
                    return;
                }

                if (APP.ConfigDirty)
                {
                    Serial.println(
                        "NORMAL rejected: local configuration has unsaved changes.");
                    return;
                }

                if (!SetOperationMode(OPM_NORMAL, true))
                {
                    Serial.println("Failed to save NORMAL mode.");
                    return;
                }

                PrintOperationMode(GetOperationMode());
                break;

            case OPM_HIBERNATE:
                if (!SetOperationMode(OPM_HIBERNATE, true))
                {
                    Serial.println("Failed to save HIBERNATE mode.");
                    return;
                }

                PrintOperationMode(GetOperationMode());
                break;

            case OPM_RESET:
                Serial.println("Resetting...");
                HAL_Power_Reset();
                break;

            default:
                Serial.println("Unsupported mode. Use R=1,2,3,4,9.");
                break;
        }
    }

    void processCommand(String line)
    {
        line.trim();

        if (line.length() == 0)
            return;

        String upper = line;
        upper.toUpperCase();

        if (upper == "?")
        {
            Console_PrintHelp();
            return;
        }

        if (upper.startsWith("R="))
        {
            processModeCommand(
                upper.substring(2).toInt());
            return;
        }

        if (upper == "S")
        {
            Core_PrintSystemStatus();
            return;
        }

        if (upper == "SC")
        {
            Core_PrintLocalConfiguration();
            return;
        }

        if (upper == "SAVE")
        {
            if (Store_SaveConfiguration(&APP_CONF))
            {
                Core_MarkConfigDirty(false);
                Serial.println("Local configuration saved to NVS.");
            }
            else
            {
                Serial.println("NVS save failed.");
            }
            return;
        }

        if (upper == "FACTORY")
        {
            Serial.println("Clearing local NVS configuration...");
            Store_ClearConfiguration();
            delay(50);
            HAL_Power_Reset();
        }

        if (upper.startsWith("SET "))
        {
            processSetCommand(line);
            return;
        }

        if (upper == "RC")
        {
            MPPT_CONFIG_TYPE config = {};

            if (Epever_ReadConfiguration(&config))
                printMpptConfig(config);
            else
            {
                Serial.print("RC failed. Modbus error 0x");
                Serial.println(Epever_GetLastModbusError(), HEX);
            }

            return;
        }

        if (upper == "WC")
        {
            const char *validationError = nullptr;

            if (!Epever_ValidateConfiguration(
                    &APP_CONF.Mppt,
                    &validationError))
            {
                Serial.print("Configuration invalid: ");
                Serial.println(validationError);
                return;
            }

            Serial.println("Writing configuration to EPEVER...");

            if (!Epever_WriteConfiguration(&APP_CONF.Mppt))
            {
                Serial.print("Write failed. Modbus error 0x");
                Serial.println(Epever_GetLastModbusError(), HEX);
                return;
            }

            Serial.println("Write complete. Verifying...");

            if (!Epever_VerifyConfiguration(&APP_CONF.Mppt))
            {
                Serial.println("VERIFY FAILED. Local configuration NOT marked complete.");
                APP_CONF.MpptConfigured = false;
                return;
            }

            APP_CONF.MpptConfigured = true;
            APP_CONF.MpptConfigRevision++;

            Core_MarkConfigDirty(false);

            if (!Store_SaveConfiguration(&APP_CONF))
            {
                Serial.println("MPPT verified, but NVS save FAILED.");
                return;
            }

            Serial.println("Configuration VERIFIED and saved.");
            Serial.print("MPPT config revision: ");
            Serial.println(APP_CONF.MpptConfigRevision);

            return;
        }

        if (upper == "VC")
        {
            if (Epever_VerifyConfiguration(&APP_CONF.Mppt))
                Serial.println("VERIFY: PASS");
            else
                Serial.println("VERIFY: FAIL");

            return;
        }

        if (upper == "RT")
        {
            MPPT_RTC_TYPE rtc = {};

            if (Epever_ReadRtc(&rtc))
            {
                Serial.print("EPEVER RTC: ");
                printRtc(rtc);
            }
            else
            {
                Serial.print("RT failed. Modbus error 0x");
                Serial.println(Epever_GetLastModbusError(), HEX);
            }

            return;
        }

        if (upper.startsWith("ST="))
        {
            MPPT_RTC_TYPE rtc = {};

            // sscanf format uses uppercase ST but only digits after it, so use
            // the uppercased command for consistent parsing.
            if (!parseRtc(upper, rtc))
            {
                Serial.println(
                    "Invalid time. Use ST=YYYY-MM-DD HH:MM:SS");
                return;
            }

            if (!Epever_SetRtc(&rtc))
            {
                Serial.print("ST failed. Modbus error 0x");
                Serial.println(Epever_GetLastModbusError(), HEX);
                return;
            }

            MPPT_RTC_TYPE verify = {};

            if (!Epever_ReadRtc(&verify))
            {
                Serial.println("RTC written, but readback failed.");
                return;
            }

            Serial.print("EPEVER RTC set: ");
            printRtc(verify);
            return;
        }

        if (upper == "RD")
        {
            MPPT_SAMPLE_TYPE sample = {};

            if (Task_ReadMppt(&sample))
                Task_PrintMpptSample(&sample);
            else
            {
                Serial.print("RD failed. Modbus error 0x");
                Serial.println(Epever_GetLastModbusError(), HEX);
            }

            return;
        }

        if (upper == "RE")
        {
            MPPT_SAMPLE_TYPE sample = {};

            if (Task_ReadMppt(&sample))
                Task_PrintEnergy(&sample);
            else
            {
                Serial.print("RE failed. Modbus error 0x");
                Serial.println(Epever_GetLastModbusError(), HEX);
            }

            return;
        }

        Serial.println("Unknown command. Type ? for help.");
    }
}

void Console_Init(void)
{
    gLineIndex = 0;
    memset(gLine, 0, sizeof(gLine));
}

void Console_PrintHelp(void)
{
    Serial.println();
    Serial.println("================ COMMAND MENU =================");
    Serial.println();
    Serial.println("OPERATING MODES");
    Serial.println("  R=1             CONFIG mode");
    Serial.println("  R=2             TEST mode");
    Serial.println("  R=3             NORMAL logger mode");
    Serial.println("  R=4             HIBERNATE");
    Serial.println("  R=9             Reset device");
    Serial.println();
    Serial.println("CONTROLLER / LOGGER");
    Serial.println("  S               System status");
    Serial.println("  SC              Show local configuration");
    Serial.println("  RC              Read configuration from EPEVER");
    Serial.println("  WC              Write local config -> EPEVER, verify, save");
    Serial.println("  VC              Verify EPEVER matches local config");
    Serial.println("  RD              Read live MPPT data");
    Serial.println("  RE              Read generated-energy statistics");
    Serial.println();
    Serial.println("CLOCK");
    Serial.println("  RT              Read EPEVER RTC");
    Serial.println("  ST=YYYY-MM-DD HH:MM:SS");
    Serial.println();
    Serial.println("LOCAL CONFIG EDITING");
    Serial.println("  SET TYPE=0");
    Serial.println("  SET CAP=70");
    Serial.println("  SET TEMPCOMP=0");
    Serial.println("  SET HVD=xx.xx");
    Serial.println("  SET CLV=xx.xx");
    Serial.println("  SET OVR=xx.xx");
    Serial.println("  SET EQ=xx.xx");
    Serial.println("  SET BOOST=xx.xx");
    Serial.println("  SET FLOAT=xx.xx");
    Serial.println("  SET BRV=xx.xx");
    Serial.println("  SET LVR=xx.xx");
    Serial.println("  SET UVR=xx.xx");
    Serial.println("  SET UVW=xx.xx");
    Serial.println("  SET LVD=xx.xx");
    Serial.println("  SET DLV=xx.xx");
    Serial.println("  SET EQMIN=0");
    Serial.println("  SET BOOSTMIN=xx");
    Serial.println("  SET INTERVAL=300");
    Serial.println("  SAVE            Save local config to ESP32 NVS");
    Serial.println();
    Serial.println("MAINTENANCE");
    Serial.println("  FACTORY         Clear local NVS and restart");
    Serial.println("  ?               Show this menu");
    Serial.println();
    Serial.println("================================================");
}

void Console_Process(void)
{
    while (Serial.available())
    {
        const char c = (char)Serial.read();

        if ((c == '\r') || (c == '\n'))
        {
            if (gLineIndex > 0)
            {
                Serial.println();
                gLine[gLineIndex] = '\0';
                processCommand(String(gLine));

                gLineIndex = 0;
                memset(gLine, 0, sizeof(gLine));

                Serial.print("> ");
            }

            continue;
        }

        // Allow '?' as a single immediate help command.
        if ((c == '?') && (gLineIndex == 0))
        {
            Serial.println('?');
            Console_PrintHelp();
            Serial.print("> ");
            continue;
        }

        if ((c == '\b') || (c == 0x7F))
        {
            if (gLineIndex > 0)
            {
                gLineIndex--;
                Serial.print("\b \b");
            }

            continue;
        }

        if ((c >= 32) && (c < 127))
        {
            if (gLineIndex < (CONSOLE_BUFFER_LEN - 1))
            {
                gLine[gLineIndex++] = c;
                Serial.print(c);
            }
            else
                gLineIndex = 0;
        }
    }
}

bool Console_WaitForStartupRequest(uint32_t timeoutMs)
{
    Serial.println();
    Serial.print("Press '?' within ");
    Serial.print(timeoutMs / 1000UL);
    Serial.println(" seconds to enter CONFIG mode.");

    const uint32_t start = millis();

    while ((millis() - start) < timeoutMs)
    {
        if (Serial.available())
        {
            const char c = (char)Serial.read();

            if (c == '?')
            {
                Serial.println();
                Serial.println("Console requested.");
                Console_PrintHelp();
                return true;
            }
        }

        delay(5);
    }

    return false;
}
