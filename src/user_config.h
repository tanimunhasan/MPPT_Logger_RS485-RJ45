//******************************************************************************
// File: user_config.h
// Purpose: Project-wide user configuration.
//******************************************************************************

#ifndef __USER_CONFIG_H
#define __USER_CONFIG_H

#include <stdint.h>

//******************************************************************************
// Firmware identification
//******************************************************************************

#define FW_PRODUCT_NAME                      "EPEVER MPPT LOGGER"
#define FW_VERSION_MAJOR                     1
#define FW_VERSION_MINOR                     0
#define FW_VERSION_RELEASE                   0

//******************************************************************************
// Console / startup
//******************************************************************************

#define DEBUG_BAUD_RATE                      115200UL

// Keep this 0 for lowest deployed energy use. CONFIG/TEST/cold boot still print.
#define NORMAL_WAKE_SERIAL_ENABLE            0

// On a normal cold boot after commissioning, the firmware waits this long
// for '?' before starting the saved NORMAL mode. Deep-sleep timer wakeups
// skip this window to save energy.
#define STARTUP_CONSOLE_WINDOW_MS            10000UL

// TEST mode keeps the MCU awake, logs a sample, and prints its result at
// this period. Six samples exercise the production SD flush path in one minute.
#define TEST_SAMPLE_INTERVAL_MS              10000UL

//******************************************************************************
// Normal logger operation
//******************************************************************************

// Main deployment setting: wake once every 5 minutes.
#define LOGGER_SAMPLE_INTERVAL_SEC           300UL

// Buffer samples in RTC RAM and write them to SD as a batch.
// 6 x 5 minutes = approximately one SD flush every 30 minutes.
#define LOGGER_SD_BUFFER_SAMPLES             6U

// Retry after a recoverable startup/communication error.
#define ERROR_RETRY_SLEEP_SEC                60UL

//******************************************************************************
// EPEVER / Modbus RTU
//******************************************************************************

#define EPEVER_MODBUS_ADDRESS                1U
#define EPEVER_MODBUS_BAUD                   115200UL
#define MODBUS_RETRY_COUNT                   3U
#define MODBUS_RETRY_DELAY_MS                60U

//******************************************************************************
// T-CAN485 hardware mapping
// PIN 4 ORANGE CABLE ------ B
// PIN 5 YELLOW CABLE ------- A
// PIN 7 BLUE CABLE ------- GND
// Official LilyGO T-CAN485 mapping:
//   TX=22, RX=21, CALLBACK=17, RS485 EN=19, 5V booster EN=16.
// Some newer/community-reported hardware revisions use GPIO19 for RS485 EN.
// If RS485 is silent on your exact Ticha/T-CAN485 board, change only
// TCAN485_RS485_EN_PIN from 9 to 19 and retest.
//******************************************************************************

#define TCAN485_RS485_TX_PIN                 22
#define TCAN485_RS485_RX_PIN                 21
#define TCAN485_RS485_CALLBACK_PIN           17
#define TCAN485_RS485_EN_PIN                 19
#define TCAN485_5V_BOOST_EN_PIN              16

#define TCAN485_RS485_EN_ACTIVE_LEVEL        HIGH
#define TCAN485_RS485_CALLBACK_ACTIVE_LEVEL  HIGH
#define TCAN485_5V_BOOST_ACTIVE_LEVEL        HIGH

#define TCAN485_WS2812_PIN                   4

// TF / microSD pins
#define TCAN485_SD_MISO_PIN                  2
#define TCAN485_SD_MOSI_PIN                  15
#define TCAN485_SD_SCLK_PIN                  14
#define TCAN485_SD_CS_PIN                    13
#define TCAN485_SD_SPI_HZ                    4000000UL

//******************************************************************************
// ESP32 NVS
//******************************************************************************

#define NVS_NAMESPACE                        "epeverlog"
#define NVS_CONFIG_KEY                       "app_conf"

//******************************************************************************
// Default EPEVER battery configuration
//
// IMPORTANT:
// These values are copied into APP_CONF on the first ever boot.
// Replace the voltage placeholders with the exact AMPS 12V 70Ah LiFePO4
// battery / EPEVER settings before using command WC.
//
// You can also modify the values at runtime in CONFIG mode with SET commands.
// WC validates the values before writing anything to the MPPT.
//******************************************************************************

// 0 is the traditional EPEVER "USER" battery profile.
#define DEF_BATTERY_TYPE                     0U
#define DEF_BATTERY_CAPACITY_AH              70U

// Register 0x9002, represented here as engineering value scaled by x100
// in the EPEVER register. Keep 0.00 unless your confirmed configuration
// requires a non-zero temperature compensation value.
#define DEF_TEMP_COMPENSATION                0.00f

// 0x9003 .. 0x900E
#define DEF_HIGH_VOLTAGE_DISCONNECT_V        0.00f
#define DEF_CHARGING_LIMIT_V                 0.00f
#define DEF_OVER_VOLTAGE_RECONNECT_V         0.00f
#define DEF_EQUALIZATION_V                   0.00f
#define DEF_BOOST_V                          0.00f
#define DEF_FLOAT_V                          0.00f
#define DEF_BOOST_RECONNECT_V                0.00f
#define DEF_LOW_VOLTAGE_RECONNECT_V          0.00f
#define DEF_UNDERVOLTAGE_RECOVER_V           0.00f
#define DEF_UNDERVOLTAGE_WARNING_V           0.00f
#define DEF_LOW_VOLTAGE_DISCONNECT_V         0.00f
#define DEF_DISCHARGE_LIMIT_V                0.00f

// 0x906B / 0x906C, minutes.
#define DEF_EQUALIZATION_DURATION_MIN        0U
#define DEF_BOOST_DURATION_MIN               0U

#endif // __USER_CONFIG_H
