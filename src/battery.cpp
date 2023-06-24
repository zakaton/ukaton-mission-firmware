#include "battery.h"
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include <i2cscan.h>

namespace battery
{
    unsigned long currentTime = 0;

    SFE_MAX1704X lipo;

    float voltage = 0;
    float soc = 0;
    uint8_t level = 0;

    uint8_t getLevel()
    {
        return level;
    }

    constexpr uint32_t check_battery_level_delay_ms = 1000 * 10; // check every 10 seconds
    unsigned long lastCheckBatteryLevelTime = 0;
    unsigned long lastUpdateBatteryLevelTime = 0;
    void checkBatteryLevel()
    {
        auto _voltage = lipo.getVoltage();
        auto _soc = lipo.getSOC();
        uint8_t _level = ceil(_soc);

        // Serial.printf("voltage: %fv\n", _voltage);
        Serial.printf("soc: %f%%\n", _soc);
        Serial.printf("level: %d%%\n", _level);

        if (_level != level)
        {
            soc = _soc;
            voltage = _voltage;
            level = _level;
            lastUpdateBatteryLevelTime = currentTime;
        }
    }

    uint8_t deviceAddress = 0;
    bool isClone = false;
    void setup()
    {
        deviceAddress = I2CSCAN::pickDevice(MAX1704x_ADDRESS, MAX1704x_CLONE_ADDRESS, false);
        isClone = deviceAddress == MAX1704x_CLONE_ADDRESS;
        if (!isClone && deviceAddress)
        {
            lipo.enableDebugging();
            Serial.println("gonna begin battery gauge");
            if (!lipo.begin())
            {
                Serial.println("MAX17043 not detected. Please check wiring");
            }
            lipo.quickStart();
            checkBatteryLevel();
            // lipo.setThreshold(20);
        }
        else
        {
            Serial.println("[battery] sensor is a clone");
        }
    }

    void loop()
    {
        if (!deviceAddress || isClone)
        {
            return;
        }

        currentTime = millis();
        if (currentTime - lastCheckBatteryLevelTime > check_battery_level_delay_ms)
        {
            checkBatteryLevel();
            lastCheckBatteryLevelTime = currentTime - (currentTime % check_battery_level_delay_ms);
        }
    }
} // namespace battery