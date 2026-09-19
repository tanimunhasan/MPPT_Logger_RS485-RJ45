//******************************************************************************
// File: hal_sd.cpp
//******************************************************************************

#include "hal_sd.h"

#include <SPI.h>

#include "../user_config.h"

namespace
{
    bool gInitialised = false;
}

bool HAL_SD_Init(void)
{
    if (gInitialised)
        return true;

    pinMode(TCAN485_SD_CS_PIN, OUTPUT);
    digitalWrite(TCAN485_SD_CS_PIN, HIGH);

    SPI.begin(
        TCAN485_SD_SCLK_PIN,
        TCAN485_SD_MISO_PIN,
        TCAN485_SD_MOSI_PIN,
        TCAN485_SD_CS_PIN);

    delay(20);

    if (!SD.begin(
            TCAN485_SD_CS_PIN,
            SPI,
            TCAN485_SD_SPI_HZ))
    {
        SPI.end();
        return false;
    }

    gInitialised = true;
    return true;
}

void HAL_SD_DeInit(void)
{
    if (gInitialised)
        SD.end();

    SPI.end();

    pinMode(TCAN485_SD_MISO_PIN, INPUT);
    pinMode(TCAN485_SD_MOSI_PIN, INPUT);
    pinMode(TCAN485_SD_SCLK_PIN, INPUT);
    pinMode(TCAN485_SD_CS_PIN, INPUT);

    gInitialised = false;
}

bool HAL_SD_IsInitialised(void)
{
    return gInitialised;
}

bool HAL_SD_Exists(const char *path)
{
    if (!HAL_SD_Init())
        return false;

    return SD.exists(path);
}

File HAL_SD_OpenAppend(const char *path)
{
    if (!HAL_SD_Init())
        return File();

    return SD.open(path, FILE_APPEND);
}

File HAL_SD_OpenRead(const char *path)
{
    if (!HAL_SD_Init())
        return File();

    return SD.open(path, FILE_READ);
}
