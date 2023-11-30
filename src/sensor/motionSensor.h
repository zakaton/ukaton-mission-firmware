#pragma once
#ifndef _MOTION_SENSOR_
#define _MOTION_SENSOR_

#include <SparkFun_BNO080_Arduino_Library.h>
#include "driver/adc.h"

#ifndef GPIO_NUM_27
#define GPIO_NUM_27 27
#endif

namespace motionSensor
{
    extern BNO080 bno;

    enum class DataType : uint8_t
    {
        ACCELERATION,
        GRAVITY,
        LINEAR_ACCELERATION,
        ROTATION_RATE,
        MAGNETOMETER,
        QUATERNION,
        COUNT
    };
    bool isValidDataType(DataType dataType);
    enum class DataSize : uint8_t
    {
        ACCELERATION = 6,
        GRAVITY = 6,
        LINEAR_ACCELERATION = 6,
        ROTATION_RATE = 6,
        MAGNETOMETER = 6,
        QUATERNION = 8,
        TOTAL = 38
    };

    enum class StabilityClassification : uint8_t
    {
        UNKNOWN = 0,
        ON_TABLE,
        STATIONARY,
        STABLE,
        MOTION,
        RESERVED
    };

    enum class CalibrationType : uint8_t
    {
        ACCELEROMETER,
        GYROSCOPE,
        MAGNETOMETER,
        QUATERNION,
        COUNT
    };
    extern uint8_t calibration[(uint8_t)CalibrationType::COUNT];
    extern unsigned long lastCalibrationUpdateTime;

#if IS_2023_DESIGN
    constexpr auto interrupt_pin = GPIO_NUM_13;
#else
    constexpr auto interrupt_pin = IS_ESP32_S3 ? GPIO_NUM_8 : IS_ESP32_C3 ? GPIO_NUM_4
                                                                          : GPIO_NUM_27;
#endif

    extern bool isAvailable;
    void setup();
    void loop();
} // namespace motionSensor

#endif // _MOTION_SENSOR_