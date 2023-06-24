#include <Arduino.h>
#include <Wire.h>
#include <i2cscan.h>

#include "definitions.h"
#include "battery.h"
#include "information/name.h"
#include "information/type.h"
#include "sensor/motionSensor.h"
#include "moveToWake.h"
#include "sensor/pressureSensor.h"
#include "steps.h"
#include "weight/weightData.h"
#include "sensor/sensorData.h"
#include "haptics.h"
#include "ble/ble.h"
#include "wifi/wifi.h"

void setup()
{
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);

    Serial.begin(115200);
    delay(2000);
    I2CSCAN::clearBus(PIN_IMU_SDA, PIN_IMU_SCL);
    Wire.end();
    delay(2000);
    Wire.begin(PIN_IMU_SDA, PIN_IMU_SCL);
    Wire.setClock(I2C_SPEED);

    battery::setup();
    name::setup();
    type::setup();
    motionSensor::setup();
#if ENABLE_MOVE_TO_WAKE
    moveToWake::setup();
#endif
    pressureSensor::setup();
    steps::setup();
    haptics::setup();
    wifi::setup();
    ble::setup();

    Wire.setClock(DEFAULT_I2C_SPEED);
}

void loop()
{
    battery::loop();
    motionSensor::loop();
#if ENABLE_MOVE_TO_WAKE
    moveToWake::loop();
#endif
    sensorData::loop();
    steps::loop();
    weightData::loop();
    haptics::loop();
    wifi::loop();
    ble::loop();
}