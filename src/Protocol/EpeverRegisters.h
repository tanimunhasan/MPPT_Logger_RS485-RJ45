//******************************************************************************
// File: EpeverRegisters.h
// Purpose: EPEVER Tracer AN Modbus register definitions used by this project.
//******************************************************************************

#ifndef __EPEVER_REGISTERS_H
#define __EPEVER_REGISTERS_H

#include <stdint.h>

namespace EpeverReg
{
    //--------------------------------------------------------------------------
    // Real-time input registers
    //--------------------------------------------------------------------------

    static constexpr uint16_t PV_VOLTAGE              = 0x3100;
    static constexpr uint16_t PV_CURRENT              = 0x3101;
    static constexpr uint16_t PV_POWER_L              = 0x3102;
    static constexpr uint16_t PV_POWER_H              = 0x3103;
    static constexpr uint16_t BATTERY_VOLTAGE         = 0x3104;
    static constexpr uint16_t CHARGE_CURRENT          = 0x3105;
    static constexpr uint16_t CHARGE_POWER_L          = 0x3106;
    static constexpr uint16_t CHARGE_POWER_H          = 0x3107;

    static constexpr uint16_t BATTERY_SOC             = 0x311A;

    // Status
    static constexpr uint16_t BATTERY_STATUS          = 0x3200;
    static constexpr uint16_t CHARGING_STATUS         = 0x3201;

    //--------------------------------------------------------------------------
    // Statistics input registers
    //--------------------------------------------------------------------------

    static constexpr uint16_t ENERGY_TODAY_L          = 0x330C;
    static constexpr uint16_t ENERGY_TODAY_H          = 0x330D;
    static constexpr uint16_t ENERGY_MONTH_L          = 0x330E;
    static constexpr uint16_t ENERGY_MONTH_H          = 0x330F;
    static constexpr uint16_t ENERGY_YEAR_L           = 0x3310;
    static constexpr uint16_t ENERGY_YEAR_H           = 0x3311;
    static constexpr uint16_t ENERGY_TOTAL_L          = 0x3312;
    static constexpr uint16_t ENERGY_TOTAL_H          = 0x3313;

    //--------------------------------------------------------------------------
    // Battery configuration holding registers
    //--------------------------------------------------------------------------

    static constexpr uint16_t BATTERY_TYPE            = 0x9000;
    static constexpr uint16_t BATTERY_CAPACITY        = 0x9001;
    static constexpr uint16_t TEMP_COMPENSATION       = 0x9002;

    static constexpr uint16_t HIGH_VOLT_DISCONNECT    = 0x9003;
    static constexpr uint16_t CHARGING_LIMIT          = 0x9004;
    static constexpr uint16_t OVER_VOLT_RECONNECT     = 0x9005;
    static constexpr uint16_t EQUALIZATION_VOLTAGE    = 0x9006;
    static constexpr uint16_t BOOST_VOLTAGE           = 0x9007;
    static constexpr uint16_t FLOAT_VOLTAGE           = 0x9008;
    static constexpr uint16_t BOOST_RECONNECT         = 0x9009;
    static constexpr uint16_t LOW_VOLT_RECONNECT      = 0x900A;
    static constexpr uint16_t UNDERVOLT_RECOVER       = 0x900B;
    static constexpr uint16_t UNDERVOLT_WARNING       = 0x900C;
    static constexpr uint16_t LOW_VOLT_DISCONNECT     = 0x900D;
    static constexpr uint16_t DISCHARGE_LIMIT         = 0x900E;

    // RTC - write all three registers together.
    static constexpr uint16_t RTC_SECONDS_MINUTES     = 0x9013;
    static constexpr uint16_t RTC_HOURS_DAY           = 0x9014;
    static constexpr uint16_t RTC_MONTH_YEAR          = 0x9015;

    // Charge durations
    static constexpr uint16_t EQUALIZATION_DURATION   = 0x906B;
    static constexpr uint16_t BOOST_DURATION          = 0x906C;
}

#endif // __EPEVER_REGISTERS_H
