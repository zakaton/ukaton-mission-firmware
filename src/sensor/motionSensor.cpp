#include "definitions.h"
#include "motionSensor.h"
#include <lwipopts.h>
#include <i2cscan.h>

namespace motionSensor
{
    BNO080 bno;

    bool isValidDataType(DataType dataType)
    {
        return dataType < DataType::COUNT;
    }
    uint8_t calibration[(uint8_t)CalibrationType::COUNT];

    bool wroteFullCalibration = false;
    void updateCalibration()
    {
        if (isAvailable)
        {
            calibration[(uint8_t)CalibrationType::ACCELEROMETER] = bno.getAccelAccuracy();
            calibration[(uint8_t)CalibrationType::GYROSCOPE] = bno.getGyroAccuracy();
            calibration[(uint8_t)CalibrationType::MAGNETOMETER] = bno.getMagAccuracy();
            calibration[(uint8_t)CalibrationType::QUATERNION] = bno.getQuatAccuracy();
        }
    }
    unsigned long lastCalibrationUpdateTime = 0;
    const uint16_t calibration_delay_ms = 1000;

    unsigned long lastTimeMoved = 0;
    void interruptCallback()
    {
        // Serial.println("interrupt");
    }

    uint8_t imuAddress = 0;
    bool isAvailable = false;

    void setup()
    {
        imuAddress = I2CSCAN::pickDevice(0x4A, 0x4B, false);

        if (imuAddress)
        {
            Wire.setClock(100000);
            // bno.enableDebugging();
            bno.begin(imuAddress, Wire, interrupt_pin);

            // bno.calibrateAll();

            attachInterrupt(digitalPinToInterrupt(interrupt_pin), interruptCallback, FALLING);
            interrupts();
            Wire.setClock(I2C_SPEED);

            isAvailable = true;
        }
        else
        {
            Serial.println("imu not found");
        }
    }

    unsigned long currentTime;
    void loop()
    {
        if (!imuAddress)
        {
            return;
        }

        bno.dataAvailable();

        currentTime = millis();

        if (currentTime >= lastCalibrationUpdateTime + calibration_delay_ms)
        {
            updateCalibration();
            lastCalibrationUpdateTime = currentTime - (currentTime % calibration_delay_ms);
        }
    }
} // namespace motionSensor