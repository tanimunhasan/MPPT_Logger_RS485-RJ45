#include <Arduino.h>
#include "App/Core.h"

void setup()
{
    Core_Init();
    Core_Run();
}

void loop()
{
    // All application flow is handled by Core_Run().
    // OPM_NORMAL finishes by entering ESP32 deep sleep.
}
