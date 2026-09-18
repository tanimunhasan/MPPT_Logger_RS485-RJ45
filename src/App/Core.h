//******************************************************************************
// File: Core.h
// Purpose: Application core types, state and top-level entry points.
//******************************************************************************

#ifndef __CORE_H
#define __CORE_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include "../Protocol/EpeverProtocol.h"

//******************************************************************************
// Persistent configuration identity
//******************************************************************************

#define APP_CONFIG_MAGIC              0x45504C47UL  // "EPLG"
#define APP_CONFIG_SCHEMA_VERSION     1U

//******************************************************************************
// Operation modes
//
// R=1 CONFIG
// R=2 TEST
// R=3 NORMAL
// R=4 HIBERNATE
// R=9 RESET
//******************************************************************************

typedef enum
{
    OPM_ERROR_START = 0,

    OPM_CONFIG      = 1,
    OPM_TEST        = 2,
    OPM_NORMAL      = 3,
    OPM_HIBERNATE   = 4,

    OPM_RESET       = 9

} OPERATION_MODE_TYPE_ENUM;

//******************************************************************************
// Persistent configuration
//******************************************************************************

typedef struct
{
    uint32_t Magic;
    uint16_t SchemaVersion;
    uint16_t Reserved;

    // Saved deployment mode. Maintenance CONFIG/TEST mode changes are
    // normally temporary unless explicitly saved by application logic.
    OPERATION_MODE_TYPE_ENUM RunMode;

    uint32_t SampleIntervalSec;

    bool MpptConfigured;
    uint8_t ReservedFlags[3];

    uint32_t MpptConfigRevision;

    MPPT_CONFIG_TYPE Mppt;

    uint32_t Crc32;

} APP_CONFIG_TYPE;

//******************************************************************************
// Runtime-only state
//******************************************************************************

typedef struct
{
    OPERATION_MODE_TYPE_ENUM RunMode;

    bool FirstBoot;
    bool TimerWake;
    bool ConfigDirty;
    bool LastMpptReadOk;
    bool LastSdWriteOk;

    uint32_t WakeCount;
    uint32_t ModbusErrorCount;
    uint32_t SdErrorCount;

    MPPT_SAMPLE_TYPE LastSample;

} APP_RUNTIME_TYPE;

//******************************************************************************
// Globals
//******************************************************************************

extern APP_RUNTIME_TYPE APP;
extern APP_CONFIG_TYPE APP_CONF;

//******************************************************************************
// Core API
//******************************************************************************

void Core_Init(void);
void Core_Run(void);

void Core_LoadDefaultConfiguration(APP_CONFIG_TYPE *config);

OPERATION_MODE_TYPE_ENUM GetOperationMode(void);

bool SetOperationMode(
    OPERATION_MODE_TYPE_ENUM mode,
    bool persistMode);

void PrintOperationMode(
    OPERATION_MODE_TYPE_ENUM mode);

const char* GetOperationModeName(
    OPERATION_MODE_TYPE_ENUM mode);

void Core_MarkConfigDirty(bool dirty);
void Core_RecordModbusError(void);
void Core_RecordSdError(void);

void Core_PrintSystemStatus(void);
void Core_PrintLocalConfiguration(void);

#endif // __CORE_H
