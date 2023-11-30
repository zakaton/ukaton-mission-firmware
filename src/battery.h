#pragma once
#ifndef _BATTERY_
#define _BATTERY_

#include <Arduino.h>

namespace battery
{
    extern unsigned long lastUpdateBatteryLevelTime;
    uint8_t getLevel();

    void setup();
    void loop();
    void sleep();
}

#endif // _BATTERY_