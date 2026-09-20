//******************************************************************************
// File: hal_power.cpp
//******************************************************************************

#include "hal_power.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp32-hal-bt.h>
#include <driver/gpio.h>

#include "../user_config.h"
#include "hal_rs485.h"
#include "hal_sd.h"

namespace
{
    void preparePinsForDeepSleep(void)
    {
        HAL_RS485_PrepareForSleep();
        HAL_SD_PrepareForSleep();
        gpio_deep_sleep_hold_en();
    }
}

void HAL_Power_Init(void)
{
    gpio_deep_sleep_hold_dis();

    // The logger does not require radios.
    WiFi.mode(WIFI_OFF);
    btStop();

    // Ensure the addressable LED is not intentionally driven.
    pinMode(TCAN485_WS2812_PIN, OUTPUT);
    digitalWrite(TCAN485_WS2812_PIN, LOW);
}

bool HAL_Power_IsTimerWake(void)
{
    return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
}

const char* HAL_Power_GetWakeReasonString(void)
{
    switch (esp_sleep_get_wakeup_cause())
    {
        case ESP_SLEEP_WAKEUP_TIMER:
            return "DEEP SLEEP TIMER";

        case ESP_SLEEP_WAKEUP_EXT0:
            return "EXT0";

        case ESP_SLEEP_WAKEUP_EXT1:
            return "EXT1";

        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            return "TOUCH";

        case ESP_SLEEP_WAKEUP_ULP:
            return "ULP";

        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            return "COLD/RESET";
    }
}

const char* HAL_Power_GetResetReasonString(void)
{
    switch (esp_reset_reason())
    {
        case ESP_RST_POWERON:   return "POWER ON";
        case ESP_RST_EXT:       return "EXTERNAL RESET";
        case ESP_RST_SW:        return "SOFTWARE RESET";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INT WDT";
        case ESP_RST_TASK_WDT:  return "TASK WDT";
        case ESP_RST_WDT:       return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEP SLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "OTHER";
    }
}

[[noreturn]] void HAL_Power_DeepSleep(uint32_t seconds)
{
    Serial.flush();

    preparePinsForDeepSleep();

    esp_sleep_enable_timer_wakeup(
        (uint64_t)seconds * 1000000ULL);

    delay(10);
    esp_deep_sleep_start();

    while (true) {}
}

[[noreturn]] void HAL_Power_Hibernate(void)
{
    // No timer wake source. Device remains in deep sleep until external reset
    // / power cycle.
    Serial.flush();
    preparePinsForDeepSleep();
    delay(10);
    esp_deep_sleep_start();

    while (true) {}
}

[[noreturn]] void HAL_Power_Reset(void)
{
    Serial.flush();
    delay(50);
    ESP.restart();

    while (true) {}
}
