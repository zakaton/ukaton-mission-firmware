#pragma once
#ifndef _HAPTICS_
#define _HAPTICS_

#include "Adafruit_DRV2605.h"
#include "definitions.h"

namespace haptics
{
    void vibrate(uint8_t *data, size_t length);

    void begin();
    void setup();
    void loop();
} // namespace haptics

#endif // _HAPTICS_