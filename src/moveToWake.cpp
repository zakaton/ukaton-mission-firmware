#include "moveToWake.h"
#include "sensor/motionSensor.h"
#include "definitions.h"

#include "ble/ble.h"
#include "battery.h"

#include "WiFi.h"
#include "wifi/webSocket.h"
#include "wifi/udp.h"
#include "wifi/osc.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include <esp_wifi.h>
#include <esp_bt.h>

namespace moveToWake
{
    bool enabled = ENABLE_MOVE_TO_WAKE;
    unsigned long delayUntilSleep = 1000 * 60 * 5;

    void print_wakeup_reason()
    {
        esp_sleep_wakeup_cause_t wakeup_reason;
        wakeup_reason = esp_sleep_get_wakeup_cause();
        switch (wakeup_reason)
        {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup caused by external signal using RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("Wakeup caused by external signal using RTC_CNTL");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup caused by timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println("Wakeup caused by touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            Serial.println("Wakeup caused by ULP program");
            break;
        default:
            Serial.println("Wakeup was not caused by deep sleep");
            break;
        }
    }

    void setup()
    {
#if DEBUG
        print_wakeup_reason();
#endif
    }

    bool initializedStabilityDetectorOnce = false;
    bool isConnected = false;
    bool _isConnected = false;

    constexpr uint16_t stability_detection_delay_ms = 5000;

    bool shouldSleep = false;
    unsigned long sleepTime;
    constexpr uint16_t sleep_delay_ms = 2000;

    unsigned long currentTime;
    void loop()
    {
        currentTime = millis();

        _isConnected = ble::isServerConnected || webSocket::isConnectedToClient() || osc::hasListener() || udp::hasListener();
        if (isConnected != _isConnected)
        {
            isConnected = _isConnected;

            if (motionSensor::isAvailable)
            {
                if (isConnected)
                {
                    // Serial.println("disabling detector!");
                    motionSensor::bno.enableStabilityDetector(0);
                }
                else
                {
                    // Serial.println("enabling detector!");
                    motionSensor::bno.enableStabilityDetector(stability_detection_delay_ms);
                }
            }
        }

        if (motionSensor::isAvailable)
        {
            if (!initializedStabilityDetectorOnce && currentTime > 2000)
            {
                // Serial.println("enabling detector!");
                motionSensor::bno.enableStabilityDetector(stability_detection_delay_ms);
                motionSensor::bno.calibrateGyro();
                initializedStabilityDetectorOnce = true;
            }
        }

        if (
            motionSensor::isAvailable &&
            !shouldSleep &&
            enabled && !isConnected &&
            (currentTime - motionSensor::bno.getLastTimeStabilityDetectorTriggered() > delayUntilSleep) &&
            (currentTime - ble::lastTimeConnected) > delayUntilSleep &&
            (currentTime - webSocket::getLastTimeConnectedToClient()) > delayUntilSleep &&
            (currentTime - osc::getLastTimeConnected()) > delayUntilSleep &&
            (currentTime - udp::getLastTimeConnected()) > delayUntilSleep)
        {
            shouldSleep = true;
            sleepTime = currentTime + sleep_delay_ms;
            motionSensor::bno.enableStabilityDetector(0);
        }

        if (shouldSleep && currentTime > sleepTime)
        {
            sleep();
        }
    }

    void sleep()
    {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        btStop();

        battery::sleep();

        adc_power_off();
        esp_wifi_stop();
        esp_bt_controller_disable();

#if IS_ESP32_C3
        const uint64_t INTERRUPT_PIN_BITMASK = 1 << motionSensor::interrupt_pin;

        // https://github.com/espressif/arduino-esp32/issues/6656
        gpio_set_direction((gpio_num_t)motionSensor::interrupt_pin, GPIO_MODE_INPUT);
        esp_deep_sleep_enable_gpio_wakeup(INTERRUPT_PIN_BITMASK, ESP_GPIO_WAKEUP_GPIO_LOW);
#else
        esp_sleep_enable_ext0_wakeup((gpio_num_t)motionSensor::interrupt_pin, 0);
#endif
        motionSensor::bno.enableSignificantMotionDetector();
        delay(1000);
        Serial.println("Time to sleep...");
        esp_deep_sleep_start();
    }
} // namespace moveToWake